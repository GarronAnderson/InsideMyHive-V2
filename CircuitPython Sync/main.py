"""
Receive LoRa packets from the beehive monitor and bridge to PostgREST on a Pi.
"""

# Imports

import json
from os import getenv
import struct
import time

import board
import busio
from digitalio import DigitalInOut
import rtc
import adafruit_datetime as datetime

from adafruit_esp32spi import adafruit_esp32spi
import adafruit_connection_manager
import adafruit_requests
import adafruit_ntp

import adafruit_datetime as datetime

import adafruit_logging as logging

from adafruit_rfm9x import RFM9x

from helpers import StatusLED

# === USER INPUT ===

LORA_FREQ = 915.0  # MHz
PI_URL = "http://192.168.4.110:3001/readings"

# packet format
rx_packet_format = "<BBHIBHIhHhHHHH"
tx_packet_format = "<HH"
time_packet_format = "<HHI"

FLAG_TIME_REQUEST = 0x1
FLAG_INT_HDC3022_ERROR = 0x2
FLAG_EXT_HDC3022_ERROR = 0x4
FLAG_BMP585_ERROR = 0x8
FLAG_NAU7802_ERROR = 0x10
FLAG_TEST_PACKET = 0x20

PACKET_VERSION = 3

# === END USER INPUT ===

# status leds
# fmt: off
power_led   = StatusLED(board.A3)
wifi_led    = StatusLED(board.A0)
lora_led    = StatusLED(board.A1)
wifi_tx_led = StatusLED(board.A2)
# fmt: on

# blinkenlights!
power_led.on()
time.sleep(0.15)
wifi_led.on()
time.sleep(0.15)
lora_led.on()
time.sleep(0.15)
wifi_tx_led.on()
time.sleep(1)

power_led.off()
time.sleep(0.15)
wifi_led.off()
time.sleep(0.15)
lora_led.off()
time.sleep(0.15)
wifi_tx_led.off()
time.sleep(1.5)

power_led.on()

# LoRa Setup


def crc16_ccitt(data):
    crc = 0xFFFF

    for byte in data:
        crc ^= byte << 8

        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ 0x1021
            else:
                crc <<= 1

            crc &= 0xFFFF

    return crc


rfm_cs = DigitalInOut(board.D25)
rfm_reset = DigitalInOut(board.D24)

lora = RFM9x(board.SPI(), rfm_cs, rfm_reset, LORA_FREQ)
lora.tx_power = 23
lora.spreading_factor = 7

lora.receive_timeout = 10

# WiFi setup

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

ntp = adafruit_ntp.NTP(pool, tz_offset=0, cache_seconds=3600)

ntp_success = False
for ntp_retries in range(5):
    try:
        rtc.RTC().datetime = ntp.datetime
        print("set RTC to NTP")
        ntp_success = True
        break
    except Exception as e:
        print(f"NTP sync failed with {e}, retrying (#{ntp_retries})")

if not ntp_success:
    raise Exception("NTP sync failed")

jwt = getenv("JWT_SECRET")
header = {
    "Authorization": f"Bearer {jwt}",
    "Content-Type": "application/json",
    "Prefer": "return=representation",
}

wifi_led.on()

# end WiFi setup

print("mainloop start")

data_to_upload = []

while True:
    lora_led.on()
    raw_packet = lora.receive(with_header=True, timeout=0.2)
    lora_led.off()
    if raw_packet is None:
        raw_packet = lora.receive(with_header=True)
    if (raw_packet is not None) and (len(data_to_upload) < 50):
        # we got a packet and have space for it
        try:
            packet = struct.unpack(rx_packet_format, raw_packet)
        except RuntimeError:
            continue
        (
            packet_version,
            node_id,
            sequence,
            timestamp,
            flags,
            backlog,
            weight,
            temperature,
            humidity,
            ext_temp,
            ext_humid,
            baro_pressure,
            rain_tips,
            crc,
        ) = packet

        calculated_crc = crc16_ccitt(raw_packet[:-2])
        
        temperature /= 100
        humidity /= 100
        baro_pressure /= 10

        # fmt: off
        int_hdc3022_error = bool(flags & FLAG_INT_HDC3022_ERROR)
        ext_hdc3022_error = bool(flags & FLAG_EXT_HDC3022_ERROR)
        bmp585_error      = bool(flags & FLAG_BMP585_ERROR)
        nau7802_error     = bool(flags & FLAG_NAU7802_ERROR)
        test_packet       = bool(flags & FLAG_TEST_PACKET)
        # fmt: on

        print(
            f"timestamp {timestamp}\nweight {weight}\nflags: {flags}\nbacklog: {backlog} packets"
        )
        print(f"crc: {crc}, calc crc: {calculated_crc}")
        print(f"SNR: {lora.snr}, RSSI: {lora.rssi}")

        if calculated_crc == crc:
            print("Good CRC, sending ACK")
            if flags & FLAG_TIME_REQUEST:
                print("sending time update")
                ack = struct.pack(
                    time_packet_format, sequence, calculated_crc, int(time.time())
                )
            else:
                ack = struct.pack(tx_packet_format, sequence, calculated_crc)
            lora_led.on()
            lora.send(ack)
            lora_led.off()
            timestamp_isoformat = (
                datetime.datetime.fromtimestamp(timestamp).isoformat() + "+00"
            )
            now_isoformat = datetime.datetime.now().isoformat() + "+00"

            if timestamp < 1767225600:
                suspect_packet = True  # rtc wasn't set
            elif packet_version != PACKET_VERSION:
                suspect_packet = True
            else:
                suspect_packet = False

            data = {
                "node_id": node_id,
                "ts": timestamp_isoformat,
                "ts_rxd": now_isoformat,
                "backlog": backlog,
                "raw_weight": weight,
                "hive_temp_c": temperature,
                "hive_humid_pct": humidity,
                "external_temp_c": ext_temp,
                "external_humid_pct": ext_humid,
                "baro_pressure_hpa": baro_pressure,
                "rain_tips": rain_tips,
                "suspect_packet": suspect_packet,
                "lora_rssi_dbm": lora.rssi,
                "lora_snr_db": lora.snr,
                "int_hdc3022_error": int_hdc3022_error,
                "ext_hdc3022_error": ext_hdc3022_error,
                "bmp585_error": bmp585_error,
                "nau7802_error": nau7802_error,
                "test_packet": test_packet,
            }

            print("adding to queue")
            data_to_upload.append(data)
        else:
            print("CRC fail :(")
    else:
        if data_to_upload:
            print("Posting to Pi")
            wifi_tx_led.on()
            try:
                for data in data_to_upload:
                    res = requests.post(PI_URL, json=data, headers=header)
                    print(f"Posted, got <{res.text}>")
                wifi_tx_led.off()
                data_to_upload = []
            except Exception as e:
                print(f"Failed to POST with {e}")
