#pragma once

namespace andromeda {
namespace distance {

template <typename int_t>
uint32_t HammingDistance(int_t a, int_t b) {
    uint32_t count = 0;
    int_t n = a ^ b;
    while (n) {
        ++count;
        n &= n - 1;
    }
    return count;
}

template <typename int_t>
uint32_t BitsInCommon(int_t a, int_t b) {
    uint32_t count = 0;
    int_t n = ~(a ^ b);
    while (n) {
        ++count;
        n &= n - 1;
    }
    return count;
}

}  // namespace distance
}  // namespace andromeda
