# NeuralBinary Global — Architecture & Codebase Design

This document details every component, file, and subsystem in **NeuralBinary Global**, explaining **what it does** and **why it was designed that way**.

---

## 1. Database Schema (`database/schema.sql`)

### What It Does
Defines the Central Knowledge Database schema (`binary_mappings` and `execution_traces`).

### Why It Was Designed This Way
- **`binary_mappings` Table**:
  - `logic_hash` (CHAR 64 PRIMARY KEY): SHA-256 hash of the canonical NS-EX S-expression. Functions with identical logic (e.g. CRC32, AES) produce the same hash. Once solved, it is cached permanently—preventing re-analysis.
  - `semantic_intent`: AI-generated high-level description of what the routine does.
  - `symbolic_constraints`: Output from Z-Engine (Z3 math proof).
  - `modernized_code`: Ported target source code (C++/Java/Rust).
  - `verification_status`: Boolean flag indicating differential verification parity.

- **`execution_traces` Table**:
  - `trace_id` (UUID/HASH): Link to dynamic emulation session.
  - `cycle_count`, `instruction_pointer`, `disassembly`: Time-series instruction snapshots.
  - `register_state`, `memory_delta`: JSON snapshots of modified registers and RAM bytes per cycle.

---

## 2. Pillar III: Symbolic Z-Engine (`pillar_3_symbolic/`)

### What It Does
A native C++ port of core `angr` modules (`SimState`, `Claripy`, `SimEngine`, `CLELoader`) integrated directly with the Z3 C++ API (`z3++`).

### Why It Was Designed This Way
Standard Python `angr` consumes 32GB–64GB+ of RAM due to Python object overhead and unconstrained state copying. `Z-Core` solves this through:

- **`include/z_core.hpp`**:
  - **`ClaripyEngine` (Module B)**: Implements Bit-Vector Interning (`symbol_cache_`). Reuses Z3 AST pointers (`z3::expr`) in C++ memory so identical expressions are allocated once.
  - **`SimState` (Module A)**: Implements Copy-on-Write state cloning (`SimState(const SimState& other)`). Register maps use C++ value semantics with Z3 handle reference counting.
  - **`SimEngine` (Module C)**: Fast dispatch loop executing IR statements on `SimState`. Handles `BRANCH_IF` state splitting into branch taken and branch not taken, checking feasibility via Z3 `solver_.check()`.
  - **`CLELoader` (Module D)**: Maps virtual address sections (`map_section`, `is_mapped`).

- **`src/main.cpp`**: Executable main driver. Demonstrates constraint path solving on symbolic register inputs (`rax ^ 0xDEADBEEF == 0xCAFEBABE`), returning SAT solutions (`rax = 0x14530451`).

- **`CMakeLists.txt`**: Build configuration linking against `/opt/homebrew/include` and `libz3`.

---

## 3. Central MCP Server (`mcp_server/server.py`)

### What It Does
Runs an Model Context Protocol (MCP) server over standard I/O (stdio) using JSON-RPC protocol.

### Why It Was Designed This Way
- **Zero Third-Party Cloud API Dependencies**: Contains zero hardcoded API keys.
- **Tools Exposed**:
  1. `analyze_function_static`: Lifts P-Code to NS-EX and saves `logic_hash` to DB.
  2. `solve_constraints`: Runs native C++ `z_core` binary to solve branch constraints.
  3. `get_execution_trace`: Emulates dynamic cycle states and records register/RAM deltas.
  4. `commit_modernized_code`: Writes ported source code to `modernized/` directory and flags verification status in DB.

---

## 4. Pillar IV: Synthesis Engine (`pillar_4_synthesis/synthesis_engine.py`)

### What It Does
Assembles synthesis context packages for the AI Agent and manages differential verification.

### Why It Was Designed This Way
- **`assemble_synthesis_context(logic_hash)`**: Queries `binary_mappings` and `execution_traces` to build a clean, compact JSON bundle containing:
  - Static NS-EX logic structure.
  - Z-Engine symbolic math proof.
  - Deduplicated Qiling cycle traces & RAM deltas.
- This gives the AI Agent **100% mathematical and behavioral truth** with minimal input tokens (~1,000 tokens per call).

---

## 5. Output Directory (`modernized/`)

### What It Does
Stores generated modernized C++/Java/Rust source code files (e.g. `modernized/modernized_verify_key.cpp`).

---

## 6. Test Suite (`tests/test_neural_binary.py`)

### What It Does
Automated unit and integration test suite checking:
1. Native C++ `z_core` solver execution & SAT output.
2. MCP server tool functionality.
3. Disk file writing & database updates.
4. Synthesis context packaging.
