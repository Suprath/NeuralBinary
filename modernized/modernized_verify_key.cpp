// Modernized C++ Implementation of Key Verification
#include <cstdint>

bool verify_key(uint64_t key) {
    return (key ^ 0xDEADBEEF) == 0xCAFEBABE;
}
