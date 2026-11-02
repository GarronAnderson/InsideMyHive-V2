create role grafana with password 'grafana!';
grant usage on schema api to grafana;
grant select on api.readings to grafana;
grant select on hives to grafana;
grant select on calibration to grafana;


