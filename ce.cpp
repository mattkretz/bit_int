#include "integer.hpp"

#include <functional>

namespace std
{
  template <typename T>
    constexpr T& operator?:(bool, reference_wrapper<T>, T&) = default;
}

using std::integer;
using std::modulo_integer;
using std::basic_int;

// import iN and uN types
using namespace std::standard_integers;

// common_type is the type that can hold *all* values of either type
static_assert(std::is_same_v<std::common_type_t<i16, integer<20>>,
                             integer<20>>);
static_assert(std::is_same_v<std::common_type_t<i16, modulo_integer<20>>,
                             integer<21>>);
static_assert(std::is_same_v<std::common_type_t<u16, integer<20>>,
                             integer<20>>);
static_assert(std::is_same_v<std::common_type_t<u16, modulo_integer<20>>,
                             modulo_integer<20>>);
static_assert(std::is_same_v<std::common_type_t<_BitInt(16), i16>,
                             i16>);
static_assert(std::is_same_v<std::common_type_t<i16, u16>,
                             integer<17>>);
static_assert(std::is_same_v<std::common_type_t<short, u16>,
                             integer<17>>);
static_assert(std::is_same_v<std::common_type_t<unsigned short, u16>,
                             u16>);

// make_(un)signed works
static_assert(std::is_same_v<std::make_signed_t<i16>, i16>);
static_assert(std::is_same_v<std::make_signed_t<u16>, i16>);
static_assert(std::is_same_v<std::make_unsigned_t<i16>, u16>);
static_assert(std::is_same_v<std::make_unsigned_t<u16>, u16>);

// implicitly convertible iff the conversion is value-preserving
static_assert(    std::convertible_to<int, i32>);
static_assert(not std::convertible_to<int, integer<31>>);
static_assert(not std::convertible_to<int, u64>);
static_assert(    std::convertible_to<unsigned, integer<33>>);
static_assert(not std::convertible_to<unsigned, i32>);
static_assert(    std::convertible_to<unsigned, u32>);
static_assert(not std::convertible_to<unsigned, modulo_integer<31>>);
static_assert(    std::convertible_to<integer<5>, integer<6>>);
static_assert(not std::convertible_to<integer<5>, integer<4>>);
static_assert(    std::convertible_to<modulo_integer<5>, integer<6>>);
static_assert(not std::convertible_to<modulo_integer<5>, integer<5>>);

// explicit conversion is allowed for conversions that are not value-preserving
static_assert(std::constructible_from<integer<31>, int>);
static_assert(std::constructible_from<u64, int>);
static_assert(std::constructible_from<i32, unsigned>);
static_assert(std::constructible_from<modulo_integer<31>, unsigned>);
static_assert(std::constructible_from<integer<4>, integer<5>>);
static_assert(std::constructible_from<integer<5>, modulo_integer<5>>);

// basic_int never implicitly converts to not-basic_int integers
#if ALLOW_CONVERSION_TO_BUILTIN
static_assert(    std::convertible_to<i32, int>);
static_assert(    std::convertible_to<u32, unsigned>);
static_assert(    std::convertible_to<i32, _BitInt(32)>);
#else
static_assert(not std::convertible_to<i32, int>);
static_assert(not std::convertible_to<u32, unsigned>);
static_assert(not std::convertible_to<i32, _BitInt(32)>);
#endif

// But explicit conversion is allowed
static_assert(std::constructible_from<int, i32>);
static_assert(std::constructible_from<unsigned, u32>);
static_assert(std::constructible_from<_BitInt(32), i32>);

template <typename From, typename To>
  concept only_explicit_cast
    = requires { static_cast<To>(std::declval<From>()); } && not std::convertible_to<From, To>;

// conversion from floating-point needs an explicit cast (never value-preserving)
static_assert(only_explicit_cast<float, i64>);
static_assert(only_explicit_cast<float, i32>);
static_assert(only_explicit_cast<float, i8>);

// conversion to floating-point is implicit only if value-preserving
static_assert(only_explicit_cast<i32, float>);
static_assert(only_explicit_cast<integer<26>, float>);
static_assert(only_explicit_cast<modulo_integer<25>, float>);
static_assert(std::convertible_to<integer<25>, float>);
static_assert(std::convertible_to<modulo_integer<24>, float>);
static_assert(std::convertible_to<i32, double>);

// mixed-sign compares use value-preserving common type
static_assert(i16(-1) < u16(1));
static_assert(i16(1) == u16(1));
static_assert(i16(-1) != u16(1));

// cast that throws when the conversion changes the value
static_assert(std::value_preserving_cast<i16>(1) == 1wb);
static_assert([] {
  try
    {
      i8 x = std::value_preserving_cast<i8>(255);
      (void)x;
    }
  catch (const std::bad_value_preserving_cast&)
    {
      return true;
    }
  return false;
}());

// _BitInt conditional:
static_assert(std::value_preserving_cast<int>(true ? -1wb : 2uwb) != -1);
// vs basic_int conditional:
static_assert(std::value_preserving_cast<int>(true ? basic_int(-1wb) : basic_int(2uwb)) == -1);

// CTAD from constant_wrapper (or integral_constant) uses minimal width to represent the value
static_assert(std::is_same_v<decltype(basic_int(std::cw<3>)), modulo_integer<2>>);
static_assert(std::is_same_v<decltype(basic_int(std::cw<-3>)), integer<3>>);

// CTAD from _BitInt matches the _BitInt width and sign
static_assert(std::is_same_v<decltype(basic_int(-1wb)), integer<2>>);
static_assert(std::is_same_v<decltype(basic_int(1uwb)), modulo_integer<1>>);

static_assert((i8(1) << 7uwb) == i8::min());

static_assert(i32(1) + 1 == 2u);
static_assert(std::is_same_v<decltype(i32(1) + 1), i32>);

// brace init enforces value-preserving conversion
// 1. Either all possible values of the input type are representable by the basic_int, or
// 2. the input value must be a constant expression and the value itself can be preserved in the
//    basic_int
static_assert(i8{} == i8());
static_assert(i8{1} == i8(1));
static_assert([] {
  i8 a = 1wb;
  i8 b{a};
  if (a != b)
    return false;
  try
    {
      i8{255};
    }
  catch(const std::bad_value_preserving_cast&)
    {
      return true;
    }
  return false;
}());

bool f(i16 a, i32 b)
{
  return a == b;
}

// CTAD and conversions from wrappers such as reference_wrapper
auto f(i16 a, u16 b, short c)
{
  basic_int x = std::ref(a);
  i32 y = std::ref(a); // calls operator i16() and converts to i32
  (void)y;
  (void)(true ? std::ref(a) : a);
  (void)(true ? std::ref(a) : b);
  return std::array{ std::ref(a) == a, std::ref(a) != b, a != b, c == b, x < c };
}

auto f(int x) { i32 a{x}; return a; }

bool good(i32 a, u32 b)
{
  return a < b;
}

auto g(integer<4> b)
{
  b &= 0x7wb;
  return b;
}

template <auto>
void show_me(auto);

void test()
{
  constexpr basic_int a = 5wb;
  show_me<a>(a);
}
