CREATE TABLE calibration (
	effective_from timestamptz,
	node_id integer REFERENCES hives (node_id),
	weight_tare integer,
	counts_per_kg double precision,
	mm_per_tip double precision,
        scale_temp_coefficient double precision,
	notes VARCHAR(150),
	PRIMARY KEY (node_id, effective_from)
);
