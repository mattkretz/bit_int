#include "integer.hpp"

#include <functional>

static_assert(20 + std::_Signed(true) == 21);
static_assert(20 + std::_Signed(false) == 20);
static_assert(20 + ~std::_Signed(true) == 20);
static_assert(20 + ~std::_Signed(false) == 21);

static_assert(std::i8::width() == 8);
static_assert(std::i16::width() == 16);
static_assert(std::i32::width() == 32);
static_assert(std::i64::width() == 64);

static_assert(std::i8::digits() == 7);
static_assert(std::i16::digits() == 15);
static_assert(std::i32::digits() == 31);
static_assert(std::i64::digits() == 63);

static_assert(std::is_same_v<std::common_type_t<std::integer<16>, std::integer<20>>, std::integer<20>>);
static_assert(std::is_same_v<std::common_type_t<std::integer<16>, std::modulo_integer<20>>, std::integer<21>>);
static_assert(std::is_same_v<std::common_type_t<std::modulo_integer<16>, std::integer<20>>, std::integer<20>>);
static_assert(std::is_same_v<std::common_type_t<std::modulo_integer<16>, std::modulo_integer<20>>, std::modulo_integer<20>>);
static_assert(std::is_same_v<std::common_type_t<_BitInt(16), short>, int>);
// GCC? static_assert(std::is_same_v<std::common_type_t<_BitInt(16), std::integer<16>>, std::integer<16>>);

static_assert(    std::convertible_to<int, std::integer<32>>);
static_assert(    std::convertible_to<unsigned, std::integer<33>>);
static_assert(    std::convertible_to<unsigned, std::modulo_integer<32>>);
static_assert(    std::convertible_to<std::integer<5>, std::integer<6>>);
static_assert(not std::convertible_to<std::integer<5>, std::integer<4>>);
static_assert(    std::convertible_to<std::modulo_integer<5>, std::integer<6>>);

// int/unsigned -> bit_*int<N> satisfies convertible_to because of the consteval constructor that
// ensures value-preserving conversions. This is not convertible in general: it requires a constant
// expression within a certain range of values.
static_assert(not std::convertible_to<int, std::integer<31>>);
static_assert(not std::convertible_to<int, std::modulo_integer<64>>);
static_assert(not std::convertible_to<unsigned, std::integer<32>>);
static_assert(not std::convertible_to<unsigned, std::modulo_integer<31>>);

// conversion from bit_*int back to int/unsigned is always explicit and even then requires a
// value-preserving conversion
#if ALLOW_CONVERSION_TO_BUILTIN
static_assert(    std::convertible_to<std::integer<32>, int>);
#else
static_assert(not std::convertible_to<std::integer<32>, int>);
#endif
static_assert(    std::constructible_from<int, std::integer<31>>);
static_assert(    std::constructible_from<int, std::integer<32>>);
static_assert(    std::constructible_from<int, std::integer<33>>);

static_assert(not std::convertible_to<std::integer<32>, unsigned>);
static_assert(    std::constructible_from<unsigned, std::integer<16>>);

#if ALLOW_CONVERSION_TO_BUILTIN
static_assert(    std::convertible_to<std::modulo_integer<32>, unsigned>);
#else
static_assert(not std::convertible_to<std::modulo_integer<32>, unsigned>);
#endif
static_assert(    std::constructible_from<unsigned, std::modulo_integer<31>>);
static_assert(    std::constructible_from<unsigned, std::modulo_integer<32>>);
static_assert(    std::constructible_from<unsigned, std::modulo_integer<33>>);

static_assert(not std::convertible_to<std::modulo_integer<32>, int>);
static_assert(    std::constructible_from<int, std::modulo_integer<31>>);
static_assert(    std::constructible_from<int, std::modulo_integer<32>>);
static_assert(    std::constructible_from<int, std::modulo_integer<33>>);

static_assert(std::convertible_to<std::reference_wrapper<std::modulo_integer<5>>, std::integer<6>>);
static_assert(std::constructible_from<std::integer<4>, std::reference_wrapper<std::modulo_integer<5>>>);

static_assert(std::constructible_from<std::integer<31>, int>);
static_assert(std::constructible_from<std::modulo_integer<64>, int>);
static_assert(std::constructible_from<std::integer<32>, unsigned>);
static_assert(std::constructible_from<std::modulo_integer<31>, unsigned>);
static_assert(std::constructible_from<std::integer<4>, std::integer<5>>);
static_assert(std::constructible_from<std::integer<5>, std::modulo_integer<5>>);

// correct mixed-type compares
static_assert(std::integer<16>(-1) < std::modulo_integer<16>(1));
static_assert(std::integer<16>(1) == std::modulo_integer<16>(1));
static_assert(std::integer<16>(-1) != std::modulo_integer<16>(1));
static_assert(std::integer<16>(1) != float(1.5));
static_assert(std::integer<16>(1) < float(1.5));

// CTAD from integral_constant deduces minimal width
static_assert(std::is_same_v<decltype(std::basic_int(std::integral_constant<int, 3>())), std::modulo_integer<2>>);
static_assert(std::is_same_v<decltype(std::basic_int(std::integral_constant<int, -3>())), std::integer<3>>);

static_assert(std::integer<32>::max() == std::numeric_limits<int>::max());
static_assert(std::integer<32>::min() == std::numeric_limits<int>::min());
static_assert(std::integer<32>::digits == std::numeric_limits<int>::digits);

static_assert(std::modulo_integer<32>::max() == std::numeric_limits<unsigned>::max());
static_assert(std::modulo_integer<32>::min() == std::numeric_limits<unsigned>::min());
static_assert(std::modulo_integer<32>::digits() == std::numeric_limits<unsigned>::digits);

template <std::size_t N>
using cbit_uint = unsigned _BitInt(N);

static_assert(std::is_same_v<decltype(std::basic_int(_BitInt(3)())), std::integer<3>>);
static_assert(std::is_same_v<decltype(std::basic_int(cbit_uint<3>())), std::modulo_integer<3>>);

static_assert(std::is_same_v<decltype(std::basic_int(0b11wb)), std::integer<3>>);
static_assert(std::is_same_v<decltype(std::basic_int(0b111uwb)), std::modulo_integer<3>>);

static_assert([] {
#if __cpp_constexpr_exceptions >= 202411L
  try { value_preserving_cast<int>(std::integer<33>::max()); }
  catch (const std::bad_value_preserving_cast&) {}
#endif
  value_preserving_cast<int>(std::integer<33>::max() / 2);
  value_preserving_cast<int>(std::integer<33>::min() / 2);
  return true;
}());

static_assert(true ? std::i8(-1) : std::u8(1) == std::i8(-1));
static_assert(std::is_same_v<decltype(true ? std::i8(-1) : std::u8(1)), std::integer<9>>);

constexpr bool
f(std::i16 x)
{ return x == 1; };

bool
test(unsigned char one)
{ return f({one}); } // OK because value-preserving conversion

// implicit value-preserving cast in consteval
static_assert([] {
  return f({1}); // f(1) would be ill-formed because int->i16 is explicit
}());
