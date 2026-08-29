# NeuralBinary Global

**NeuralBinary Global** is a unified, AI-augmented reverse engineering, decompilation, and software modernization platform. Designed to operate strictly within a **16GB RAM constraint**, it decouples computational workloads into four autonomous pillars linked through a Central Knowledge Database (PostgreSQL / SQLite) and orchestrated by a Model Context Protocol (MCP) Server.

---

## Architectural Overview

```
                          ┌──────────────────────────┐
                          │  AI Agent (Gemini / MCP) │
                          └────────────┬─────────────┘
                                       │ MCP Protocol (stdio)
                          ┌────────────▼─────────────┐
                          │      MCP Central Server  │
                          │   (mcp_server/server.py) │
                          └─────┬──────┬──────┬──────┘
                                │      │      │
          ┌─────────────────────┘      │      └─────────────────────┐
          │                            │                            │
┌─────────▼───────────┐      ┌─────────▼───────────┐      ┌─────────▼───────────┐
│ Pillar I: Static    │      │ Pillar II: Dynamic  │      │ Pillar III: Z-Engine│
│ Ghidra / Sleigh IR  │      │ Qiling / Mock OS    │      │ C++ / Z3 Solver     │
│ Structural Mapping  │      │ Cycle Traces & RAM  │      │ Memory State Stash  │
└─────────┬───────────┘      └─────────┬───────────┘      └─────────┬───────────┘
          │                            │                            │
          └─────────────────────┐      │      ┌─────────────────────┘
                                │      │      │
                          ┌─────▼──────▼──────▼──────┐
                          │    Central Knowledge DB  │
                          │ PostgreSQL / TimescaleDB │
                          └──────────────────────────┘
```

---

## The Four Autonomous Pillars

1. **Pillar I: Static Intelligence (Ghidra / Sleigh)**
   - Extracts structural topology (Functions, Call Graphs, CFGs, Basic Blocks).
   - Lifts binary machine code into **NS-EX (Normalized Semantic Expressions)**.
   - Hashes logic blocks using SHA-256 (`logic_hash`) to prevent duplicate analysis.

2. **Pillar II: Dynamic Oracle (Qiling / Unicorn Engine)**
   - Runs target functions inside a virtual sandboxed OS environment.
   - Captures per-cycle instruction register states and memory write deltas.
   - Resolves pointer aliasing, dynamic memory structures, and indirect jumps.

3. **Pillar III: Symbolic Solver (Native C++ Z-Engine)**
   - Native C++ port of core `angr` modules (`SimState`, `Claripy`, `SimEngine`, `CLELoader`) integrated with `z3++`.
   - Solves path feasibility and extracts mathematical input constraints required to reach target branches.
   - Employs Copy-on-Write (CoW) state management and Bit-Vector interning to stay under 16GB RAM.

4. **Pillar IV: Synthesis & Verification Bridge (MCP + AI)**
   - Exposes tools via MCP over stdio.
   - Combines Static NS-EX + Dynamic Traces + Z-Core Proofs into structured JSON context.
   - Synthesizes modern C++/Java source code and runs **Differential Fuzzing (`differential_verifier.py`)** to verify 100% behavioral parity.

---

## Database & Docker Setup

NeuralBinary supports containerized PostgreSQL with TimescaleDB via Docker Compose, as well as a local zero-config SQLite fallback:

```bash
# Start Enterprise PostgreSQL + TimescaleDB Database Container
docker-compose up -d
```

---

## Quickstart Guide

### 1. Prerequisites
- macOS / Linux
- `docker` & `docker-compose` (optional, for PostgreSQL)
- `cmake` (>= 3.15)
- `clang++` or `g++` (C++17 support)
- `z3` C++ library (`brew install z3` on macOS)
- `python3` (>= 3.9)

### 2. Build Native Z-Engine (C++)
```bash
cd pillar_3_symbolic
mkdir -p build && cd build
cmake ..
make
./z_core
```

### 3. Run Integration Tests
```bash
python3 -m unittest tests/test_neural_binary.py
```

### 4. Start MCP Server
```bash
python3 mcp_server/server.py --serve
```

---

## Project Layout

- `database/`: SQL schemas (`schema.sql`) and database client (`db_client.py`).
- `docker-compose.yml`: Docker configuration for PostgreSQL + TimescaleDB.
- `mcp_server/`: FastMCP / JSON-RPC tool server (`server.py`).
- `pillar_1_static/`: Ghidra headless scripts and NS-EX lifter routines (`nsex_lifter.py`).
- `pillar_2_dynamic/`: Qiling / Mock OS runner (`mock_os_runner.py`).
- `pillar_3_symbolic/`: Native C++ Z-Engine (`z_core.hpp`, `main.cpp`, `CMakeLists.txt`).
- `pillar_4_synthesis/`: Context packager (`synthesis_engine.py`) and differential verifier (`differential_verifier.py`).
- `modernized/`: Output directory for generated C++/Java modernized source files.
- `tests/`: Automated unit and integration test suite (`test_neural_binary.py`).
