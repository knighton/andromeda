#pragma once

#include <armadillo>
#include <cstdint>
#include <utility>
#include <vector>

#include "base/int128.h"
#include "base/retypable_array.h"

using andromeda::base::uint128_t;
using andromeda::base::UnsignedRetypableArray;
using std::pair;
using std::vector;

namespace andromeda {
namespace heatspace {

// Items are either points or clouds.
typedef uint32_t ItemID;

// A point is a single high-dimensional unit vector.
typedef uint32_t PointID;

// Clouds are collections of points.  Each collection has an ID.  Groups of
// collections are collection types.
typedef uint32_t CloudType;
typedef uint32_t CloudID;

typedef vector<pair<float, CloudID>> PointCloudMemberships;

typedef vector<pair<float, PointID>> CloudPoints;

enum class LookupStatus {
    OK,
    ERROR_NO_ITEMS_TO_LOOK_UP,
    ERROR_BAD_DIM,
    ERROR_BAD_POINT_ID,
    ERROR_BAD_WEIGHT,
    ERROR_BAD_CLOUD_TYPE,
    ERROR_BAD_CLOUD_ID,
    ERROR_POINTS_WEIGHTS_MISMATCH
};

#define RESULTS_TYPE_POINTS UINT32_MAX

struct LookupParams {
    uint32_t num_threads_or_zero{0};
    float rotation_sigma{0.1f};
    float quantization_noise{0.05f};
    uint32_t lookups_per_thread{8};
    uint32_t points_per_lookup{32};
    uint32_t results_type{RESULTS_TYPE_POINTS};
    uint32_t num_results{30};

    uint32_t num_threads() const;
};

class EmbeddingHeatspace {
  public:
    // (Re-)construct the index, enabling fast queries over the embedding space.
    //
    // * `points`                   The embeddings (ie, high-dimensional unit
    //                              vectors).
    //
    // * `weights`                  Associated weight multipliers for each
    //                              point. Higher weights are more promiment.
    //
    // * `type2cloud2weights_ids`   The definitions of each point cloud, by
    //                              type.
    //
    // * `pyramid_qz_per_point`     How many times to insert quantized forms of
    //                              each point into the pyramid.  Higher leads
    //                              to smoother lookup results.
    //
    // * `rotation_sigma`           Deviation in the Gaussian distribution used
    //                              to randomly rotate embeddings slightly
    //                              before quantization.
    //
    // * `quantization_noise`       Fraction of random bit flips during
    //                              quantization (to smooth the distribution).
    //
    // * `target_points_per_bucket` Average number of points per bucket at the
    //                              bottom (most fine-grained) resolution level
    //                              of the pyramid that we desire (we err on the
    //                              low side).
    //
    // * `u128_per_point`           How many max-resolution (128-bit)
    //                              quantizations of each embedding to store for
    //                              bitwise embedding quantization comparison.
    //                              More quantizations results in smoother
    //                              comparisons, however you could compare the
    //                              (lossless) embeddding floats themselves
    //                              reasonaby fast.
    void Init(
        const arma::Mat<float>& points, const vector<float>& point_weights,
        const vector<vector<CloudPoints>>& cloudtype2cloud2weights_ids,
        uint32_t pyramid_qz_per_point, float rotation_sigma,
        float quantization_noise, float target_points_per_bucket,
        uint32_t u128_per_point);

    // Find neigbhors with high proximity and weight, given parameters.
    //
    // Possible inputs:
    // * Embedding
    // * Group of weighted embeddings
    // * Point ID
    // * Group of weighted Point IDs
    // * Cloud ID
    //
    // Possible outputs:
    // * Point IDs
    // * Cloud IDs
    LookupStatus SimpleLookupLocation(
        const arma::Row<float>& location, float rotation_sigma,
        float quantization_noise, uint32_t results_type, uint32_t num_results,
        vector<pair<float, ItemID>>* weights_ids) const;

    LookupStatus LookupLocation(
        const arma::Row<float>& location, const LookupParams& params,
        vector<pair<float, ItemID>>* weights_ids) const;

    LookupStatus LookupLocations(
        const arma::Mat<float>& locations, const vector<float>& weights,
        const LookupParams& params,
        vector<pair<float, ItemID>>* weights_ids) const;

    LookupStatus LookupPoint(
        PointID point_id, const LookupParams& params,
        vector<pair<float, ItemID>>* weights_ids) const;

    LookupStatus LookupPoints(
        const vector<pair<float, PointID>>& weights_points,
        const LookupParams& params,
        vector<pair<float, ItemID>>* weights_ids) const;

    LookupStatus LookupCloud(
        CloudType cloud_type, CloudID cloud_id, const LookupParams& params,
        vector<pair<float, ItemID>>* weights_ids) const;

  private:
    LookupStatus LookupLocationsCommon(
        const arma::Mat<float>& locations, const vector<float>& weights,
        const LookupParams& params,
        vector<pair<float, ItemID>>* weights_ids) const;

    // The original points and weights.
    arma::Mat<float> points_;
    vector<float> point_weights_;

    // The pyramid of successively more fine-grained quantizations.
    //
    // Each resolution level is indexed by hashes that are 4 bits longer than
    // the last one (ie, 16-way fanout).  They start at 4 bits (resolution
    // level 0).
    vector<UnsignedRetypableArray> resolution2qz2count_;

    // List of IDs per quantization at the bottom of the quantization pyramid.
    //
    // Quantization -> list of point IDs.
    vector<vector<PointID>> ids_per_qz_;

    // Several highest-resolution quantizations for each point, for fast
    // comparison.
    //
    // Point ID -> list of quantizations.
    vector<vector<uint128_t>> id2proj2bits_;

    // Cloud membership.
    //
    // Cloud type -> cloud ID -> list of (weight, point ID).
    // Cloud type -> point ID -> list of (weight, cloud ID).
    vector<vector<CloudPoints>> cloudtype2cloud2weights_points_;
    vector<vector<PointCloudMemberships>> cloudtype2point2weights_clouds_;
};

}  // namespace heatspace
}  // namespace andromeda
