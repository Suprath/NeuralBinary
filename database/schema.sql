-- NeuralBinary Global Schema
-- Database: PostgreSQL (with optional TimescaleDB extension) or SQLite fallback

CREATE TABLE IF NOT EXISTS binary_mappings (
    logic_hash VARCHAR(64) PRIMARY KEY, -- SHA-256 of the NS-EX Logic
    function_name TEXT NOT NULL,
    start_address VARCHAR(32),
    semantic_intent TEXT,              -- AI-generated description
    symbolic_constraints TEXT,         -- Output from Z-Engine (C++ angr-lite)
    execution_trace_id VARCHAR(64),    -- Link to the Mock OS trace
    modernized_code TEXT,             -- Final ported code (C++/Java/Rust)
    verification_status BOOLEAN DEFAULT FALSE, -- Result of Differential Fuzzer
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS execution_traces (
    trace_id VARCHAR(64) NOT NULL,
    cycle_count BIGINT NOT NULL,
    instruction_pointer BIGINT NOT NULL,
    disassembly TEXT,
    register_state TEXT NOT NULL,      -- JSON Snapshot of RAX, RBX, etc.
    memory_delta TEXT NOT NULL,        -- JSON Delta of modified RAM
    PRIMARY KEY (trace_id, cycle_count)
);

CREATE INDEX IF NOT EXISTS idx_traces_id ON execution_traces(trace_id);
