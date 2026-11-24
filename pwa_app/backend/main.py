from contextlib import asynccontextmanager
from fastapi import FastAPI, Request
from fastapi import APIRouter, Depends
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel

from psycopg_pool import ConnectionPool
from psycopg.rows import dict_row

from datetime import timedelta

DSN = "postgresql://web_api:web_api@localhost:5432/hive"

NODES_SQL = """
SELECT
    h.node_id,
    h.hive_name,
    h.notes,
    h.location_desc,
    h.location[0]                               AS lon,
    h.location[1]                               AS lat,
    extract(epoch FROM h.expected_interval)::float8 AS expected_interval_s,
    h.gateway_id,
    h.test_node,
    g.gateway_name,
    s.last_reading,
    s.reading_age_s::float8                     AS reading_age_s,
    s.node_stale,
    cq.command_id,
    cq.command,
    cq.value,
    cq.sent,
    cq.queued,
    cq.ts                                         AS command_ts,
    rdgs.backlog as last_backlog
FROM api.nodes h
LEFT JOIN api.node_status s
    ON s.node_id = h.node_id
LEFT JOIN api.gateways g
    ON g.gateway_id = h.gateway_id
LEFT JOIN LATERAL (
    SELECT
        q.command_id,
        q.command,
        q.value,
        q.sent,
        q.queued,
        q.ts
    FROM api.commands_queue q
    WHERE q.node_id = h.node_id
    ORDER BY q.command_id DESC
    LIMIT 1
) cq ON true
LEFT join lateral (
  select
    r.backlog
  from api.readings r 
  where r.node_id = h.node_id
  order by r.ts desc
  limit 1
) rdgs on true
ORDER BY h.node_id;
"""

GATEWAYS_SQL = """
SELECT
g.gateway_id,
g.gateway_name,
s.heartbeat_age_s,
s.gateway_stale
FROM api.gateways g
LEFT JOIN api.gateway_status s
on s.gateway_id = g.gateway_id;
"""

@asynccontextmanager
async def lifespan(app: FastAPI):
    app.state.pool = ConnectionPool(DSN, open=False, kwargs={"row_factory":dict_row})
    app.state.pool.open(wait=True)
    yield
    app.state.pool.close()

def get_conn(request: Request):
    with request.app.state.pool.connection() as conn:
        yield conn

app = FastAPI(lifespan=lifespan)
router = APIRouter(prefix="/api")

@router.get("/nodes")
def list_nodes(conn=Depends(get_conn)):
    with conn.cursor() as cur:
        cur.execute(NODES_SQL)
        return cur.fetchall()

@router.get("/gateways")
def list_gateways(conn=Depends(get_conn)):
    with conn.cursor() as cur:
        cur.execute(GATEWAYS_SQL)
        return cur.fetchall()

class Command(BaseModel):
    node_id: int
    command: str
    value: int

@router.post("/commands")
def set_sleep_time(command: Command, conn=Depends(get_conn)):
    if command.command == "interval":
        with conn.cursor() as cur:
            delta = timedelta(seconds=command.value)
            cur.execute("UPDATE api.nodes SET expected_interval=%s WHERE node_id=%s", (delta, command.node_id,))
            cur.execute("INSERT INTO api.commands_queue (node_id, command, value) values (%s, %s, %s)", (command.node_id, command.command, command.value,))
    else:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Command not found"
        )

app.include_router(router)

app.mount("/", StaticFiles(directory="../frontend", html=True))
