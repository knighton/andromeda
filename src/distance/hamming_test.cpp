#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "distance/hamming.h"

using andromeda::distance::HammingDistance;
using andromeda::distance::BitsInCommon;

int main() {
    uint64_t num_iter = 100000;

    {
        uint32_t a = 0xDEADBEEF;
        uint32_t b = a;
        assert(HammingDistance(a, b) == 0);
        assert(BitsInCommon(a, b) == 32);
    }

    {
        uint32_t a = 0xAAAAAAAA;
        uint32_t b = 0x55555555;
        assert(HammingDistance(a, b) == 32);
        assert(BitsInCommon(a, b) == 0);
    }

    uint64_t hamming_sum = 0;
    uint64_t bic_sum = 0;
    for (uint32_t i = 0; i < num_iter; ++i) {
        int32_t a = rand();
        int32_t b = rand();

        uint32_t hamming = HammingDistance(a, b);
        assert(hamming < 32);
        hamming_sum += hamming;

        uint32_t bic = BitsInCommon(a, b);
        assert(bic < 32);
        bic_sum += bic;
    }
    double hamming_avg = (double)hamming_sum / num_iter;
    double bic_avg = (double)bic_sum / num_iter;
    printf("%lf\n", hamming_avg);
    printf("%lf\n", bic_avg);
    assert(15 < hamming_avg && hamming_avg < 17);
    assert(15 < bic_avg && bic_avg < 17);

    for (uint32_t i = 0; i < num_iter; ++i) {
        uint32_t a = (uint32_t)rand();
        uint32_t b = a ^ (1 << (rand() % 32));
        assert(HammingDistance(a, b) == 1);
        assert(BitsInCommon(a, b) == 31);
    }
}
