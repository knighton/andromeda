#pragma once

namespace andromeda {
namespace distance {

template <typename int_t>
uint32_t HammingDistance(int_t a, int_t b);

template <typename int_t>
uint32_t BitsInCommon(int_t a, int_t b);

}  // namespace distance
}  // namespace andromeda

#include "hamming_impl.h"
