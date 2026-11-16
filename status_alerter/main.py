"""
Send ntfy.sh alerts for hive or gateway down.
"""

import datetime
import requests
import shelve
import pprint
import psycopg2

# === USER INPUT ===

NTFY_URL = "https://ntfy.sh/ganderson_hivestatus"

NOTIFICATION_REPEAT_COUNT = 3
NOTIFICATION_REPEAT_INTERVAL = -1 # mins

NOTIFICATION_DOWNTIME_START = "21:00" # local time
NOTIFICATION_DOWNTIME_END = "08:00" 


IGNORE_NODES = [0] # node ids to ignore, ones that are in testing
IGNORE_GATEWAYS = [0] # ditto for gateways

# === END USER INPUT ===

def is_downtime(now):
    start = datetime.time.fromisoformat(NOTIFICATION_DOWNTIME_START)
    end = datetime.time.fromisoformat(NOTIFICATION_DOWNTIME_END)
    t = now.time()
    if start <= end:
        return start <= t < end
    return t >= start or t < end

def should_notify(alerts, key, now):
    pprint.pprint(alerts)
    if is_downtime(now):
        return False

    entry = alerts.get(key)
    if entry is None:
        # haven't alerted on this yet
        return True

    if entry['count'] >= NOTIFICATION_REPEAT_COUNT:
        return False

    return (now - entry['last']) >= datetime.timedelta(minutes=NOTIFICATION_REPEAT_INTERVAL)

def notify(alerts, key, title, message, now, urgent):
    if urgent:
        headers={"Priority": "5", "Title": title}
    else:
        headers={"Title": title}

    requests.post(
            NTFY_URL,
            data=message.encode('utf-8'),
            headers=headers,
            )

    entry = alerts.get(key, {'count': 0})
    alerts[key] = {'last': now, 'count': entry['count'] + 1}

# Fetch statuses from PSQL
conn = psycopg2.connect("host=localhost port=5432 dbname=hive user=grafana password=grafana!")
cur = conn.cursor()

cur.execute("SELECT * from api.gateway_status")
gateway_rows = cur.fetchall()

cur.execute("SELECT * from api.node_status")
node_rows = cur.fetchall()
conn.close()

now = datetime.datetime.now()

with shelve.open("notifications", writeback=True) as shelf:
    alerts = shelf.get("alerts", {})
    active = set()

    for (gateway_name, gateway_id, last_heard, secs_down, stale) in gateway_rows:
        if not stale or gateway_id in IGNORE_GATEWAYS:
            continue

        key = f'gateway:{gateway_id}'
        active.add(key)
        if should_notify(alerts, key, now):
            notify(
                    alerts, key,
                    f"{gateway_name} down",
                    f'Gateway "{gateway_name}" (id #{gateway_id}) down since {last_heard}',
                    now, 
                    False)
    
    for (node_name, node_id, last_heard, secs_down, stale) in node_rows:
        if not stale or node_id in IGNORE_NODES:
            continue

        key = f'node:{node_id}'
        active.add(key)
        if should_notify(alerts, key, now):
            notify(
                    alerts, key,
                    f"{node_name} down",
                    f'Node "{node_name}" (id #{node_id}) down since {last_heard}',
                    now, 
                    False)
    

    # clear old ones and save the shelf
    alerts = {k: v for k,v in alerts.items() if k in active}
    shelf['alerts'] = alerts
