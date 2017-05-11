#pragma once

namespace andromeda {
namespace base {

template <typename float_t, typename uint_t>
void QuantizeEmbedding(const arma::Row<float_t>& unit_vector,
                       uint32_t to_num_bits, float noise, uint_t* quantized);

}  // namespace base
}  // namespace andromeda

#include "quantization_impl.h"
