#pragma once

#include <cassert>

namespace andromeda {
namespace base {

template <typename float_t, typename uint_t>
void QuantizeEmbedding(const arma::Row<float_t>& unit_vector,
                       uint32_t to_num_bits, float noise, uint_t* quantized) {
    *quantized = 0;
    assert(to_num_bits <= sizeof(*quantized) * 8);
    assert(0 <= noise);
    assert(noise <= 1);
    for (uint32_t i = 0; i < to_num_bits; ++i) {
        uint32_t begin = i * (uint32_t)unit_vector.size() / to_num_bits;
        uint32_t end_excl =
            (i + 1) * (uint32_t)unit_vector.size() / to_num_bits;
        if (begin == end_excl) {
            continue;
        }

        float_t sum = 0.0;
        for (uint32_t j = begin; j < end_excl; ++j) {
            sum += unit_vector(j);
        }
        float_t avg = sum / (end_excl - begin);

        uint_t bit = avg < 0.0 ? 0 : 1;
        if (noise != 0.0) {
            float frac = (uint32_t)rand() / UINT32_MAX;
            if (frac < noise) {
                bit = 1 - bit;
            }
        }

        (*quantized) |= bit << i;
    }
}

}  // namespace base
}  // namespace andromeda
