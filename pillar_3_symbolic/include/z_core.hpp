#ifndef Z_CORE_HPP
#define Z_CORE_HPP

#include <z3++.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <iostream>

namespace ZCore {

// ============================================================================
// Module B: Claripy (AST Engine with Bit-Vector Interning)
// ============================================================================
class ClaripyEngine {
public:
    ClaripyEngine(z3::context& ctx) : ctx_(ctx) {}

    z3::expr bv_const(const std::string& name, unsigned bits) {
        auto key = name + ":" + std::to_string(bits);
        auto it = symbol_cache_.find(key);
        if (it != symbol_cache_.end()) {
            return it->second;
        }
        z3::expr sym = ctx_.bv_const(name.c_str(), bits);
        symbol_cache_.insert({key, sym});
        return sym;
    }

    z3::expr bv_val(uint64_t val, unsigned bits) {
        return ctx_.bv_val(val, bits);
    }

    z3::context& ctx() { return ctx_; }

private:
    z3::context& ctx_;
    std::unordered_map<std::string, z3::expr> symbol_cache_;
};

// ============================================================================
// Module A: SimState (Copy-on-Write Register Snapshot Manager)
// ============================================================================
class SimState {
public:
    SimState(ClaripyEngine& claripy) : claripy_(claripy), solver_(claripy.ctx()) {}

    // Copy constructor cloning registers and Z3 solver assertions
    SimState(const SimState& other) 
        : claripy_(other.claripy_),
          registers_(other.registers_),
          solver_(other.claripy_.ctx()) {
        for (const auto& expr : other.constraints_) {
            constraints_.push_back(expr);
            solver_.add(expr);
        }
    }

    void set_register(const std::string& reg_name, const z3::expr& val) {
        auto it = registers_.find(reg_name);
        if (it != registers_.end()) {
            it->second = val;
        } else {
            registers_.insert({reg_name, val});
        }
    }

    z3::expr get_register(const std::string& reg_name, unsigned default_bits = 64) {
        auto it = registers_.find(reg_name);
        if (it != registers_.end()) {
            return it->second;
        }
        z3::expr sym = claripy_.bv_const(reg_name, default_bits);
        registers_.insert({reg_name, sym});
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
            for (const auto& [reg, expr] : registers_) {
                if (expr.is_bv()) {
                    z3::expr eval_res = m.eval(expr, true);
                    if (eval_res.is_numeral()) {
                        solution[reg] = eval_res.get_numeral_uint64();
                    }
                }
            }
        }
        return solution;
    }

    z3::solver& solver() { return solver_; }

private:
    ClaripyEngine& claripy_;
    std::unordered_map<std::string, z3::expr> registers_;
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
// Module C: SimEngine (P-Code / Symbolic Execution Dispatcher)
// ============================================================================
enum class OpType { ADD, SUB, XOR, CMP_EQ, CMP_GT, BRANCH_IF };

struct PCodeInstruction {
    OpType op;
    std::string dest;
    std::string src1;
    std::string src2;
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
