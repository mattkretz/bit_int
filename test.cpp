#include "bit_int.hpp"

static_assert(std::is_same_v<std::common_type_t<bit_int<16>, bit_int<20>>, bit_int<20>>);
static_assert(std::is_same_v<std::common_type_t<bit_int<16>, bit_uint<20>>, bit_int<21>>);
static_assert(std::is_same_v<std::common_type_t<bit_uint<16>, bit_int<20>>, bit_int<20>>);
static_assert(std::is_same_v<std::common_type_t<bit_uint<16>, bit_uint<20>>, bit_uint<20>>);
static_assert(std::is_same_v<std::common_type_t<_BitInt(16), bit_int<16>>, bit_int<16>>);

static_assert(    std::convertible_to<int, bit_int<32>>);
static_assert(    std::convertible_to<unsigned, bit_int<33>>);
static_assert(    std::convertible_to<unsigned, bit_uint<32>>);
static_assert(    std::convertible_to<bit_int<5>, bit_int<6>>);
static_assert(not std::convertible_to<bit_int<5>, bit_int<4>>);
static_assert(    std::convertible_to<bit_uint<5>, bit_int<6>>);

// int/unsigned -> bit_*int<N> satisfies convertible_to because of the consteval constructor that
// ensures value-preserving conversions. This is not convertible in general: it requires a constant
// expression within a certain range of values.
static_assert(    std::convertible_to<int, bit_int<31>>);
static_assert(    std::convertible_to<int, bit_uint<64>>);
static_assert(    std::convertible_to<unsigned, bit_int<32>>);
static_assert(    std::convertible_to<unsigned, bit_uint<31>>);

// conversion from bit_*int back to int/unsigned is always explicit and even then requires a
// value-preserving conversion
static_assert(not std::convertible_to<bit_int<32>, int>);
static_assert(    std::constructible_from<int, bit_int<31>>);
static_assert(    std::constructible_from<int, bit_int<32>>);
static_assert(not std::constructible_from<int, bit_int<33>>);

static_assert(not std::convertible_to<bit_int<32>, unsigned>);
static_assert(not std::constructible_from<unsigned, bit_int<16>>);

static_assert(not std::convertible_to<bit_uint<32>, unsigned>);
static_assert(    std::constructible_from<unsigned, bit_uint<31>>);
static_assert(    std::constructible_from<unsigned, bit_uint<32>>);
static_assert(not std::constructible_from<unsigned, bit_uint<33>>);

static_assert(not std::convertible_to<bit_uint<32>, int>);
static_assert(    std::constructible_from<int, bit_uint<31>>);
static_assert(not std::constructible_from<int, bit_uint<32>>);
static_assert(not std::constructible_from<int, bit_uint<33>>);

static_assert(std::convertible_to<std::reference_wrapper<bit_uint<5>>, bit_int<6>>);
static_assert(std::constructible_from<bit_int<4>, std::reference_wrapper<bit_uint<5>>>);

static_assert(std::constructible_from<bit_int<31>, int>);
static_assert(std::constructible_from<bit_uint<64>, int>);
static_assert(std::constructible_from<bit_int<32>, unsigned>);
static_assert(std::constructible_from<bit_uint<31>, unsigned>);
static_assert(std::constructible_from<bit_int<4>, bit_int<5>>);
static_assert(std::constructible_from<bit_int<5>, bit_uint<5>>);

// no promotions
static_assert(std::is_same_v<bit_int_common_type_t<bit_int<16>, bit_uint<16>>, bit_int<17>>);
static_assert(std::is_same_v<bit_int_common_type_t<short, bit_uint<16>>, bit_int<17>>);
static_assert(std::is_same_v<bit_int_common_type_t<unsigned short, bit_uint<16>>, bit_uint<16>>);
static_assert(std::is_same_v<bit_int_common_type_t<short, std::reference_wrapper<bit_uint<16>>>,
    bit_int<17>>);

// correct mixed-signed compares
static_assert(bit_int<16>(-1) < bit_uint<16>(1));
static_assert(bit_int<16>(1) == bit_uint<16>(1));
static_assert(bit_int<16>(-1) != bit_uint<16>(1));

// CTAD from integral_constant deduces minimal width
static_assert(std::is_same_v<decltype(bit_int(std::integral_constant<int, 3>())), bit_uint<2>>);
static_assert(std::is_same_v<decltype(bit_int(std::integral_constant<int, -3>())), bit_int<3>>);
