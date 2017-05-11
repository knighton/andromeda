#pragma once

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <type_traits>

namespace andromeda {
namespace base {

namespace {

template <typename A, typename B>
bool ResizeArrayType(size_t size, void** bytes) {
    A** old_items = (A**)bytes;
    B max_encodable_value = (B)~0ul;
    B* new_items = (B*)malloc(size * sizeof(B));
    for (size_t i = 0; i < size; ++i) {
        auto& value = (*old_items)[i];
        if (max_encodable_value < value) {
            free(new_items);
            return false;
        }
        new_items[i] = (B)value;
    }
    free(*bytes);
    *bytes = (void*)new_items;
    return true;
}

}  // namespace

template <typename N8, typename N16, typename N32, typename N64>
RetypableArray<N8, N16, N32, N64>::RetypableArray() {
}

template <typename N8, typename N16, typename N32, typename N64>
RetypableArray<N8, N16, N32, N64>::RetypableArray(const RetypableArray& rhs) {
    size_t num_bytes = rhs.size_ * rhs.int_size_;
    ints_ = malloc(num_bytes);
    std::memcpy(ints_, rhs.ints_, num_bytes);
    size_ = rhs.size_;
    int_size_ = rhs.int_size_;
    min_ = rhs.min_;
    max_ = rhs.max_;
}

template <typename N8, typename N16, typename N32, typename N64>
RetypableArray<N8, N16, N32, N64>::~RetypableArray() {
    if (ints_) {
        free(ints_);
    }
}

template <typename N8, typename N16, typename N32, typename N64>
bool RetypableArray<N8, N16, N32, N64>::Init(
        size_t size, size_t initial_int_size) {
    if (!size) {
        return false;
    }

    switch (initial_int_size) {
        case 1:
        case 2:
        case 4:
        case 8:
            break;
        default:
            return false;
    }

    size_ = size;
    int_size_ = initial_int_size;
    if (ints_) {
        free(ints_);
    }
    ints_ = malloc(size * int_size_);
    memset(ints_, 0, size * int_size_);
    
    return true;
}

template <typename N8, typename N16, typename N32, typename N64>
inline N64 RetypableArray<N8, N16, N32, N64>::GetMinValueBeforeResize() const {
    if (std::is_signed<N64>()) {
        return (N64)(~0l >> (8 - (int64_t)int_size_) * 8);
    } else {
        return 0;
    }
}

template <typename N8, typename N16, typename N32, typename N64>
inline N64 RetypableArray<N8, N16, N32, N64>::GetMaxValueBeforeResize() const {
    if (std::is_signed<N64>()) {
        return (N64)(~0l >> ((8 - (int64_t)int_size_) * 8 + 1));
    } else {
        return ~0ul >> (8 - int_size_) * 8;
    }
}

template <typename N8, typename N16, typename N32, typename N64>
bool RetypableArray<N8, N16, N32, N64>::ChangeIntWidth(size_t to_int_size) {
    auto f = ResizeArrayType<uint8_t, uint16_t>;
    switch (int_size_) {
        case 1:
            switch (to_int_size) {
                case 1:
                    return true;
                case 2:
                    f = ResizeArrayType<uint8_t, uint16_t>;
                    break;
                case 4:
                    f = ResizeArrayType<uint8_t, uint32_t>;
                    break;
                case 8:
                    f = ResizeArrayType<uint8_t, uint64_t>;
                    break;
                default:
                    return false;
            }
            break;
        case 2:
            switch (to_int_size) {
                case 1:
                    f = ResizeArrayType<uint16_t, uint8_t>;
                    break;
                case 2:
                    return true;
                case 4:
                    f = ResizeArrayType<uint16_t, uint32_t>;
                    break;
                case 8:
                    f = ResizeArrayType<uint16_t, uint64_t>;
                    break;
                default:
                    return false;
            }
            break;
        case 4:
            switch (to_int_size) {
                case 1:
                    f = ResizeArrayType<uint32_t, uint8_t>;
                    break;
                case 2:
                    f = ResizeArrayType<uint32_t, uint16_t>;
                    break;
                case 4:
                    return true;
                case 8:
                    f = ResizeArrayType<uint32_t, uint64_t>;
                    break;
                default:
                    return false;
            }
            break;
        case 8:
            switch (to_int_size) {
                case 1:
                    f = ResizeArrayType<uint64_t, uint8_t>;
                    break;
                case 2:
                    f = ResizeArrayType<uint64_t, uint16_t>;
                    break;
                case 4:
                    f = ResizeArrayType<uint64_t, uint32_t>;
                    break;
                case 8:
                    return true;
                default:
                    return false;
            }
            break;
        default:
            return false;
    }

    if (!f(size_, &ints_)) {
        return false;
    }

    int_size_ = to_int_size;

    return true;
}

template <typename N8, typename N16, typename N32, typename N64>
inline N64 RetypableArray<N8, N16, N32, N64>::Get(size_t index) const {
    switch (int_size_) {
        case 1:
            return ((N8*)ints_)[index];
        case 2:
            return ((N16*)ints_)[index];
        case 4:
            return ((N32*)ints_)[index];
        case 8:
            return ((N64*)ints_)[index];
        default:
            assert(false);
    }
}

template <typename N8, typename N16, typename N32, typename N64>
inline void RetypableArray<N8, N16, N32, N64>::Set(
        size_t index, N64 new_value) {
    N64 min_encodable = GetMinValueBeforeResize();
    if (new_value < min_encodable) {
        assert(ChangeIntWidth(int_size_ * 2));
        Set(index, new_value);
        return;
    }

    N64 max_encodable = GetMaxValueBeforeResize();
    if (max_encodable < new_value) {
        assert(ChangeIntWidth(int_size_ * 2));
        Set(index, new_value);
        return;
    }

    switch (int_size_) {
        case 1:
            {
                auto& value = ((N8*)ints_)[index];
                value = (N8)new_value;
                break;
            }
        case 2:
            {
                auto& value = ((N16*)ints_)[index];
                value = (N16)new_value;
                break;
            }
        case 4:
            {
                auto& value = ((N32*)ints_)[index];
                value = (N32)new_value;
                break;
            }
        case 8:
            {
                auto& value = ((N64*)ints_)[index];
                value = (N64)new_value;
                break;
            }
        default:
            assert(false);
    }
}

template <typename N8, typename N16, typename N32, typename N64>
bool RetypableArray<N8, N16, N32, N64>::Incr(
        size_t index, N64 incr) {
    N64 old_value = Get(index);
    N64 new_value = old_value + incr;

    if (0 < incr) {
        if (new_value < old_value) {
            return false;
        }
    } else {
        if (old_value < new_value) {
            return false;
        }
    }

    N64 min_encodable = GetMinValueBeforeResize();
    if (new_value < min_encodable) {
        assert(ChangeIntWidth(int_size_ * 2));
        Set(index, new_value);
        return true;
    }

    N64 max_encodable = GetMaxValueBeforeResize();
    if (max_encodable < new_value) {
        assert(ChangeIntWidth(int_size_ * 2));
        Set(index, new_value);
        return true;
    }

    switch (int_size_) {
        case 1:
            {
                auto& value = ((N8*)ints_)[index];
                value = (N8)new_value;
                break;
            }
        case 2:
            {
                auto& value = ((N16*)ints_)[index];
                value = (N16)new_value;
                break;
            }
        case 4:
            {
                auto& value = ((N32*)ints_)[index];
                value = (N32)new_value;
                break;
            }
        case 8:
            {
                auto& value = ((N64*)ints_)[index];
                value = (N64)new_value;
                break;
            }
        default:
            assert(false);
    }

    return true;
}

}  // namespace base
}  // namespace andromeda
