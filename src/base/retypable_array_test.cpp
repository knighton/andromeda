#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "base/retypable_array.h"

using andromeda::base::UnsignedRetypableArray;
using std::string;
using std::vector;

#define MILLION 1000000.0

namespace {

void TestU16Array(uint32_t num_slots, uint32_t num_iter) {
    printf("- array of uint16_t:\n");

    auto begin = std::chrono::high_resolution_clock::now();

    uint16_t* a = (uint16_t*)calloc(num_slots, sizeof(uint16_t));
    for (uint32_t i = 0; i < num_iter; ++i) {
        uint32_t index = (uint32_t)rand() % num_slots;
        if (a[index] == UINT16_MAX) {
            printf("      failed (counts too big)\n");
            printf("\n");
            free(a);
            return;
        }
        ++a[index];
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        end - begin).count();

    free(a);

    printf("      %.6lf ms at %zu byte(s)/elem aka %.3lf mb\n", ns / MILLION,
           sizeof(uint16_t), num_slots * sizeof(uint16_t) / MILLION);
    printf("\n");
}

void TestU32Array(uint32_t num_slots, uint32_t num_iter) {
    printf("- array of uint32_t:\n");

    auto begin = std::chrono::high_resolution_clock::now();

    uint32_t* a = (uint32_t*)calloc(num_slots, sizeof(uint32_t));
    for (uint32_t i = 0; i < num_iter; ++i) {
        ++a[(uint32_t)rand() % num_slots];
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        end - begin).count();

    free(a);

    printf("      %.6lf ms at %zu byte(s)/elem aka %.3lf mb\n", ns / MILLION,
           sizeof(uint32_t), num_slots * sizeof(uint32_t) / MILLION);
    printf("\n");
}

void TestRetypableArray(uint32_t num_slots, uint32_t num_iter) {
    printf("- RetypableArray:\n");

    auto begin = std::chrono::high_resolution_clock::now();

    UnsignedRetypableArray a;
    a.Init(num_slots, 1);
    for (uint32_t i = 0; i < num_iter; ++i) {
        a.Incr((uint32_t)rand() % num_slots, 1);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        end - begin).count();

    printf("      %.6lf ns at %zu byte(s)/elem aka %.3lf mb\n", ns / MILLION,
           a.int_size(), a.size() * a.int_size() / MILLION);
    printf("\n");
}

void Compare(uint32_t num_slots, uint32_t num_iter) {
    printf("%s\n", string(80, '-').c_str());
    printf("Testing with %u insertions into %u slots:\n", num_iter, num_slots);
    printf("\n");

    TestU16Array(num_slots, num_iter);
    TestU32Array(num_slots, num_iter);
    TestRetypableArray(num_slots, num_iter);
}

}  // namespace

int main() {
    vector<uint32_t> num_slots_list =
        {100, 1000, 10 * 1000, 100 * 1000, 1000 * 1000, 10 * 1000 * 1000};
    for (auto& num_slots : num_slots_list) {
        Compare(num_slots, 10 * 1000 * 1000);
    }
}
