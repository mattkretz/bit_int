// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2026      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
//                       Matthias Kretz <m.kretz@gsi.de>

#include <algorithm>
#include <bit>
#include <compare>
#include <concepts>
#include <exception>
#include <limits>
#include <type_traits>

/*
 * integers with value-preserving conversions
 *
 * Design goals:
 *
 * 1. Conversions
 *  a) value-preserving conversions are implicit
 *  b) otherwise conversions are explicit and not checked
 *  c) value_preserving_cast<T>(x) checks that the value is preserved
 *  d) conversions to built-in integers are *all* explicit, even if value-preserving
 *     Rationale: Built-in integers are not value-preserving on conversion and their operators bind
 *     to implicit conversion operators.
 *     Open question: After I overloaded operator?: maybe this isn't as important anymore? Was this
 *     initially only because interconvertible types break the synthesized conditional operator? It
 *     seems like ALLOW_CONVERSION_TO_BUILTIN is not creating any issues (anymore).
 *  e) Construction via list-init implements core language-like rejection of values that cannot be
 *     converted without preserving the value. E.g. while `i8(255)` compiles, `i8{255}` is
 *     ill-formed. If the initializing value is not a constant expression, then the conversion from
 *     that type to basic_int must be value-preserving.
 *     List-init with more than one value is EB.
 *
 * 2. Overflow behavior
 *  We inherit the behavior of (un)signed _BitInt. The behavior is called out in the name of the
 *  alias templates:
 *  - `integer<W>` models ℤ (overflow is UB)
 *  - `modulo_integer<W>` models ℤ/2ᵂℤ (The implicit mod 2ᵂ of that space leads to wrap-around)
 *  We do *not* aim for an elastic type that requires additional bits for every arithmetic
 *  operation.
 *
 * 3. Common type(s)
 *  - Binary operators of mixed types T and U are evaluated in T or U: implicit conversions
 *    determine the type.
 *  - The conditional operator (and std::common_type) can resolve to a third type: a basic_int
 *    specialization that can hold all possible values of both types.
 *    Rationale: Since assignment back to a smaller basic_int is not implicit, there is little room
 *    for accidental widening. Rather, widening via operator?: is a convenience feature with little
 *    to no performance concerns.
 *  - Comparison operators of mixed types are evaluated in the std::common_type.
 *    Rationale: Since compares return bool, this is a trade-off of paying a few cycles of
 *    performance for the correct result.
 *
 * 4. Similar to a built-in integer type but not all the way
 *  - `std::is_integral_v<basic_int<...>>` is always false
 *  - `std::make_unsiged_t<basic_int<...>>` (and make_signed_t) do work
 *  - `numeric_limits` would be implemented but the goal is to only support the new numeric traits
 *  - `abs` and `<bit>` functions should get support for "small" widths
 *  - TBD after consensus on the basic_int type itself
 *
 * 5. Extensions
 *  - There could be inplace-modifier functions for making wide (large W) integers more efficient to
 *    manipulate. E.g. `integer<4096> n = ...; n.inplace_abs();`
 *
 * 6. Literals
 *  - built-in literals, especially the `wb` and `uwb` _BitInt literals are sufficient.
 *  - Adding a `template <char...> operator""something()` could be done, though.
 */

namespace std
{
  enum class _Signed
  {
    _Yes = 1,
    _No = 0
  };

  consteval _Signed
  _SignedCtor(bool __y) noexcept
  { return __y ? _Signed::_Yes : _Signed::_No; }

  constexpr bool
  operator!(_Signed __s) noexcept
  { return __s != _Signed::_Yes; }

  constexpr _Signed
  operator~(_Signed __s) noexcept
  { return !__s ? _Signed::_Yes : _Signed::_No; }

  constexpr int
  operator+(int __w, _Signed __s) noexcept
  { return __w + static_cast<int>(__s); }

  constexpr int
  operator-(int __w, _Signed __s) noexcept
  { return __w - static_cast<int>(__s); }

  //============================================================================
  //= The user-facing types/templates
  template <int _Wp, _Signed _Sp = _Signed::_Yes>
    class basic_int;

  template <int _Wp>
    using integer = basic_int<_Wp, _Signed::_Yes>;

