from contextlib import asynccontextmanager
from fastapi import FastAPI, Request
from fastapi import APIRouter, Depends

from psycopg_pool import ConnectionPool
from psycopg.rows import dict_row

DSN = "postgresql://grafana:grafana!@localhost:5432/hive"

NODES_SQL = """
    SELECT
        h.node_id,
        h.hive_name,
        h.notes,
        h.location_desc,
        h.location[0]                              AS lon,
        h.location[1]                              AS lat,
        extract(epoch FROM h.expected_interval)::float8 AS expected_interval_s,
        h.gateway_id,
        g.gateway_name,
        s.last_reading,
        s.reading_age_s::float8                    AS reading_age_s,
        s.node_stale
    FROM api.nodes h
    LEFT JOIN api.node_status s ON s.node_id = h.node_id
    LEFT JOIN gateways g        ON g.gateway_id = h.gateway_id
    ORDER BY h.node_id
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

app.include_router(router)

