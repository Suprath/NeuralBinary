#include "z_core.hpp"
#include <iostream>

int main(int argc, char** argv) {
    std::cout << "=== NeuralBinary Z-Core Native Engine (angr-lite in C++) ===" << std::endl;

    z3::context ctx;
    ZCore::ClaripyEngine claripy(ctx);
    ZCore::SimEngine engine(claripy);
    ZCore::CLELoader loader;

    loader.map_section(".text", 0x401000, 0x1000);

    auto initial_state = std::make_shared<ZCore::SimState>(claripy);

    // Setup input symbolic register RAX (e.g. key or user input)
    z3::expr input_key = claripy.bv_const("input_key", 64);
    initial_state->set_register("rax", input_key);

    // Instruction 1: RBX = RAX ^ 0xDEADBEEF
    ZCore::PCodeInstruction instr1{ZCore::OpType::XOR, "rbx", "rax", "const_deadbeef", 0};
    initial_state->set_register("const_deadbeef", claripy.bv_val(0xDEADBEEFULL, 64));
    auto states_after_instr1 = engine.step(initial_state, instr1);

    if (states_after_instr1.empty()) {
        std::cerr << "State unsat after instr1" << std::endl;
        return 1;
    }
    auto state = states_after_instr1[0];

    // Instruction 2: CMP_EQ rbx, 0xCAFEBABE
    initial_state->set_register("target_val", claripy.bv_val(0xCAFEBABEULL, 64));
    ZCore::PCodeInstruction instr2{ZCore::OpType::CMP_EQ, "cond_flag", "rbx", "target_val", 0};
    auto states_after_instr2 = engine.step(state, instr2);
    state = states_after_instr2[0];

    // Instruction 3: BRANCH_IF cond_flag -> 0x401500 (Secret Bypass target)
    ZCore::PCodeInstruction branch_instr{ZCore::OpType::BRANCH_IF, "", "cond_flag", "", 0x401500};
    auto branched_states = engine.step(state, branch_instr);

    std::cout << "Discovered " << branched_states.size() << " feasible execution paths." << std::endl;

    for (size_t i = 0; i < branched_states.size(); ++i) {
        std::cout << "\nPath " << i + 1 << " Feasibility: " << (branched_states[i]->is_satisfiable() ? "SAT" : "UNSAT") << std::endl;
        auto solution = branched_states[i]->eval_solution();
        for (const auto& [reg, val] : solution) {
            std::cout << "  " << reg << " = 0x" << std::hex << val << std::dec << std::endl;
        }
    }

    return 0;
}
