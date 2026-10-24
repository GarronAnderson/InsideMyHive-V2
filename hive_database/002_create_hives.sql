CREATE TABLE hives (
	node_id integer PRIMARY KEY,
	hive_name VARCHAR(100),
	notes VARCHAR(300),
	location point,
	location_desc VARCHAR(100)
);

ALTER TABLE readings
ADD FOREIGN KEY (node_id) 
REFERENCES hives (node_id);
