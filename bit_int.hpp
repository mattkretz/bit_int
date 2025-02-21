/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include <algorithm>
#include <climits>
#include <concepts>
#include <functional>

enum class sign { Signed = 0, Unsigned = 1 };

constexpr int
operator+(int lhs, sign rhs)
{ return lhs + static_cast<int>(rhs); }

constexpr sign
operator!(sign s)
{ return s == sign::Signed ? sign::Unsigned : sign::Signed; }

template <int W, sign S = sign::Signed>
  class bit_int;

template <int W>
  using bit_uint = bit_int<W, sign::Unsigned>;

template <typename T>
  struct is_bit_int : std::false_type {};

template <int W, sign S>
  struct is_bit_int<bit_int<W, S>> : std::true_type {};

template <typename T>
  concept bitprecise_integral = is_bit_int<T>::value;

// specialize the cases where ?: might not work (mixed sign)
template <int Wl, sign S, int Wr>
  struct std::common_type<bit_int<Wl, S>, bit_int<Wr, !S>>
  {
    using type = bit_int<std::max(Wl + S, Wr + !S)>;
  };

// Given any type, can we somehow turn it into a bit_int specialization?
template <typename T>
  struct as_bit_int;

template <typename T>
  requires requires(T a) { { +a } -> bitprecise_integral; }
  struct as_bit_int<T>
  { using type = decltype(+std::declval<T>()); };

template <typename T>
  requires (not requires(T a) { { +a } -> bitprecise_integral; })
    and requires(T a) { bit_int(a); }
  struct as_bit_int<T>
  { using type = decltype(bit_int(std::declval<T>())); };

template <typename T>
  using as_bit_int_t = as_bit_int<T>::type;

// Given two types (bit_int or convertible to bit_int), what is the bit_int with minimal width to
// store either.
template <typename T, typename U>
  using bit_int_common_type_t
    = typename std::common_type<as_bit_int_t<T>, as_bit_int_t<U>>::type;

// Given two types, is there a *third* type that needs to act as their common type?
template <typename T, typename U>
  concept bit_int_use_common_type
    = not std::same_as<T, U>
        and requires { typename as_bit_int<T>::type; typename as_bit_int<U>::type; }
        and requires { typename bit_int_common_type_t<T, U>; }
        and std::convertible_to<T, bit_int_common_type_t<T, U>>
        and std::convertible_to<U, bit_int_common_type_t<T, U>>
        and is_bit_int<bit_int_common_type_t<T, U>>::value;

template<typename T>
  concept constexpr_wrapper_like
    = std::convertible_to<T, decltype(T::value)>
        and std::equality_comparable_with<T, decltype(T::value)>
        and std::bool_constant<T() == T::value>::value
        and std::bool_constant<static_cast<decltype(T::value)>(T()) == T::value>::value;

// Implementation of comparisons that need a *third* type to use as common type.
struct bit_int_ops
{
  template <typename L, bit_int_use_common_type<L> R>
    friend constexpr bool
    operator==(const L& lhs, const R& rhs)
    {
      using C = bit_int_common_type_t<L, R>;
      return C(lhs) == C(rhs);
    }

  template <typename L, bit_int_use_common_type<L> R>
    friend constexpr std::strong_ordering
    operator<=>(const L& lhs, const R& rhs)
    {
      using C = bit_int_common_type_t<L, R>;
      return C(lhs) <=> C(rhs);
    }
};

