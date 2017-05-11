#pragma once

#include <cstddef>
#include <cstdint>

namespace andromeda {
namespace base {

// For storing large arrays of small-valued counts.
//
// You don't know if they will fit in uint8_t or require uin16_t, so you use
// this class and it will spill over automatically.
//
// Base version (use either signed or unsigned versions below).
template <typename N8, typename N16, typename N32, typename N64>
class RetypableArray {
  public:
    const void* ints() const { return ints_; }
    size_t size() const { return size_; }
    size_t int_size() const { return int_size_; }
    N64 min() const { return min_; }
    N64 max() const { return max_; }

    // Free memory.
    RetypableArray();
    RetypableArray(const RetypableArray& rhs);
    ~RetypableArray();

    // Simply initializes its memory.  Returns true on success.
    //
    // * `size` must be nonzero.
    // * `initial_int_size` must be in {1, 2, 4, 8}.
    bool Init(size_t size, size_t initial_int_size);

    // Get the range outside of which it will resize its integer type.
    N64 GetMinValueBeforeResize() const;
    N64 GetMaxValueBeforeResize() const;

    // Attempts to change the width of the int type that stores its values (eg,
    // to accomodate larger numbers).
    //
    // * `to_int_size` must be in {1, 2, 4, 8}.
    //
    // Returns false and stays in the previous state if (a) it encountered a
    // number that was too big to be encoded in the new width.
    bool ChangeIntWidth(size_t to_int_size);

    // Getters/setters.
    N64 Get(size_t index) const;
    void Set(size_t index, N64 new_value);
    bool Incr(size_t index, N64 incr);

  private:
    void* ints_{nullptr};
    size_t size_{0};
    size_t int_size_{0};
    N64 min_{0};
    N64 max_{0};
};

// Unsigned version.
using UnsignedRetypableArray =
    RetypableArray<uint8_t, uint16_t, uint32_t, uint64_t>;

// Signed version.
using SignedRetypableArray =
    RetypableArray<int8_t, int16_t, int32_t, int64_t>;

}  // namespace base
}  // namespace andromeda

#include "retypable_array_impl.h"
