import os
import sqlite3
from pathlib import Path

ROOT_DIR = Path(__file__).parent.parent
SQLITE_DB_PATH = ROOT_DIR / "database" / "neural_binary.db"
SCHEMA_PATH = ROOT_DIR / "database" / "schema.sql"

class DatabaseClient:
    """
    Database Interface for NeuralBinary Global.
    Supports PostgreSQL (Docker container) and SQLite fallback for local development.
    """

    def __init__(self):
        self.pg_host = os.getenv("POSTGRES_HOST", "localhost")
        self.pg_port = os.getenv("POSTGRES_PORT", "5432")
        self.pg_db = os.getenv("POSTGRES_DB", "neural_binary")
        self.pg_user = os.getenv("POSTGRES_USER", "neural_user")
        self.pg_password = os.getenv("POSTGRES_PASSWORD", "neural_password")

    def get_connection(self):
        # Try PostgreSQL connection if psycopg2 or asyncpg is installed
        try:
            import psycopg2
            conn = psycopg2.connect(
                host=self.pg_host,
                port=self.pg_port,
                dbname=self.pg_db,
                user=self.pg_user,
                password=self.pg_password
            )
            return conn, "postgres"
        except Exception:
            # SQLite local fallback
            SQLITE_DB_PATH.parent.mkdir(parents=True, exist_ok=True)
            conn = sqlite3.connect(SQLITE_DB_PATH)
            return conn, "sqlite"

    def init_schema(self):
        conn, engine = self.get_connection()
        with open(SCHEMA_PATH, "r") as f:
            schema_sql = f.read()
        
        if engine == "sqlite":
            conn.executescript(schema_sql)
            conn.commit()
            conn.close()
        else:
            cursor = conn.cursor()
            cursor.execute(schema_sql)
            conn.commit()
            conn.close()

if __name__ == "__main__":
    db = DatabaseClient()
    db.init_schema()
    _, engine = db.get_connection()
    print(f"DatabaseClient initialized successfully (Engine: {engine}).")
