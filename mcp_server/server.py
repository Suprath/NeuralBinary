#!/usr/bin/env python3
import sys
import json
import sqlite3
import subprocess
import hashlib
from pathlib import Path

# Paths & Module Import Setup
ROOT_DIR = Path(__file__).parent.parent
sys.path.insert(0, str(ROOT_DIR))

DB_PATH = ROOT_DIR / "database" / "neural_binary.db"
Z_CORE_BIN = ROOT_DIR / "pillar_3_symbolic" / "build" / "z_core"
MODERNIZED_DIR = ROOT_DIR / "modernized"

from database.db_client import DatabaseClient
db_client = DatabaseClient()
db_client.init_schema()

# ============================================================================
# Tool Implementation Functions
# ============================================================================

def analyze_function_static(address: str = "0x401000", function_name: str = "func_target", binary_path: str = None) -> dict:
    """Extracts NS-EX (Normalized Semantic Expressions) for a function at a specific address."""
    from pillar_1_static.ghidra_headless_runner import GhidraHeadlessRunner
    runner = GhidraHeadlessRunner(db_path=DB_PATH)
    return runner.analyze_binary(binary_path=binary_path, address=address, function_name=function_name)

def solve_constraints(start_address: str, target_address: str) -> dict:
    """Triggers native C++ Z-Engine (angr-lite) to solve branch input constraints."""
    if not Z_CORE_BIN.exists():
        return {"status": "error", "error": f"Z-Core executable not found at {Z_CORE_BIN}"}

    try:
        proc = subprocess.run([str(Z_CORE_BIN)], capture_output=True, text=True, check=True)
        solver_output = proc.stdout
    except Exception as e:
        return {"status": "error", "error": f"Failed to execute Z-Core binary: {str(e)}"}

    return {
        "status": "success",
        "start_address": start_address,
        "target_address": target_address,
        "z_core_proof": solver_output.strip()
    }

def get_execution_trace(address: str = "0x401000", input_data: str = "0x14530451", binary_path: str = None) -> dict:
    """Runs Dynamic Oracle (Qiling/Unicorn) on a function and returns instruction state transition log."""
    from pillar_2_dynamic.mock_os_runner import MockOSRunner
    runner = MockOSRunner(db_path=DB_PATH)
    return runner.run_trace(binary_path=binary_path, address=address, input_data=input_data, profile_execution=False)

def analyze_performance_bottlenecks(address: str = "0x401000", input_data: str = "0x14530451", binary_path: str = None) -> dict:
    """Runs CPU Profiler telemetry to identify tight loops, hotspot instructions, and memory write bottlenecks."""
    from pillar_2_dynamic.mock_os_runner import MockOSRunner
    runner = MockOSRunner(db_path=DB_PATH)
    trace_res = runner.run_trace(binary_path=binary_path, address=address, input_data=input_data, profile_execution=True)
    return {
        "status": "success",
        "address": address,
        "bottleneck_profile": trace_res.get("bottleneck_profile", {})
    }

def commit_modernized_code(logic_hash: str, source_code: str, language: str = "cpp", filename: str = "modernized_func.cpp") -> dict:
    """Saves final ported code to DB, writes file, and runs dynamic differential verification."""
    from pillar_4_synthesis.differential_verifier import DifferentialVerifier
    
    file_path = MODERNIZED_DIR / filename
    with open(file_path, "w") as f:
        f.write(source_code.strip() + "\n")

    # Run Differential Fuzzer Verification
    verifier = DifferentialVerifier()
    v_res = verifier.verify_code(source_code)
    is_verified = (v_res.get("verification_status") == "VERIFIED_100_PERCENT_PARITY")

    conn, engine = db_client.get_connection()
    cursor = conn.cursor()
    cursor.execute("""
        UPDATE binary_mappings
        SET modernized_code = ?, verification_status = ?
        WHERE logic_hash = ?
    """, (source_code, 1 if is_verified else 0, logic_hash))
    conn.commit()
    conn.close()

    return {
        "status": "success",
        "logic_hash": logic_hash,
        "file_written": str(file_path),
        "language": language,
        "verification_status": v_res.get("verification_status", "UNVERIFIED")
    }

# ============================================================================
# Stdio / JSON-RPC Handler for MCP Protocols
# ============================================================================

TOOLS = {
    "analyze_function_static": analyze_function_static,
    "solve_constraints": solve_constraints,
    "get_execution_trace": get_execution_trace,
    "analyze_performance_bottlenecks": analyze_performance_bottlenecks,
    "commit_modernized_code": commit_modernized_code
}

def handle_json_rpc(request: dict) -> dict:
    method = request.get("method")
    params = request.get("params", {})
    req_id = request.get("id")

    if method in TOOLS:
        try:
            result = TOOLS[method](**params)
            return {"jsonrpc": "2.0", "result": result, "id": req_id}
        except Exception as e:
            return {"jsonrpc": "2.0", "error": {"code": -32603, "message": str(e)}, "id": req_id}
    else:
        return {"jsonrpc": "2.0", "error": {"code": -32601, "message": f"Method {method} not found"}, "id": req_id}

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--serve":
        for line in sys.stdin:
            line = line.strip()
            if not line:
                continue
            try:
                req = json.loads(line)
                resp = handle_json_rpc(req)
                print(json.dumps(resp), flush=True)
            except Exception as err:
                print(json.dumps({"jsonrpc": "2.0", "error": {"code": -32700, "message": str(err)}}), flush=True)
    else:
        print("NeuralBinary Global MCP Server initialized successfully.")
        print(f"Available tools: {list(TOOLS.keys())}")
