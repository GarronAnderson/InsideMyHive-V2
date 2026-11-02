create role web_anon nologin;

grant usage on schema api to web_anon;
grant select on api.readings to web_anon;

create role authenticator noinherit login password 'inside_unit';
grant web_anon to authenticator;

create role inside_unit nologin;
grant inside_unit to authenticator;

grant usage on schema api to inside_unit;
grant all on api.readings to inside_unit;

