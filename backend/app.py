from flask import Flask, render_template, request, jsonify
import json
import requests
from google.transit import gtfs_realtime_pb2
from google.protobuf.json_format import MessageToJson
import json
import collections
import time
from datetime import datetime
import pandas as pd
import os

trainId = ['E', 'F', 'R']
config = 'config.json'
stops_file = './data/stops.csv'
stop_name = '63 Dr-Rego Park'
ori = 'S'


app = Flask(__name__)
def stops(stops_file, stop, ori):
    df = pd.read_csv(stops_file)
    df.set_index('stop_id', inplace=True)
    stops_dict = df.to_dict(orient='index')
    stop_name_li = set()
    for k, v in stops_dict.items():
        stop_name = v.get('stop_name')
        parent_station = v.get('parent_station')
        if stop_name == stop and pd.notna(parent_station):
            stop_name_li.add(parent_station)
    for stop in stop_name_li:
        return stop+ori
    return False


def get_real_time_data(trainId, config, stops_file, stop, ori):
    with open(config, 'r') as config_file:
        config = json.load(config_file)
    
    stopId = stops(stops_file, stop, ori)

    API_KEY = config['api_key']
    headers = {'x-api-key': API_KEY}

    lines = config['subway_lines']
    train_sch = []
    train_dic = collections.defaultdict(set)
    for train in trainId:
        for line in lines:
            if train == line['id']:
                apiEndpoint = line['url']
                response = requests.get(apiEndpoint, headers=headers)
                feed = gtfs_realtime_pb2.FeedMessage()
                feed.ParseFromString(response.content)
                json_data = MessageToJson(feed)
                file_path = f'./data/{line["resp"]}'
                with open(file_path, 'w') as json_file:
                    json_file.write(json_data)

                try:
                    with open(file_path, 'r') as f:
                        data = json.load(f)
                except FileNotFoundError:
                    print(f"File not found: {file_path}")
                except json.decoder.JSONDecodeError as e:
                    print(f"Error decoding JSON: {e}")
                ent_dic = data['entity']
                all_time = []

                

                for i, each in enumerate(ent_dic):
                    try:
                        routeId = each["tripUpdate"]["trip"]["routeId"]
                        stopTimeUpdate = ent_dic[i]["tripUpdate"]["stopTimeUpdate"]
                        if routeId == train:
                            for stop_time in stopTimeUpdate:
                                if stop_time["stopId"] == stopId:
                                    train_dic[train].add(int(stop_time["arrival"]["time"]))
                    except:
                        continue
                
                train_dic[train] = sorted(train_dic[train])
                
    current_time = int(time.time())
    dt_object = datetime.fromtimestamp(current_time)

    top_two = []
    for line, times in train_dic.items():
        if times:
            for timeline in times:
                # dt_object = datetime.fromtimestamp(timeline)
                till = int((timeline-current_time)/60)
                if till >= 6:
                    if top_two and len(top_two)<2: #==1
                        top_two.append([line, timeline])
                        top_two = sorted(top_two, key=lambda x:x[1])
                    
                    elif len(top_two) == 2: #==2
                        if timeline < top_two[0][1]:
                            top_two.pop()
                            top_two.insert(0, [line, timeline])
                        elif timeline < top_two[-1][1]:
                            top_two[-1] = [line, timeline]
                        
                        else: #==0
                            continue

                    elif not top_two:
                        top_two.append([line, timeline])
    
    train_data = ''
    if not top_two:
        train_data = 'NO MTA SERVICE'
        print('NO MTA SERVICE')


    else:
        
        for each in top_two:
            dt_object = datetime.fromtimestamp(each[1])
            till = int((each[1]-current_time)/60)
            train_data = f'{train_data}({each[0]}) ETA {till} min\n'
        train_data = train_data.rstrip('\n')
    print(train_data)
    return train_data

@app.route('/')
def mta_tracker():
    
    train_data = get_real_time_data(trainId, config, stops_file, stop_name, ori)

    return train_data
    # return 'hello'


if __name__ == '__main__':
    
    app.run(host='0.0.0.0', debug=True)

    
