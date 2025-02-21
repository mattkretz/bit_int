#include "bit_int.hpp"

static_assert(std::is_same_v<std::common_type_t<bit_int<16>, bit_int<20>>, bit_int<20>>);
static_assert(std::is_same_v<std::common_type_t<bit_int<16>, bit_uint<20>>, bit_int<21>>);
static_assert(std::is_same_v<std::common_type_t<bit_uint<16>, bit_int<20>>, bit_int<20>>);
static_assert(std::is_same_v<std::common_type_t<bit_uint<16>, bit_uint<20>>, bit_uint<20>>);

static_assert(    std::convertible_to<int, bit_int<32>>);
static_assert(not std::convertible_to<int, bit_int<31>>);
static_assert(not std::convertible_to<int, bit_uint<64>>);
static_assert(    std::convertible_to<unsigned, bit_int<33>>);
static_assert(not std::convertible_to<unsigned, bit_int<32>>);
static_assert(    std::convertible_to<unsigned, bit_uint<32>>);
static_assert(not std::convertible_to<unsigned, bit_uint<31>>);
static_assert(    std::convertible_to<bit_int<5>, bit_int<6>>);
static_assert(not std::convertible_to<bit_int<5>, bit_int<4>>);
static_assert(    std::convertible_to<bit_uint<5>, bit_int<6>>);

static_assert(std::convertible_to<std::reference_wrapper<bit_uint<5>>, bit_int<6>>);
static_assert(std::constructible_from<bit_int<4>, std::reference_wrapper<bit_uint<5>>>);

static_assert(std::constructible_from<bit_int<31>, int>);
static_assert(std::constructible_from<bit_uint<64>, int>);
static_assert(std::constructible_from<bit_int<32>, unsigned>);
static_assert(std::constructible_from<bit_uint<31>, unsigned>);
static_assert(std::constructible_from<bit_int<4>, bit_int<5>>);
static_assert(std::constructible_from<bit_int<5>, bit_uint<5>>);

static_assert(std::is_same_v<bit_int_common_type_t<bit_int<16>, bit_uint<16>>, bit_int<17>>);
static_assert(std::is_same_v<bit_int_common_type_t<short, bit_uint<16>>, bit_int<17>>);
static_assert(std::is_same_v<bit_int_common_type_t<unsigned short, bit_uint<16>>, bit_uint<16>>);
static_assert(std::is_same_v<bit_int_common_type_t<short, std::reference_wrapper<bit_uint<16>>>,
    bit_int<17>>);

static_assert(bit_int<16>(-1) < bit_uint<16>(1));
static_assert(bit_int<16>(1) == bit_uint<16>(1));
static_assert(bit_int<16>(-1) != bit_uint<16>(1));

static_assert(std::is_same_v<decltype(bit_int(std::integral_constant<int, 3>())), bit_uint<2>>);
static_assert(std::is_same_v<decltype(bit_int(std::integral_constant<int, -3>())), bit_int<3>>);

auto f(bit_int<16> a, bit_uint<16> b, short c)
{
  bit_int x = std::ref(a);
  return std::array{ std::ref(a) == a, std::ref(a) != b, a != b, c == b, x < c };
}

bool good(bit_int<64> a, bit_uint<64> b)
{
  return a == b;
}

bool bad(_BitInt(64) a, unsigned _BitInt(64) b)
{
  return a == b;
}
