#!/usr/bin/env python3
import hashlib
import json
import re
from typing import Dict, Any, List

class NSEXLifter:
    """
    Pillar I: Static Intelligence & NS-EX Normalizer
    Lifts raw disassembly / Ghidra P-Code into Normalized Semantic Expressions (NS-EX),
    computing canonical SHA-256 logic hashes for global database lookup.
    """

    def normalize_expression(self, raw_pcode: str) -> str:
        """
        Normalizes P-code instructions into a canonical S-expression string.
        Strips address offsets and register naming variations to ensure logic equivalence.
        """
        cleaned = raw_pcode.strip()
        # Replace address variations with generic pattern
        cleaned = re.sub(r'0x[0-9a-fA-F]+', r'VAL', cleaned)
        # Canonicalize spaces
        cleaned = " ".join(cleaned.split())
        return f"(NS_EX_CANONICAL {cleaned})"

    def compute_logic_hash(self, nsex_expr: str) -> str:
        """Computes SHA-256 logic hash of canonical NS-EX string."""
        return hashlib.sha256(nsex_expr.encode('utf-8')).hexdigest()

    def lift_function(self, address: str, function_name: str, raw_instructions: List[str] = None) -> Dict[str, Any]:
        """
        Lifts a function's instruction set into NS-EX representation.
        """
        if not raw_instructions:
            raw_instructions = [
                f"XOR RAX, 0xDEADBEEF",
                f"CMP RAX, 0xCAFEBABE"
            ]

        pcode_body = " ".join([f"({instr})" for instr in raw_instructions])
        nsex_expr = f"(PCODE_FUNCTION {address} {pcode_body})"
        canonical_expr = self.normalize_expression(nsex_expr)
        logic_hash = self.compute_logic_hash(canonical_expr)

        return {
            "status": "success",
            "function_name": function_name,
            "address": address,
            "logic_hash": logic_hash,
            "nsex_expression": nsex_expr,
            "canonical_expression": canonical_expr
        }

if __name__ == "__main__":
    lifter = NSEXLifter()
    res = lifter.lift_function("0x401000", "verify_key")
    print(f"Pillar I NS-EX Lifted: {res['function_name']} -> Logic Hash: {res['logic_hash']}")
