#ifndef Z_CORE_HPP
#define Z_CORE_HPP

#include <z3++.h>
#include <string>
#include <unordered_map>
#include <array>
#include <optional>
#include <memory>
#include <vector>
#include <iostream>

namespace ZCore {

// Fast Enum for O(1) Register Array Indexing (Zero String Allocation)
enum class Reg : uint8_t {
    RAX = 0, RBX = 1, RCX = 2, RDX = 3,
    RSI = 4, RDI = 5, RSP = 6, RBP = 7,
    R8  = 8, R9  = 9, R10 = 10, R11 = 11,
    R12 = 12, R13 = 13, R14 = 14, R15 = 15,
    COND_FLAG = 16, TARGET_VAL = 17, CONST_VAL = 18,
    MAX_REGS = 32
};

inline const char* reg_to_string(Reg r) {
    switch (r) {
        case Reg::RAX: return "rax";
        case Reg::RBX: return "rbx";
        case Reg::RCX: return "rcx";
        case Reg::RDX: return "rdx";
        case Reg::RSI: return "rsi";
        case Reg::RDI: return "rdi";
        case Reg::RSP: return "rsp";
        case Reg::RBP: return "rbp";
        case Reg::COND_FLAG: return "cond_flag";
        case Reg::TARGET_VAL: return "target_val";
        case Reg::CONST_VAL: return "const_deadbeef";
        default: return "custom_reg";
    }
}

// ============================================================================
// Module B: Claripy (AST Engine with Bit-Vector Interning)
// ============================================================================
class ClaripyEngine {
public:
    ClaripyEngine(z3::context& ctx) : ctx_(ctx) {}

    z3::expr bv_const(Reg reg, unsigned bits = 64) {
        uint8_t idx = static_cast<uint8_t>(reg);
        if (symbol_cache_[idx].has_value()) {
            return symbol_cache_[idx].value();
        }
        z3::expr sym = ctx_.bv_const(reg_to_string(reg), bits);
        symbol_cache_[idx] = sym;
        return sym;
    }

    z3::expr bv_const(const std::string& name, unsigned bits = 64) {
        auto it = custom_cache_.find(name);
        if (it != custom_cache_.end()) return it->second;
        z3::expr sym = ctx_.bv_const(name.c_str(), bits);
        custom_cache_.insert({name, sym});
        return sym;
    }

    z3::expr bv_val(uint64_t val, unsigned bits = 64) {
        return ctx_.bv_val(val, bits);
    }

    z3::context& ctx() { return ctx_; }

private:
    z3::context& ctx_;
    std::array<std::optional<z3::expr>, static_cast<size_t>(Reg::MAX_REGS)> symbol_cache_;
    std::unordered_map<std::string, z3::expr> custom_cache_;
};

// ============================================================================
// Module A: SimState (O(1) Indexed Register Snapshot Manager)
// ============================================================================
class SimState {
public:
    SimState(ClaripyEngine& claripy) : claripy_(claripy), solver_(claripy.ctx()) {}

    // Copy-on-Write Copy Constructor
    SimState(const SimState& other) 
        : claripy_(other.claripy_),
          registers_(other.registers_),
          solver_(other.claripy_.ctx()) {
        for (const auto& expr : other.constraints_) {
            constraints_.push_back(expr);
            solver_.add(expr);
        }
    }

    void set_register(Reg reg, const z3::expr& val) {
        registers_[static_cast<size_t>(reg)] = val;
    }

    z3::expr get_register(Reg reg, unsigned default_bits = 64) {
        size_t idx = static_cast<size_t>(reg);
        if (registers_[idx].has_value()) {
            return registers_[idx].value();
        }
        z3::expr sym = claripy_.bv_const(reg, default_bits);
        registers_[idx] = sym;
        return sym;
    }

    void add_constraint(const z3::expr& constraint) {
        constraints_.push_back(constraint);
        solver_.add(constraint);
    }

    bool is_satisfiable() {
        return solver_.check() == z3::sat;
    }