  template <int _Wp>
    using modulo_integer = basic_int<_Wp, _Signed::_No>;

  inline namespace standard_integers
  {
    using i8  = integer<8>;
    using i16 = integer<16>;
    using i32 = integer<32>;
    using i64 = integer<64>;
    using i128 = integer<128>;

    using u8  = modulo_integer<8>;
    using u16 = modulo_integer<16>;
    using u32 = modulo_integer<32>;
    using u64 = modulo_integer<64>;
    using u128 = modulo_integer<128>;
  }

  template <typename _Tp>
    concept basic_integral = same_as<_Tp, basic_int<_Tp::width(), _Tp::_S_signed>>;

  //============================================================================
  //= _BitInt concept
  template <typename>
    constexpr bool __is_bitprecise_integral_v = false;

  template <size_t _Np>
    constexpr bool __is_bitprecise_integral_v<_BitInt(_Np)> = true;

  template <size_t _Np>
    constexpr bool __is_bitprecise_integral_v<unsigned _BitInt(_Np)> = true;

  template <typename _Tp>
    concept __bitprecise_integral = __is_bitprecise_integral_v<_Tp>;

  //============================================================================
  //= any integral language type
  template <typename _Tp>
    concept __builtin_integral = __bitprecise_integral<_Tp> || integral<_Tp>;

  //============================================================================
  //= any integral type (language and basic_int)
  template <typename _Tp>
    concept __any_integral = basic_integral<_Tp> || __builtin_integral<_Tp>;

  //============================================================================
  //= integer traits that work for any integral type
  template <__any_integral _Tp>
    inline constexpr bool __is_signed_v = signed_integral<_Tp>;

  template <size_t _Wp>
    inline constexpr bool __is_signed_v<_BitInt(_Wp)> = true;

  template <size_t _Wp>
    inline constexpr bool __is_signed_v<unsigned _BitInt(_Wp)> = false;

  template <int _Wp, _Signed _Sp>
    inline constexpr bool __is_signed_v<basic_int<_Wp, _Sp>> = !!_Sp;

  template <__any_integral _Tp>
    inline constexpr int __width_v = numeric_limits<_Tp>::digits + signed_integral<_Tp>;

  template <size_t _Wp>
    inline constexpr int __width_v<_BitInt(_Wp)> = _Wp;

  template <size_t _Wp>
    inline constexpr int __width_v<unsigned _BitInt(_Wp)> = _Wp;

  template <int _Wp, _Signed _Sp>
    inline constexpr int __width_v<basic_int<_Wp, _Sp>> = _Wp;

  template <__any_integral _Tp>
    inline constexpr int __digits_v = numeric_limits<_Tp>::digits;

  template <size_t _Wp>
    inline constexpr int __digits_v<_BitInt(_Wp)> = _Wp - 1;

  template <size_t _Wp>
    inline constexpr int __digits_v<unsigned _BitInt(_Wp)> = _Wp;

  template <int _Wp, _Signed _Sp>
    inline constexpr int __digits_v<basic_int<_Wp, _Sp>> = _Wp - _Sp;

  template <__any_integral _Tp>
    inline constexpr _Tp __max_v
      = __digits_v<_Tp> == 0
          ? _Tp() : __is_signed_v<_Tp> ? ((((_Tp(1) << cw<__digits_v<_Tp> - 1>)
                                              - _Tp(1)) << 1uwb) | _Tp(1))
                                       : ~_Tp();

  template <__any_integral _Tp>
    inline constexpr _Tp __min_v
      = __is_signed_v<_Tp> ? (_Tp(1) << cw<__digits_v<_Tp>>) : _Tp();

  //============================================================================
  //= value-preserving cast
  class bad_value_preserving_cast : public std::exception
  {
  public:
    constexpr const char*
    what() const noexcept override
    { return "the given value cannot be preserved on cast"; }
  };

  class requires_one_element : public std::exception
  {
  public:
    constexpr const char*
    what() const noexcept override
    { return "scalar basic_int requires one element in initializer"; }
  };

