# NeuralBinary Global — Architecture & Codebase Design

This document details every component, file, and subsystem in **NeuralBinary Global**, explaining **what it does** and **why it was designed that way**.

---

## 1. Database Schema & Docker (`database/` & `docker-compose.yml`)

### What It Does
- `docker-compose.yml`: Launches PostgreSQL 15 with TimescaleDB extension in a Docker container (`docker-compose up -d`).
- `database/schema.sql`: Defines `binary_mappings` and `execution_traces`.
- `database/db_client.py`: Database client that connects to PostgreSQL when Docker is running, or falls back to SQLite (`neural_binary.db`). Optimized with `batch_insert_traces()` using `executemany` for high-throughput 100k-row commits.

---

## 2. Pillar I: Static Intelligence & NS-EX Lifter (`pillar_1_static/`)

### What It Does
Lifts binary instructions into canonical **Normalized Semantic Expressions (NS-EX)** and runs Ghidra Headless static analysis.

### Why It Was Designed This Way
- **`pillar_1_static/nsex_lifter.py`**:
  - Uses fast single-pass tokenization (replacing slow regex patterns) to normalize disassembly/P-code expressions into canonical S-expressions (`(PCODE_FUNCTION ...)`).
  - Computes the canonical SHA-256 `logic_hash` used as the master key across the entire platform.
- **`pillar_1_static/ghidra_headless_runner.py`**:
  - Automates Ghidra Headless analysis without launching the GUI.
  - Feeds static function mappings directly into `binary_mappings`.

---

## 3. Pillar II: Dynamic Oracle & CPU Profiler (`pillar_2_dynamic/mock_os_runner.py`)

### What It Does
Executes binary libraries inside a virtual sandboxed OS environment using **Qiling Framework** and **Unicorn Engine**, while capturing CPU execution profiling telemetry (`ExecutionProfiler`).

### Why It Was Designed This Way
- **Dynamic Oracle Tracing**: Intercepts CPU execution at every instruction cycle (`ql.hook_code`), extracting register states (`RAX`, `RBX`, `RSP`), disassemblies, and RAM write deltas into `execution_traces`.
- **CPU Bottleneck Analysis (`ExecutionProfiler`)**:
  - Counts instruction hit frequencies (`instruction_counts`) to pinpoint tight loops.
  - Tracks memory write hotspots (`memory_writes`).
  - Generates actionable performance optimization suggestions.

---

## 4. Pillar III: Symbolic Z-Engine (`pillar_3_symbolic/`)

### What It Does
A native C++ port of core `angr` modules (`SimState`, `Claripy`, `SimEngine`, `CLELoader`) integrated directly with the Z3 C++ API (`z3++`).

### Why It Was Designed This Way
Standard Python `angr` consumes 32GB–64GB+ of RAM due to Python object overhead and unconstrained state copying. `Z-Core` solves this through:

- **`include/z_core.hpp`**:
  - **$O(1)$ Register Indexing**: Uses `enum class Reg : uint8_t` and `std::array<std::optional<z3::expr>, 32>` instead of `std::string` hash maps. Lookups are direct memory offsets ($O(1)$, zero string allocation).
  - **`ClaripyEngine` (Module B)**: Implements Bit-Vector Interning (`symbol_cache_`). Reuses Z3 AST pointers (`z3::expr`) in C++ memory.
  - **`SimState` (Module A)**: Implements Copy-on-Write state cloning (`SimState(const SimState& other)`). Register maps use C++ value semantics with Z3 handle reference counting.
  - **`SimEngine` (Module C)**: Fast O(1) dispatch loop executing IR statements on `SimState`. Handles `BRANCH_IF` state splitting.
  - **`CLELoader` (Module D)**: Maps virtual address sections (`map_section`, `is_mapped`).

- **`src/main.cpp`**: Executable main driver. Demonstrates constraint path solving on symbolic register inputs (`rax ^ 0xDEADBEEF == 0xCAFEBABE`), returning SAT solutions (`rax = 0x14530451`).

---

## 5. Central MCP Server (`mcp_server/server.py`)

### What It Does
Runs an Model Context Protocol (MCP) server over standard I/O (stdio) using JSON-RPC protocol.

### Tools Exposed:
1. `analyze_function_static`: Invokes Pillar I Ghidra & NS-EX lifter, saving `logic_hash` to DB.
2. `solve_constraints`: Runs native C++ `z_core` binary to solve branch constraints.
3. `get_execution_trace`: Runs Qiling/Unicorn dynamic oracle and records register/RAM deltas.
4. `analyze_performance_bottlenecks`: Runs CPU Profiler telemetry to pinpoint hot loops and memory write bottlenecks.
5. `commit_modernized_code`: Writes ported source code to `modernized/` directory, runs differential fuzzing, and updates DB.

---

## 6. Pillar IV: Synthesis & Differential Verifier (`pillar_4_synthesis/`)

### What It Does
Assembles context packages and verifies 100% behavioral parity of generated code.

### Subsystems:
- **`synthesis_engine.py`**: Builds context packages (NS-EX + Z3 proofs + traces) for AI Agent queries.
- **`differential_verifier.py`**: Compiles generated C++ code on-the-fly into dynamic libraries (`.dylib`) using `clang++`, loads them via `ctypes`, and runs differential fuzzing against original Qiling traces to guarantee behavioral parity.

---

## 7. Test Suite (`tests/test_neural_binary.py`)

### What It Does
Automated integration test suite checking:
1. Native C++ `z_core` solver execution & SAT output.
2. Pillar II Qiling / Dynamic Oracle trace generation.
3. Pillar I NS-EX Lifter & Ghidra headless analysis.
4. Pillar IV Differential Fuzzing & dynamic compilation.
5. MCP server tool functionality.
6. Context packaging.
7. CPU Profiler bottleneck telemetry (`test_07_cpu_profiler_bottlenecks`).
