"""
Send ntfy.sh alerts for hive or gateway down.
"""

import datetime
import requests
import shelve

import psycopg2

# === USER INPUT ===

NTFY_URL = "https://ntfy.sh/ganderson_hivestatus"

NOTIFICATION_REPEAT_COUNT = 3
NOTIFICATION_REPEAT_INTERVAL = -1 # mins

NOTIFICATION_DOWNTIME_START = "21:00" # local time
NOTIFICATION_DOWNTIME_END = "08:00" 

IGNORE_NODES = [0] # node ids to ignore, ones that are in testing

# === END USER INPUT ===

gateways_down = []
nodes_down = []
with shelve.open("notifications") as shelf:
    # Are there active alerts?
    conn = psycopg2.connect("host=localhost port=5432 dbname=hive user=grafana password=grafana!")
    cur = conn.cursor()

    cur.execute("SELECT * FROM api.gateway_status")
    for (gateway_name, gateway_id, last_heard, secs_down, gateway_stale) in cur:
        if gateway_stale:
            gateways_down.append([gateway_name, gateway_id, last_heard, secs_down])

    for gateway in gateways_down:
        if (datetime.datetime.now() - shelf.get("gateways", {}).get(gateway_id, {}).get("last_notification_time", datetime.datetime.now())) >= datetime.timedelta(minutes=NOTIFICATION_REPEAT_INTERVAL): 
            shelf["gateways"][gateway_id]["last_notification_time"] = datetime.datetime.now()
            print("notifying")
        message = f'Gateway "{gateway_name}" (id #{gateway_id}) down since {last_heard}'

        requests.post(NTFY_URL, data=message.encode("utf-8"), headers={"Title": f"{gateway_name} down"})