  template <__any_integral _To, __any_integral _From>
    constexpr _To
    value_preserving_cast(const _From& __x)
    {
      if constexpr (!__is_signed_v<_To> && __is_signed_v<_From>)
        {
          if (__x < _From())
            throw bad_value_preserving_cast();
          else if constexpr (__digits_v<_To> < __digits_v<_From>)
            {
              if (__x > __max_v<_To>)
                throw bad_value_preserving_cast();
            }
        }
      else if constexpr (__digits_v<_To> < __digits_v<_From>)
        {
          if (__x > __max_v<_To>)
            throw bad_value_preserving_cast();
          if constexpr (__is_signed_v<_To> && __is_signed_v<_From>)
            {
              if (__x < __min_v<_To>)
                throw bad_value_preserving_cast();
            }
        }
      return static_cast<_To>(__x);
    }

  template <floating_point _To, __bitprecise_integral _From>
    constexpr _To
    value_preserving_cast(const _From& __x)
    {
      if (static_cast<_From>(static_cast<_To>(__x)) != __x)
        throw bad_value_preserving_cast();
      return static_cast<_To>(__x);
    }

  template <typename _To, typename _From>
    requires is_arithmetic_v<_From> && is_arithmetic_v<_To>
                                    && (floating_point<_To> || floating_point<_From>)
    constexpr _To
    value_preserving_cast(const _From& __x)
    {
      using _Up = __conditional_t<signed_integral<_From>, make_unsigned<_From>,
                                  type_identity<_From>>::type;
      if (static_cast<_Up>(static_cast<_To>(__x)) != static_cast<_Up>(__x))
        throw bad_value_preserving_cast();
      return static_cast<_To>(__x);
    }

  //============================================================================
  //= make_(un)signed for basic_int
  template <int _Wp, _Signed _Sp>
    struct make_signed<basic_int<_Wp, _Sp>>
    { using type = basic_int<_Wp, _Signed::_Yes>; };

  template <int _Wp, _Signed _Sp>
    struct make_unsigned<basic_int<_Wp, _Sp>>
    { using type = basic_int<_Wp, _Signed::_No>; };

  //============================================================================
  // Concept for a type with implicit conversion operator to one basic_int specialization.
  // Prerequisite: basic_int must be in the associated namespaces for name lookup for _Tp.
  template <typename _Tp>
    concept __converts_to_basic_int
      = !basic_integral<_Tp> && requires(_Tp __a) {
        { auto(__as_basic_int(__a)) } -> basic_integral;
      };

  // A common_type exists, but it's neither _Tp nor _Up.
  template <typename _Tp, typename _Up>
    concept __common_type_is_other_or_not_basic
      = (!same_as<common_type_t<_Tp, _Up>, _Tp> && !same_as<common_type_t<_Tp, _Up>, _Up>)
          || !basic_integral<common_type_t<_Tp, _Up>>;

  //============================================================================
  //= ?:, ==, and <=> operators
  struct _BasicIntOps
  {
    // The built-in conditional operator covers most cases. Here we cover the cases where the common
    // type is neither _Tp nor _Up.
    template <__any_integral _Tp, __any_integral _Up>
      friend constexpr basic_int<max(__width_v<_Tp> + (!__is_signed_v<_Tp> && __is_signed_v<_Up>),
                                     __width_v<_Up> + (!__is_signed_v<_Up> && __is_signed_v<_Tp>)),
                                 _SignedCtor(__is_signed_v<_Tp> || __is_signed_v<_Up>)>
      operator?:(bool, const _Tp&, const _Up&) = default;

    // We catch the cases where _Up is convertible to basic_int and the resulting basic_int and
    // any_integral have a common_type and make ?: work. Try to stay clear of the obvious one's
    // where the resulting basic_int type is exactly _Tp: that's really the job of _Up to implement,
    // not ours. If we remove this constraint we're risking ambiguous operator?: candidates.
    template <__any_integral _Tp, __converts_to_basic_int _Up>
      requires (!same_as<_Tp, remove_cvref_t<decltype(__as_basic_int(declval<_Up>()))>>)
      friend constexpr decltype(true ? declval<_Tp>() : __as_basic_int(declval<_Up>()))
      operator?:(bool, const _Tp&, const _Up&) = default;

