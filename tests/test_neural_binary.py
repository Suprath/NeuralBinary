import unittest
import json
import sqlite3
import subprocess
from pathlib import Path

ROOT_DIR = Path(__file__).parent.parent
DB_PATH = ROOT_DIR / "database" / "neural_binary.db"
Z_CORE_BIN = ROOT_DIR / "pillar_3_symbolic" / "build" / "z_core"
MODERNIZED_DIR = ROOT_DIR / "modernized"

class TestNeuralBinaryPipeline(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        if not Z_CORE_BIN.exists():
            subprocess.run(["cmake", "-B", "build"], cwd=ROOT_DIR / "pillar_3_symbolic", check=True)
            subprocess.run(["make"], cwd=ROOT_DIR / "pillar_3_symbolic" / "build", check=True)

    def test_01_z_core_symbolic_solver(self):
        """Test native C++ Z-Core solver execution and SAT proof output."""
        result = subprocess.run([str(Z_CORE_BIN)], capture_output=True, text=True, check=True)
        output = result.stdout
        self.assertIn("Discovered 2 feasible execution paths", output)
        self.assertIn("Path 1 Feasibility: SAT", output)
        self.assertIn("rax = 0x14530451", output)

    def test_02_dynamic_oracle_runner(self):
        """Test Pillar II Dynamic Oracle Qiling / Mock OS runner."""
        from pillar_2_dynamic.mock_os_runner import MockOSRunner
        runner = MockOSRunner(db_path=DB_PATH)
        res = runner.run_trace(address="0x401000", input_data="0x14530451")
        self.assertEqual(res["status"], "success")
        self.assertEqual(res["cycle_count"], 4)
        self.assertIsNotNone(res["trace_id"])

    def test_03_mcp_server_tools(self):
        """Test MCP server tool functions without third-party cloud API dependencies."""
        from mcp_server.server import analyze_function_static, get_execution_trace, solve_constraints, commit_modernized_code
        
        # 1. Static Analysis
        res1 = analyze_function_static("0x401000", "verify_key")
        self.assertEqual(res1["status"], "success")
        logic_hash = res1["logic_hash"]

        # 2. Dynamic Trace
        res2 = get_execution_trace("0x401000", "0x14530451")
        self.assertEqual(res2["status"], "success")
        self.assertEqual(res2["cycle_count"], 4)

        # 3. Constraint Solving (Z-Core Native C++)
        res3 = solve_constraints("0x401000", "0x401500")
        self.assertEqual(res3["status"], "success")
        self.assertIn("rax = 0x14530451", res3["z_core_proof"])

        # 4. Commit Modernized Code File
        modern_code = """// Modernized C++ Implementation of Key Verification
#include <cstdint>

bool verify_key(uint64_t key) {
    return (key ^ 0xDEADBEEF) == 0xCAFEBABE;
}
"""
        res4 = commit_modernized_code(logic_hash, modern_code, "cpp", "modernized_verify_key.cpp")
        self.assertEqual(res4["status"], "success")
        self.assertEqual(res4["verification_status"], "VERIFIED_100_PERCENT_PARITY")

        # 5. Verify file written on disk
        target_file = MODERNIZED_DIR / "modernized_verify_key.cpp"
        self.assertTrue(target_file.exists())
        with open(target_file, "r") as f:
            content = f.read()
        self.assertIn("bool verify_key(uint64_t key)", content)

    def test_04_synthesis_context(self):
        """Test synthesis context packaging for AI Agent MCP queries."""
        from pillar_4_synthesis.synthesis_engine import assemble_synthesis_context
        from mcp_server.server import analyze_function_static
        
        res1 = analyze_function_static("0x401000", "verify_key")
        logic_hash = res1["logic_hash"]

        ctx = assemble_synthesis_context(logic_hash)
        self.assertEqual(ctx["logic_hash"], logic_hash)
        self.assertEqual(ctx["function_name"], "verify_key")

if __name__ == "__main__":
    unittest.main()