template <int W, sign S>
  class bit_int : bit_int_ops
  {
    static_assert(S == sign::Signed ? W >= 2 : W >= 1,
                  "at least one value bit (besides a sign bit) is required");

    using value_type = std::conditional_t<S == sign::Signed, _BitInt(std::max(2, W)),
                                          unsigned _BitInt(W)>;

  public:
    static constexpr std::integral_constant<int, W> width = {};

    static constexpr std::integral_constant<sign, S> sign = {};

    value_type value;

    constexpr bit_int() = default;

    constexpr bit_int(const bit_int&) = default;

    // enable implicit/explicit conversion to bit_int<W1, S1> from types with conversion operator to
    // bit_int<W0, S0>
    template <typename T>
      requires (not bitprecise_integral<T> and not std::integral<T>
                  and not constexpr_wrapper_like<T>)
        and requires { typename as_bit_int<T>::type; }
      constexpr explicit(not std::convertible_to<as_bit_int_t<T>, bit_int>)
      bit_int(const T& x)
      : bit_int(static_cast<as_bit_int_t<T>>(x)) {}

    // initialization from integral_constant
    template <constexpr_wrapper_like T>
      consteval
      bit_int(T)
      : value(T::value)
      {
        static_assert(T(value_type(T::value)) == T::value,
                      "bit_int width too small for given constant");
      }

    // conversions from other bit_int specializations
    template <int W1>
      constexpr explicit(W1 > W)
      bit_int(bit_int<W1, S> x)
      : value(x.value) {}

    template <int W1>
      constexpr explicit(S == sign::Unsigned or W1 + 1 > W)
      bit_int(bit_int<W1, !S> x)
      : value(x.value) {}

    // conversions from arithmetic types
    template <std::signed_integral T>
      constexpr explicit(S == sign::Unsigned or sizeof(T) * CHAR_BIT > W)
      bit_int(T x)
      : value(x) {}

    template <std::unsigned_integral T>
      constexpr explicit(sizeof(T) * CHAR_BIT + !S > W)
      bit_int(T x)
      : value(x) {}

    template <std::floating_point T>
      constexpr explicit
      bit_int(T x)
      : value(x) {}

    // comparisons (see also bit_int_ops)
    friend constexpr bool
    operator==(bit_int lhs, bit_int rhs)
    { return lhs.value == rhs.value; }

    friend constexpr std::strong_ordering
    operator<=>(bit_int lhs, bit_int rhs)
    { return lhs.value <=> rhs.value; }

    // arithmetic
    friend constexpr bit_int
    operator+(bit_int a, bit_int b)
    { return a.value + b.value; }

    friend constexpr bit_int
    operator-(bit_int a, bit_int b)
    { return a.value - b.value; }

    friend constexpr bit_int
    operator*(bit_int a, bit_int b)
    { return a.value * b.value; }

    friend constexpr bit_int
    operator/(bit_int a, bit_int b)
    { return a.value / b.value; }

    friend constexpr bit_int
    operator%(bit_int a, bit_int b)
    { return a.value % b.value; }

    // bitwise
    friend constexpr bit_int
    operator|(bit_int a, bit_int b)
    { return a.value | b.value; }

    friend constexpr bit_int
    operator&(bit_int a, bit_int b)
    { return a.value & b.value; }

    friend constexpr bit_int
    operator^(bit_int a, bit_int b)
    { return a.value ^ b.value; }

    // compound assignment
    friend constexpr bit_int&
    operator+=(bit_int& a, bit_int b)
    { return a = a + b; }

    friend constexpr bit_int&
    operator-=(bit_int& a, bit_int b)
    { return a = a - b; }

    friend constexpr bit_int&
    operator*=(bit_int& a, bit_int b)
    { return a = a * b; }

    friend constexpr bit_int&
    operator/=(bit_int& a, bit_int b)
    { return a = a / b; }

    friend constexpr bit_int&
    operator%=(bit_int& a, bit_int b)
    { return a = a % b; }

    friend constexpr bit_int&
    operator|=(bit_int& a, bit_int b)
    { return a = a | b; }

    friend constexpr bit_int&
    operator&=(bit_int& a, bit_int b)
    { return a = a & b; }

    friend constexpr bit_int&
    operator^=(bit_int& a, bit_int b)
    { return a = a ^ b; }

    // unary
    friend constexpr bit_int
    operator-(bit_int a)
    { return -a.value; }

    friend constexpr bit_int
    operator+(bit_int a)
    { return +a.value; }

    friend constexpr bit_int
    operator~(bit_int a)
    { return ~a.value; }

    friend constexpr bool
    operator!(bit_int a)
    { return !a.value; }
  };

template <std::signed_integral T>
  bit_int(T) -> bit_int<sizeof(T) * CHAR_BIT>;

template <std::unsigned_integral T>
  bit_int(T) -> bit_int<sizeof(T) * CHAR_BIT, sign::Unsigned>;

template <constexpr_wrapper_like T>
  requires std::signed_integral<decltype(T::value)>
  bit_int(T)
  -> bit_int<(T::value < 0) + std::bit_width(static_cast<std::make_unsigned_t<decltype(T::value)>>(
                                               T::value < 0 ? ~T::value : T::value)),
             T::value < 0 ? sign::Signed : sign::Unsigned>;

template <constexpr_wrapper_like T>
  requires std::unsigned_integral<decltype(T::value)>
  bit_int(T)
  -> bit_int<std::bit_width(T::value), sign::Unsigned>;

// enable CTAD from types that have a conversion operator to a bit_int specialization
template <typename T>
  requires (not constexpr_wrapper_like<T>)
    and requires { typename as_bit_int<T>::type; }
  bit_int(T) -> bit_int<as_bit_int_t<T>::width(), as_bit_int_t<T>::sign()>;