    template <typename _Lp, __common_type_is_other_or_not_basic<_Lp> _Rp>
      [[__gnu__::__always_inline__]]
      friend constexpr bool
      operator==(const _Lp& __lhs, const _Rp& __rhs) noexcept
      {
        static_assert(!same_as<_Lp, _Rp>);
        using _Cp = common_type_t<_Lp, _Rp>;
        if constexpr (basic_integral<_Cp>)
          return static_cast<_Cp>(__lhs)._M_value == static_cast<_Cp>(__rhs)._M_value;
        else
          return static_cast<_Cp>(__lhs) == static_cast<_Cp>(__rhs);
      }

    // You would typically expect strong_ordering as return type. However, if the common_type _Cp is
    // e.g. float, this uses partial_ordering as return type, which is why we use 'auto'.
    template <typename _Lp, __common_type_is_other_or_not_basic<_Lp> _Rp>
      [[__gnu__::__always_inline__]]
      friend constexpr auto
      operator<=>(const _Lp& __lhs, const _Rp& __rhs) noexcept
      {
        static_assert(!same_as<_Lp, _Rp>);
        using _Cp = common_type_t<_Lp, _Rp>;
        if constexpr (basic_integral<_Cp>)
          return static_cast<_Cp>(__lhs)._M_value <=> static_cast<_Cp>(__rhs)._M_value;
        else
          return static_cast<_Cp>(__lhs) <=> static_cast<_Cp>(__rhs);
      }
  };

#if __OPTIMIZE__
  [[__gnu__::__noipa__,
    __gnu__::__error__("scalar basic_int requires one element in initializer")]]
  inline int
  _S_brace_init_requires_one_element_error()
  { __builtin_trap(); }
#endif

  //============================================================================
  //= basic_int definition
  template <int _Wp, _Signed _Sp>
    class basic_int : public _BasicIntOps
    {
      static_assert(_Wp >= 1, "at least one value/sign bit is required");

      using _ValueType = __conditional_t<!_Sp, unsigned _BitInt(_Wp), _BitInt(_Wp)>;

      // TODO: This assumes two sizeof(size_t) registers can pass a single basic_int.
      using _BasicIntArg = __conditional_t<(sizeof(_ValueType) <= sizeof(size_t) * 2),
                                           basic_int, const basic_int&>;

    public:
      static constexpr _Signed _S_signed = _Sp;

      static constexpr integral_constant<int, _Wp> width = {};

      static constexpr integral_constant<bool, !!_Sp> is_signed = {};

      static constexpr integral_constant<int, _Wp - _Sp> digits = {};