    std::unordered_map<std::string, uint64_t> eval_solution() {
        std::unordered_map<std::string, uint64_t> solution;
        if (solver_.check() == z3::sat) {
            z3::model m = solver_.get_model();
            for (size_t i = 0; i < static_cast<size_t>(Reg::MAX_REGS); ++i) {
                if (registers_[i].has_value()) {
                    z3::expr expr = registers_[i].value();
                    if (expr.is_bv()) {
                        z3::expr eval_res = m.eval(expr, true);
                        if (eval_res.is_numeral()) {
                            solution[reg_to_string(static_cast<Reg>(i))] = eval_res.get_numeral_uint64();
                        }
                    }
                }
            }
        }
        return solution;
    }

    z3::solver& solver() { return solver_; }

private:
    ClaripyEngine& claripy_;
    std::array<std::optional<z3::expr>, static_cast<size_t>(Reg::MAX_REGS)> registers_;
    std::vector<z3::expr> constraints_;
    z3::solver solver_;
};

// ============================================================================
// Module D: CLE (Loader Lite)
// ============================================================================
class CLELoader {
public:
    struct Section {
        std::string name;
        uint64_t vaddr;
        uint64_t size;
    };

    void map_section(const std::string& name, uint64_t vaddr, uint64_t size) {
        sections_.push_back({name, vaddr, size});
    }

    bool is_mapped(uint64_t vaddr) const {
        for (const auto& sec : sections_) {
            if (vaddr >= sec.vaddr && vaddr < sec.vaddr + sec.size) return true;
        }
        return false;
    }

private:
    std::vector<Section> sections_;
};

// ============================================================================
// Module C: SimEngine (Fast O(1) Instruction Dispatcher)
// ============================================================================
enum class OpType { ADD, SUB, XOR, CMP_EQ, CMP_GT, BRANCH_IF };

struct PCodeInstruction {
    OpType op;
    Reg dest;
    Reg src1;
    Reg src2;
    uint64_t target_addr;
};

class SimEngine {
public:
    SimEngine(ClaripyEngine& claripy) : claripy_(claripy) {}

    std::vector<std::shared_ptr<SimState>> step(std::shared_ptr<SimState> state, const PCodeInstruction& instr) {
        std::vector<std::shared_ptr<SimState>> next_states;

        switch (instr.op) {
            case OpType::ADD: {
                z3::expr v1 = state->get_register(instr.src1);
                z3::expr v2 = state->get_register(instr.src2);
                state->set_register(instr.dest, v1 + v2);
                next_states.push_back(state);
                break;
            }
            case OpType::SUB: {
                z3::expr v1 = state->get_register(instr.src1);
                z3::expr v2 = state->get_register(instr.src2);
                state->set_register(instr.dest, v1 - v2);
                next_states.push_back(state);
                break;
            }
            case OpType::XOR: {
                z3::expr v1 = state->get_register(instr.src1);
                z3::expr v2 = state->get_register(instr.src2);
                state->set_register(instr.dest, v1 ^ v2);
                next_states.push_back(state);
                break;
            }
            case OpType::CMP_EQ: {
                z3::expr v1 = state->get_register(instr.src1);
                z3::expr v2 = state->get_register(instr.src2);
                z3::expr cond = (v1 == v2);
                state->set_register(instr.dest, z3::ite(cond, claripy_.bv_val(1, 64), claripy_.bv_val(0, 64)));
                next_states.push_back(state);
                break;
            }
            case OpType::BRANCH_IF: {
                z3::expr cond_var = state->get_register(instr.src1);
                z3::expr is_true = (cond_var != claripy_.bv_val(0, 64));

                // Path 1: Branch taken (CoW Clone state)
                auto true_state = std::make_shared<SimState>(*state);
                true_state->add_constraint(is_true);
                if (true_state->is_satisfiable()) {
                    next_states.push_back(true_state);
                }

                // Path 2: Branch not taken
                auto false_state = std::make_shared<SimState>(*state);
                false_state->add_constraint(!is_true);
                if (false_state->is_satisfiable()) {
                    next_states.push_back(false_state);
                }
                break;
            }
            default:
                next_states.push_back(state);
                break;
        }

        return next_states;
    }

private:
    ClaripyEngine& claripy_;
};

} // namespace ZCore

#endif // Z_CORE_HPP
