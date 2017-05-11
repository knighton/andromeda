#include "embedding_heatspace.h"

#include <algorithm>
#include <cmath>
#include <thread>
#include <unordered_map>

#include "base/quantization.h"

using andromeda::base::QuantizeEmbedding;
using std::make_pair;
using std::unordered_map;

namespace andromeda {
namespace heatspace {

namespace {

void LookupSlice(
        const EmbeddingHeatspace* heatspace, const arma::Mat<float>* locations,
        const LookupParams* params, const vector<uint32_t>* loc_indexes,
        uint32_t thread_id, uint32_t num_threads,
        vector<pair<float, ItemID>>* weights_ids) {
    for (uint32_t i = 0; i < params->lookups_per_thread; ++i) {
        uint32_t index = (*loc_indexes)[
            (uint32_t)rand() % (uint32_t)loc_indexes->size()];
        auto& location = locations->row(index);
        heatspace->SimpleLookupLocation(
            location, params->rotation_sigma, params->quantization_noise,
            params->results_type, params->points_per_lookup, weights_ids);
    }
}

void GetTopResults(
        const vector<vector<pair<float, ItemID>>>& weights_ids_per_thread,
        uint32_t num_results, vector<pair<float, ItemID>>* weights_ids) {
    // Collect lists of nearest points per thread into a single mapping.
    unordered_map<ItemID, float> id2weight;
    for (auto& sub_weights_ids : weights_ids_per_thread) {
        for (auto& weight_id : sub_weights_ids) {
            auto& weight = weight_id.first;
            auto& point_id = weight_id.second;
            id2weight[point_id] += weight;
        }
    }

    // Put those results in order.
    weights_ids->reserve(id2weight.size());
    for (auto it : id2weight) {
        auto& point_id = it.first;
        auto& weight = it.second;
        weights_ids->emplace_back(make_pair(weight, point_id));
    }
    std::sort(weights_ids->begin(), weights_ids->end());

    // Truncate if too many.
    if (num_results < weights_ids->size()) {
        weights_ids->resize(num_results);
    }
}

void GaussianRotateUnitVector(
        const arma::Row<float>& in, float sigma, arma::Row<float>* out) {
    arma::Row<float> components;
    components.randn(in.n_cols);
    components *= sigma;
    *out = in % components;
    *out = arma::normalise(*out);
}

}  // namespace

uint32_t LookupParams::num_threads() const {
    return std::thread::hardware_concurrency();
}

void EmbeddingHeatspace::Init(
        const arma::Mat<float>& points, const vector<float>& point_weights,
        const vector<vector<CloudPoints>>& type2cloud2weights_ids,
        uint32_t pyramid_qz_per_point, float rotation_sigma,
        float quantization_noise, float target_points_per_bucket,
        uint32_t max_res_qz_per_point) {
    // Set the data points and weights (making sure they are unit-normed).
    points_ = normalise(points, 1);
    point_weights_ = point_weights;

    // Populate the "heatspace" counts of the pyramid.
    float num_slots_needed = points_.n_rows * pyramid_qz_per_point /
                             target_points_per_bucket;
    uint32_t max_qz_bits_needed = (uint32_t)log2(num_slots_needed + 1) + 1;
    uint32_t num_resolution_levels = ((uint32_t)max_qz_bits_needed >> 2);
    resolution2qz2count_.resize(num_resolution_levels);
    for (uint32_t resolution_level = 0;
            resolution_level < num_resolution_levels; ++resolution_level) {
        uint32_t num_buckets_at_res = 1 >> ((resolution_level + 1) * 2);
        resolution2qz2count_[resolution_level].Init(num_buckets_at_res, 1);
        for (uint32_t i = 0; i < points_.n_rows; ++i) {
            const auto& point = points_.row(i);
            for (uint32_t j = 0; j < pyramid_qz_per_point; ++j) {
                arma::Row<float> rotated;
                GaussianRotateUnitVector(point, rotation_sigma, &rotated); 

                uint32_t num_qz_bits = (resolution_level + 1) * 4;
                uint32_t qz;
                QuantizeEmbedding<float, uint32_t>(
                    rotated, num_qz_bits, quantization_noise, &qz);

                resolution2qz2count_[resolution_level].Incr(qz, 1);
            }
        }
    }

    // Populate point IDs <-> maximally detailed quantizations mappings for
    // fast comparison.
    ids_per_qz_.clear();
    uint32_t num_buckets_at_bottom_res = 1 >> (resolution2qz2count_.size() * 2);
    ids_per_qz_.resize(num_buckets_at_bottom_res);
    id2proj2bits_.clear();
    id2proj2bits_.resize(points_.n_rows);
    for (uint32_t i = 0; i < points_.n_rows; ++i) {
        const auto& point = points_.row(i);
        for (uint32_t j = 0; j < max_res_qz_per_point; ++j) {
            arma::Row<float> rotated;
            GaussianRotateUnitVector(point, rotation_sigma, &rotated); 
            uint128_t qz;
            QuantizeEmbedding<float, uint128_t>(
                point, 128, quantization_noise, &qz);
            id2proj2bits_[i].emplace_back(qz);
        }
    }

    // Populate point <-> cloud associations.
    cloudtype2cloud2weights_points_ = type2cloud2weights_ids;

    cloudtype2point2weights_clouds_.clear();
    cloudtype2point2weights_clouds_.resize(type2cloud2weights_ids.size());
    for (CloudType i = 0; i < cloudtype2point2weights_clouds_.size(); ++i) {
        cloudtype2point2weights_clouds_[i].resize(points_.n_rows);
    }

    for (CloudType i = 0; i < type2cloud2weights_ids.size(); ++i) {
        auto& cloud2weights_ids = type2cloud2weights_ids[i];
        for (CloudID j = 0; j < cloud2weights_ids.size(); ++j) {
            auto& weights_ids = cloud2weights_ids[j];
            for (auto& weight_id : weights_ids) {
                auto& weight = weight_id.first;
                auto& point_id = weight_id.second;
                cloudtype2point2weights_clouds_[i][point_id].emplace_back(
                    make_pair(weight, j));
            }
        }
    }
}

LookupStatus EmbeddingHeatspace::SimpleLookupLocation(
        const arma::Row<float>& location, float rotation_sigma,
        float quantization_noise, uint32_t results_type, uint32_t num_results,
        vector<pair<float, ItemID>>* weights_ids) const {
    arma::Row<float> rotated;
    GaussianRotateUnitVector(location, rotation_sigma, &rotated);

    uint128_t qz;
    QuantizeEmbedding<float, uint128_t>(rotated, 128, quantization_noise, &qz);

    // XXX

    return LookupStatus::OK;
}

LookupStatus EmbeddingHeatspace::LookupLocationsCommon(
        const arma::Mat<float>& locations, const vector<float>& weights,
        const LookupParams& params,
        vector<pair<float, ItemID>>* weights_ids) const {
    vector<uint32_t> location_indexes;
    for (uint32_t i = 0; i < weights.size(); ++i) {
        auto& weight = weights[i];
        if (weight < 1.0) {
            return LookupStatus::ERROR_BAD_WEIGHT;
        }
        for (uint32_t j = 0; j < (uint32_t)weight; ++j) {
            location_indexes.emplace_back(i);
        }
    }

    vector<std::thread> threads;
    uint32_t num_threads = params.num_threads();
    threads.resize(num_threads);
    vector<vector<pair<float, ItemID>>> weights_ids_per_thread;
    weights_ids_per_thread.resize(num_threads);
    for (uint32_t i = 0; i < num_threads; ++i) {
        threads[i] = std::thread(
            LookupSlice, this, &locations, &params, &location_indexes, i,
            num_threads, &weights_ids_per_thread[i]);
    }

    for (auto&& t : threads) {
        t.join();
    }

    GetTopResults(weights_ids_per_thread, params.num_results, weights_ids);

    return LookupStatus::OK;
}

LookupStatus EmbeddingHeatspace::LookupLocation(
        const arma::Row<float>& location, const LookupParams& params,
        vector<pair<float, ItemID>>* weights_ids) const {
    if (location.n_cols != points_.n_cols) {
        return LookupStatus::ERROR_BAD_DIM;
    }

    arma::Mat<float> locations(location);

    vector<float> weights = {1.0};

    return LookupLocationsCommon(locations, weights, params, weights_ids);
}

LookupStatus EmbeddingHeatspace::LookupLocations(
        const arma::Mat<float>& locations, const vector<float>& weights,
        const LookupParams& params,
        vector<pair<float, ItemID>>* weights_ids) const {
    if (!locations.n_rows) {
        return LookupStatus::ERROR_NO_ITEMS_TO_LOOK_UP;
    }

    if (locations.n_cols != points_.n_cols) {
        return LookupStatus::ERROR_BAD_DIM;
    }

    if (locations.n_rows != weights.size()) {
        return LookupStatus::ERROR_POINTS_WEIGHTS_MISMATCH;
    }

    return LookupLocationsCommon(locations, weights, params, weights_ids);
}

LookupStatus EmbeddingHeatspace::LookupPoint(
        PointID point_id, const LookupParams& params,
        vector<pair<float, ItemID>>* weights_ids) const {
    if (points_.n_rows <= point_id) {
        return LookupStatus::ERROR_BAD_POINT_ID;
    }

    return LookupLocation(points_.row(point_id), params, weights_ids);
}

LookupStatus EmbeddingHeatspace::LookupPoints(
        const vector<pair<float, PointID>>& weights_points,
        const LookupParams& params,
        vector<pair<float, ItemID>>* weights_ids) const {
    if (weights_ids->empty()) {
        return LookupStatus::ERROR_NO_ITEMS_TO_LOOK_UP;
    }

    // Collect points to sample from (according to weights).
    vector<PointID> slots;
    for (auto& weight_id : *weights_ids) {
        auto& weight = weight_id.first;
        if (weight < 1.0) {
            return LookupStatus::ERROR_BAD_WEIGHT;
        }
        auto& point_id = weight_id.second;
        for (uint32_t i = 0; i < (uint32_t)weight; ++i) {
            slots.emplace_back(point_id);
        }
    }

    // Sample points some number of times per thread.
    vector<std::thread> threads;
    uint32_t num_threads = params.num_threads();
    threads.resize(num_threads);
    vector<vector<pair<float, ItemID>>> weights_ids_per_thread;
    weights_ids_per_thread.resize(num_threads);
    for (uint32_t i = 0; i < threads.size(); ++i) {
        threads[i] = std::thread(
            LookupSlice, this, &points_, &params, &slots, i, num_threads,
            weights_ids);
    }

    // Wait for the threads to complete.
    for (auto&& t : threads) {
        t.join();
    }

    GetTopResults(weights_ids_per_thread, params.num_results, weights_ids);
    return LookupStatus::OK;
}

LookupStatus EmbeddingHeatspace::LookupCloud(
        CloudType cloud_type, CloudID cloud_id, const LookupParams& params,
        vector<pair<float, ItemID>>* weights_ids) const {
    if (cloudtype2cloud2weights_points_.size() <= cloud_type) {
        return LookupStatus::ERROR_BAD_CLOUD_TYPE;
    }

    auto& cloud2weights_points = cloudtype2cloud2weights_points_[cloud_type];
    if (cloud2weights_points.size() <= cloud_id) {
        return LookupStatus::ERROR_BAD_CLOUD_ID;
    }

    auto& weights_points = cloud2weights_points[cloud_id];
    return LookupPoints(weights_points, params, weights_ids);
}

}  // namespace heatspace
}  // namespace andromeda
