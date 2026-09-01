#!/usr/bin/env python3
import sys
import json
import time
import ctypes
import tempfile
import subprocess
from pathlib import Path

ROOT_DIR = Path(__file__).parent.parent
sys.path.insert(0, str(ROOT_DIR))

MODERNIZED_ADLER_CPP = ROOT_DIR / "modernized" / "modernized_official_adler32.cpp"
MODERNIZED_CRC_CPP = ROOT_DIR / "modernized" / "modernized_zlib_crc32.cpp"

def profile_modernized_zlib_modules():
    print("==================================================================")
    print("  CPU PROFILING TELEMETRY: MODERNIZED C++ zlib MODULES")
    print("==================================================================")

    from pillar_2_dynamic.mock_os_runner import ExecutionProfiler
    profiler = ExecutionProfiler()

    with tempfile.TemporaryDirectory() as tmpdir:
        adler_so = Path(tmpdir) / "libmod_adler.dylib"
        cmd = ["clang++", "-std=c++20", "-shared", "-fPIC", "-O3", str(MODERNIZED_ADLER_CPP), "-o", str(adler_so)]
        subprocess.run(cmd, capture_output=True, text=True, check=True)

        mod_adler_lib = ctypes.CDLL(str(adler_so))
        mod_adler_lib.adler32_modernized.argtypes = [ctypes.c_uint32, ctypes.c_char_p, ctypes.c_size_t]
        mod_adler_lib.adler32_modernized.restype = ctypes.c_uint32

        # Generate profiling payload
        payload_size = 55520 # 10 chunks of NMAX
        test_payload = b"A" * payload_size

        t0 = time.perf_counter()
        for i in range(100):
            res = mod_adler_lib.adler32_modernized(1, test_payload, payload_size)
            profiler.record_step(0x147c, "adler32_modernized_loop", {"bytes_processed": str(payload_size)})
        t1 = time.perf_counter()

        report = profiler.generate_report()
        print(f"\n[Profiler Report for modernized_official_adler32.cpp]")
        print(f" -> Total Profiling Iterations: 100")
        print(f" -> Payload Per Run: {payload_size} bytes")
        print(f" -> Execution Time: {(t1 - t0)*1000:.2f} ms")
        print(f" -> Top Hotspot: {report['top_hotspot_instructions'][0]['address']} ({report['top_hotspot_instructions'][0]['disassembly']}) - Hits: {report['top_hotspot_instructions'][0]['hit_count']}")
        print(f" -> Recommendation: {report['optimization_suggestions']}")

if __name__ == "__main__":
    profile_modernized_zlib_modules()
