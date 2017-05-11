#include <armadillo>
#include <cassert>

#include "distance/euclidean.h"

using andromeda::distance::EuclideanDistance;

int main() {
    arma::Row<float> a = {0, 0};
    arma::Row<float> b = a;
    assert(EuclideanDistance(a, b) == 0);
    b = {3, 4};
    assert(EuclideanDistance(a, b) == 5);
}