      // hidden friend is found via ADL for e.g. reference_wrapper<basic_int<...>>
      [[__gnu__::__always_inline__]]
      friend constexpr const basic_int&
      __as_basic_int(const basic_int& __x) noexcept
      { return __x; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_int&
      __as_basic_int(basic_int& __x) noexcept
      { return __x; }

      // structural type
      _ValueType _M_value;

      static consteval basic_int
      max() noexcept
      { return __max_v<_ValueType>; }

      static consteval basic_int
      min() noexcept
      { return __min_v<_ValueType>; }

      constexpr basic_int() noexcept = default;

      // conversions from other basic_int specializations
      template <int _W1>
        [[__gnu__::__always_inline__]]
        constexpr explicit(_W1 > _Wp)
        basic_int(const basic_int<_W1, _Sp>& __x)
        : _M_value(__x._M_value) {}

      template <int _W1>
        [[__gnu__::__always_inline__]]
        constexpr explicit(!_Sp || basic_int<_W1, ~_Sp>::digits() > digits())
        basic_int(const basic_int<_W1, ~_Sp>& __x)
        : _M_value(__x._M_value) {}

      // enable implicit/explicit conversion to basic_int<W1, S1> from types with conversion
      // operator to basic_int<W0, S0>
      template <__converts_to_basic_int _Tp>
        requires (!__any_integral<_Tp> && !__constexpr_wrapper_like<_Tp>)
        [[__gnu__::__always_inline__]]
        constexpr explicit(!convertible_to<decltype(__as_basic_int(declval<_Tp>())), basic_int>)
        basic_int(const _Tp& __x)
        : basic_int(__as_basic_int(__x)) {}

      // initialization from _BitInt
      template <__bitprecise_integral _Tp>
        [[__gnu__::__always_inline__]]
        constexpr explicit(__digits_v<_Tp> > digits() || (_Tp(-1) < _Tp() && !is_signed()))
        basic_int(const _Tp& __x) noexcept
        : _M_value(__x)
        {}

      // initialization from integral_constant/constant_wrapper
      template <__constexpr_wrapper_like _Tp>
        consteval
        basic_int(_Tp)
        : _M_value(value_preserving_cast<_ValueType>(_Tp::value))
        {}

      // conversions from arithmetic types
      template <integral _Tp>
        [[__gnu__::__always_inline__]]
        constexpr explicit((signed_integral<_Tp> && !_Sp) || __digits_v<_Tp> > digits())
        basic_int(_Tp __x)
        : _M_value(__x) {}

      template <floating_point _Tp>
        [[__gnu__::__always_inline__]]
        constexpr explicit
        basic_int(_Tp __x)
        : _M_value(__x) {}

      // brace-init
      // EB if the initializer list holds more than one value
      //
      template <__any_integral _Tp>
        requires (__digits_v<_Tp> <= digits())
          && (!__is_signed_v<_Tp> || is_signed())
        [[__gnu__::__always_inline__]]
        constexpr
        basic_int(std::initializer_list<_Tp> __list)
        : _M_value(__list.size() == 0 ? _ValueType() : static_cast<_ValueType>(*__list.begin()))
        {
          if (__list.size() > 1u)
            {
              if consteval
                {
                  throw requires_one_element();
                }
#if __OPTIMIZE__
              else
                {
                  _S_brace_init_requires_one_element_error();
                }
#endif
            }
        }

      template <typename _Tp>
        [[__gnu__::__always_inline__]]
        consteval
        basic_int(std::initializer_list<_Tp> __list)
        : basic_int(__list.size() == 0
                      ? _ValueType()
                      : (__list.size() == 1
                           ? value_preserving_cast<_ValueType>(*__list.begin())
                           : throw(requires_one_element())))
        {}

      // conversions to language types
      template <__builtin_integral _To>
        [[__gnu__::__always_inline__]]
        constexpr explicit
#define ALLOW_CONVERSION_TO_BUILTIN 1
#if ALLOW_CONVERSION_TO_BUILTIN
        // BUG: CWG2844 and GCC PR85250
        // It appears like implicit conversions to built-in types opt in to all possible built-in
        // operators. This is implemented by Clang but not GCC.
        // CWG2844 suggests to make the GCC behavior conforming.
        (digits() > __digits_v<_To> || (is_signed() && !__is_signed_v<_To>))
#endif
        operator _To() const
        { return static_cast<_To>(_M_value); }

      template <floating_point _To>
        [[__gnu__::__always_inline__]]
        constexpr explicit(digits() > numeric_limits<_To>::digits)
        operator _To() const
        { return static_cast<_To>(_M_value); }

      // ==, <=>, and ?: are also in _BasicIntOps
      [[__gnu__::__always_inline__]]
      friend constexpr bool
      operator==(_BasicIntArg __a, _BasicIntArg __b) noexcept
      { return __a._M_value == __b._M_value; }

      [[__gnu__::__always_inline__]]
      friend constexpr strong_ordering
      operator<=>(_BasicIntArg __a, _BasicIntArg __b) noexcept
      { return __a._M_value <=> __b._M_value; }

      // no need for operator?: because the language rules already provide it

      // arithmetic
      [[__gnu__::__always_inline__]]
      friend constexpr basic_int
      operator+(_BasicIntArg __a, _BasicIntArg __b)
      { return __a._M_value + __b._M_value; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_int
      operator-(_BasicIntArg __a, _BasicIntArg __b)
      { return __a._M_value - __b._M_value; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_int
      operator*(_BasicIntArg __a, _BasicIntArg __b)
      { return __a._M_value * __b._M_value; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_int
      operator/(_BasicIntArg __a, _BasicIntArg __b)
      { return __a._M_value / __b._M_value; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_int
      operator%(_BasicIntArg __a, _BasicIntArg __b)
      { return __a._M_value % __b._M_value; }

      // bit ops
      [[__gnu__::__always_inline__]]
      friend constexpr basic_int
      operator|(_BasicIntArg __a, _BasicIntArg __b) noexcept
      { return __a._M_value | __b._M_value; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_int
      operator&(_BasicIntArg __a, _BasicIntArg __b) noexcept
      { return __a._M_value & __b._M_value; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_int
      operator^(_BasicIntArg __a, _BasicIntArg __b) noexcept
      { return __a._M_value ^ __b._M_value; }

      using _ShiftType = modulo_integer<std::bit_width(unsigned(width() - 1))>;

      [[__gnu__::__always_inline__]]
      friend constexpr basic_int
      operator<<(_BasicIntArg __a, _ShiftType __b)
      { return __a._M_value << __b._M_value; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_int
      operator>>(_BasicIntArg __a, _ShiftType __b)
      { return __a._M_value >> __b._M_value; }

      // compound assignment
      [[__gnu__::__always_inline__]]
      friend constexpr basic_int&
      operator+=(basic_int& __a, _BasicIntArg __b)
      { return __a = __a + __b; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_int&
      operator-=(basic_int& __a, _BasicIntArg __b)
      { return __a = __a - __b; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_int&
      operator*=(basic_int& __a, _BasicIntArg __b)
      { return __a = __a * __b; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_int&
      operator/=(basic_int& __a, _BasicIntArg __b)
      { return __a = __a / __b; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_int&
      operator%=(basic_int& __a, _BasicIntArg __b)
      { return __a = __a % __b; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_int&
      operator|=(basic_int& __a, _BasicIntArg __b) noexcept
      { return __a = __a | __b; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_int&
      operator&=(basic_int& __a, _BasicIntArg __b) noexcept
      { return __a = __a & __b; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_int&
      operator^=(basic_int& __a, _BasicIntArg __b) noexcept
      { return __a = __a ^ __b; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_int&
      operator<<=(basic_int& __a, _ShiftType __b)
      { return __a = __a << __b; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_int&
      operator>>=(basic_int& __a, _ShiftType __b)
      { return __a = __a >> __b; }

      // unary
      [[__gnu__::__always_inline__]]
      friend constexpr basic_int
      operator-(_BasicIntArg __a)
      { return -__a._M_value; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_int
      operator+(_BasicIntArg __a) noexcept
      { return +__a._M_value; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_int
      operator~(_BasicIntArg __a) noexcept
      { return ~__a._M_value; }

      [[__gnu__::__always_inline__]]
      friend constexpr bool
      operator!(_BasicIntArg __a) noexcept
      { return !__a._M_value; }
    };

  //============================================================================
  //= deduction guides
  template <signed_integral _Tp>
    basic_int(_Tp) -> basic_int<numeric_limits<_Tp>::digits + 1, _Signed::_Yes>;

  template <unsigned_integral _Tp>
    basic_int(_Tp) -> basic_int<numeric_limits<_Tp>::digits, _Signed::_No>;

  template <__bitprecise_integral _Tp>
    basic_int(_Tp) -> basic_int<__width_v<_Tp>, _SignedCtor(_Tp(-1) < _Tp())>;

  template <__constexpr_wrapper_like _Tp>
    requires signed_integral<decltype(_Tp::value)>
    basic_int(_Tp)
    -> basic_int<(_Tp::value < 0) + bit_width(static_cast<make_unsigned_t<decltype(_Tp::value)>>(
                                                _Tp::value < 0 ? ~_Tp::value : _Tp::value)),
                 _SignedCtor(_Tp::value < 0)>;

  template <__constexpr_wrapper_like _Tp>
    requires unsigned_integral<decltype(_Tp::value)>
    basic_int(_Tp)
    -> basic_int<bit_width(_Tp::value), _Signed::_No>;

  // enable CTAD from types that have a conversion operator to a basic_int specialization
  template <typename _Tp>
    using __as_basic_int_t = remove_cvref_t<decltype(__as_basic_int(declval<_Tp>()))>;

  template <__converts_to_basic_int _Tp>
    requires (!__constexpr_wrapper_like<_Tp>)
    basic_int(_Tp) -> basic_int<__as_basic_int_t<_Tp>::width(), __as_basic_int_t<_Tp>::_S_signed>;
}
