"""
Test PostgreSQL connection.
Send a request with test data for the system to use.
"""

from os import getenv

import adafruit_connection_manager
import adafruit_requests
import board
import busio
from digitalio import DigitalInOut
import json
import rtc
import time
import adafruit_datetime as datetime

from adafruit_esp32spi import adafruit_esp32spi
import adafruit_ntp

# === USER INPUT ===

PI_URL = "http://192.168.4.110:3001/readings"

# === WIFI SETUP ===

ssid = getenv("CIRCUITPY_WIFI_SSID")
password = getenv("CIRCUITPY_WIFI_PASSWORD")

# Featherwing Wiring
esp32_cs = DigitalInOut(board.D13)
esp32_ready = DigitalInOut(board.D11)
esp32_reset = DigitalInOut(board.D12)

spi = board.SPI()

esp = adafruit_esp32spi.ESP_SPIcontrol(spi, esp32_cs, esp32_ready, esp32_reset)

pool = adafruit_connection_manager.get_radio_socketpool(esp)
ssl_context = adafruit_connection_manager.get_radio_ssl_context(esp)
requests = adafruit_requests.Session(pool, ssl_context)

if esp.status == adafruit_esp32spi.WL_IDLE_STATUS:
    print("ESP32 found and in idle mode")

print("Connecting to AP...")
while not esp.is_connected:
    try:
        esp.connect_AP(ssid, password)
    except OSError as e:
        print("could not connect to AP, retrying: ", e)
        continue
print("Connected to", esp.ap_info.ssid, "\tRSSI:", esp.ap_info.rssi)
print("My IP address is", esp.ipv4_address)

ntp = adafruit_ntp.NTP(pool, tz_offset=-5, cache_seconds=3600)

# === END WIFI SETUP ===

ntp_timestamp = ntp.datetime
print(f"NTP: {ntp_timestamp}")

rtc.RTC().datetime = ntp.datetime
print("set RTC to NTP")

jwt = getenv("JWT_SECRET")

now_isoformat = datetime.datetime.now().isoformat() + "-05"

header = {
    "Authorization": f"Bearer {jwt}",
    "Content-Type": "application/json",
    "Prefer": "return=representation",
}
data = {
    "node_id": 0,
    "ts": now_isoformat,
    "ts_rxd": now_isoformat,
    "backlog": 0,
    "raw_weight": 22345,
    "hive_temp_c": 42,
    "hive_humid_pct": 42,
    "external_temp_c": 0,
    "external_humid_pct": 0,
    "baro_pressure_hpa": 1000,
    "rain_tips": 0,
    "suspect_packet": False,
    "lora_rssi_dbm": 123,
    "lora_snr_db": 2,
}

print("POSTing to Pi")
res = requests.post(PI_URL, json=data, headers=header)
print(f"Posted, got <{res.text}>")
