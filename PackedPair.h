#pragma once
#include <type_traits>

/*
   A packed type containing two seperate types
*/
template <typename T, typename U, std::enable_if_t<std::is_trivially_copyable_v<U> && std::is_trivially_copyable_v<T>, int> = 0>
class PackedPair {
    static_assert(sizeof(T) <= 4, "T must fit within 32 bits.");
    static_assert(sizeof(U) <= 4, "E must fit within 32 bits.");

public:
    PackedPair(T firstValue, U secondValue) : data(pack(firstValue, secondValue)) {}

    // Extract the original T and E from the packed data.
    T first() const { return static_cast<T>(data & 0xFFFFFFFF); }
    U second() const { return static_cast<U>((data >> 32) & 0xFFFFFFFF); }

    // Ensure the size is fixed at 64 bits.

    // Comparison for equality (useful for CAS operations).
    bool operator==(const PackedPair& other) const { return data == other.data; }
    bool operator!=(const PackedPair& other) const { return !(*this == other); }

private:
    uint64_t data;

    // Helper to pack the value and enum into a single 64-bit integer.
    static uint64_t pack(T value, U enumValue) {
        uint64_t packedEnum = static_cast<uint64_t>(enumValue) << 32;
        uint64_t packedValue = static_cast<uint64_t>(value) & 0xFFFFFFFF;
        return packedEnum | packedValue;
    }

};
