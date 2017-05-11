#pragma once

#include <armadillo>

namespace andromeda {
namespace distance {

template <typename T>
T EuclideanDistance(const arma::Row<T>& a, const arma::Row<T>& b);

}  // namespace distance
}  // namespace andromeda

#include "euclidean_impl.h"
