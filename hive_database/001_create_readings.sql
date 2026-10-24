CREATE TABLE readings (
    node_id integer,
	ts timestamptz,
	ts_rxd timestamptz NOT NULL,
	ts_ingest timestamptz DEFAULT NOW(),
	raw_weight integer,
	hive_temp_c real,
	hive_humid_pct real,
	external_temp_c real,
	external_humid_pct real,
	baro_pressure_hpa real,
	rain_tips integer,
	lora_rssi_dbm real,
	lora_snr_db real,
	backlog integer,
	suspect_packet boolean,
	PRIMARY KEY (node_id, ts)
);
