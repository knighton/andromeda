#pragma once

namespace andromeda {
namespace distance {

template <typename T>
T EuclideanDistance(const arma::Row<T>& a, const arma::Row<T>& b) {
    T r = 0.0;
    for (uint32_t i = 0; i < a.n_cols; ++i) {
        r += (b(i) - a(i)) * (b(i) - a(i));
    }
    return (T)sqrt(r);
}

}  // namespace distance
}  // namespace andromeda
