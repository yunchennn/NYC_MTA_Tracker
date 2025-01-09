pip install requests
pip install --upgrade google-api-python-client
pip install --upgrade gtfs-realtime-bindings
pip install Flask
pip install pandas


<!-- https://stackoverflow.com/questions/3855127/find-and-kill-process-locking-port-3000-on-mac -->
lsof -i tcp:5000
kill <PID>