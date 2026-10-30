/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2013-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef RANGES_V3_UTILITY_COMMON_TYPE_HPP
#define RANGES_V3_UTILITY_COMMON_TYPE_HPP

#include <tuple>
#include <utility>

#include <meta/meta.hpp>

#include <concepts/type_traits.hpp>

#include <range/v3/range_fwd.hpp>

#include <range/v3/detail/prologue.hpp>

// Since C++23 (P2321), `std::tuple` and `std::pair` have everything that the
// reference types of `views::zip` need and that used to require the separate
// types `common_tuple` and `common_pair`: assignment through `const` objects
// and specializations of `std::basic_common_reference` and `std::common_type`.
// Also since C++23 (P2165), `std::apply` and the other tuple utilities of the
// standard library only accept its own tuple-like types. So from then on,
// `common_tuple` and `common_pair` are aliases for `std::tuple` and `std::pair`.
#if defined(__cpp_lib_ranges_zip) && __cpp_lib_ranges_zip >= 202110L
#define RANGES_COMMON_TUPLE_IS_STD_TUPLE 1
#else
#define RANGES_COMMON_TUPLE_IS_STD_TUPLE 0
#endif

// Sadly, this is necessary because of:
//  - std::common_type is !SFINAE-friendly, and
//  - The specification of std::common_type makes it impossibly
//    difficult to specialize on user-defined types without spamming
//    out a bajillion copies to handle all combinations of cv and ref
//    qualifiers.

namespace ranges
{
    template<typename... Ts>
    using common_type = concepts::common_type<Ts...>;

    template<typename... Ts>
    using common_type_t = concepts::common_type_t<Ts...>;

    template<typename... Ts>
    using common_reference = concepts::common_reference<Ts...>;

    template<typename... Ts>
    using common_reference_t = concepts::common_reference_t<Ts...>;

#if RANGES_COMMON_TUPLE_IS_STD_TUPLE
    template<typename F, typename S>
    using common_pair = std::pair<F, S>;

    template<typename... Ts>
    using common_tuple = std::tuple<Ts...>;
#else
    /// \cond
    template<typename F, typename S>
    struct common_pair;

    template<typename... Ts>
    struct common_tuple;
    /// \endcond
#endif
} // namespace ranges

/// \cond
// Specializations for pair and tuple
namespace concepts
{
    // With `RANGES_COMMON_TUPLE_IS_STD_TUPLE`, the specializations for
    // `common_pair`/`common_tuple` and `std::pair`/`std::tuple` are the same as
    // the ones for two `common_pair`s/`common_tuple`s, so they are omitted.
#if !RANGES_COMMON_TUPLE_IS_STD_TUPLE
    // common_type for std::pairs
    template<typename F1, typename S1, typename F2, typename S2>
    struct common_type<std::pair<F1, S1>, ::ranges::common_pair<F2, S2>>;

    template<typename F1, typename S1, typename F2, typename S2>
    struct common_type<::ranges::common_pair<F1, S1>, std::pair<F2, S2>>;
#endif

    template<typename F1, typename S1, typename F2, typename S2>
    struct common_type<::ranges::common_pair<F1, S1>, ::ranges::common_pair<F2, S2>>;

#if !RANGES_COMMON_TUPLE_IS_STD_TUPLE
    // common_type for std::tuples
    template<typename... Ts, typename... Us>
    struct common_type<::ranges::common_tuple<Ts...>, std::tuple<Us...>>;

    template<typename... Ts, typename... Us>
    struct common_type<std::tuple<Ts...>, ::ranges::common_tuple<Us...>>;
#endif

    template<typename... Ts, typename... Us>
    struct common_type<::ranges::common_tuple<Ts...>, ::ranges::common_tuple<Us...>>;

#if !RANGES_COMMON_TUPLE_IS_STD_TUPLE
    // A common reference for std::pairs
    template<typename F1, typename S1, typename F2, typename S2,
             template<typename> class Qual1, template<typename> class Qual2>
    struct basic_common_reference<::ranges::common_pair<F1, S1>, std::pair<F2, S2>, Qual1, Qual2>;

    template<typename F1, typename S1, typename F2, typename S2,
             template<typename> class Qual1, template<typename> class Qual2>
    struct basic_common_reference<std::pair<F1, S1>, ::ranges::common_pair<F2, S2>, Qual1, Qual2>;
#endif

    template<typename F1, typename S1, typename F2, typename S2,
             template<typename> class Qual1, template<typename> class Qual2>
    struct basic_common_reference<::ranges::common_pair<F1, S1>, ::ranges::common_pair<F2, S2>, Qual1, Qual2>;

#if !RANGES_COMMON_TUPLE_IS_STD_TUPLE
    // A common reference for std::tuples
    template<typename... Ts, typename... Us, template<typename> class Qual1,
             template<typename> class Qual2>
    struct basic_common_reference<::ranges::common_tuple<Ts...>, std::tuple<Us...>, Qual1, Qual2>;

    template<typename... Ts, typename... Us, template<typename> class Qual1,
             template<typename> class Qual2>
    struct basic_common_reference<std::tuple<Ts...>, ::ranges::common_tuple<Us...>, Qual1, Qual2>;
#endif

    template<typename... Ts, typename... Us, template<typename> class Qual1,
             template<typename> class Qual2>
    struct basic_common_reference<::ranges::common_tuple<Ts...>, ::ranges::common_tuple<Us...>, Qual1, Qual2>;
} // namespace concepts

// With `RANGES_COMMON_TUPLE_IS_STD_TUPLE`, the standard library itself has these
// specializations for `std::pair` and `std::tuple`.
#if RANGES_CXX_VER > RANGES_CXX_STD_17 && !RANGES_COMMON_TUPLE_IS_STD_TUPLE
RANGES_DIAGNOSTIC_PUSH
RANGES_DIAGNOSTIC_IGNORE_MISMATCHED_TAGS
RANGES_BEGIN_NAMESPACE_STD
RANGES_BEGIN_NAMESPACE_VERSION

    template<typename...>
    struct common_type;

    // common_type for std::pairs
    template<typename F1, typename S1, typename F2, typename S2>
    struct common_type<std::pair<F1, S1>, ::ranges::common_pair<F2, S2>>;

    template<typename F1, typename S1, typename F2, typename S2>
    struct common_type<::ranges::common_pair<F1, S1>, std::pair<F2, S2>>;

    template<typename F1, typename S1, typename F2, typename S2>
    struct common_type<::ranges::common_pair<F1, S1>, ::ranges::common_pair<F2, S2>>;

    // common_type for std::tuples
    template<typename... Ts, typename... Us>
    struct common_type<::ranges::common_tuple<Ts...>, std::tuple<Us...>>;

    template<typename... Ts, typename... Us>
    struct common_type<std::tuple<Ts...>, ::ranges::common_tuple<Us...>>;

    template<typename... Ts, typename... Us>
    struct common_type<::ranges::common_tuple<Ts...>, ::ranges::common_tuple<Us...>>;

    template<typename, typename, template<typename> class, template<typename> class>
    struct basic_common_reference;

    // A common reference for std::pairs
    template<typename F1, typename S1, typename F2, typename S2,
             template<typename> class Qual1, template<typename> class Qual2>
    struct basic_common_reference<::ranges::common_pair<F1, S1>, std::pair<F2, S2>, Qual1, Qual2>;

    template<typename F1, typename S1, typename F2, typename S2,
             template<typename> class Qual1, template<typename> class Qual2>
    struct basic_common_reference<std::pair<F1, S1>, ::ranges::common_pair<F2, S2>, Qual1, Qual2>;

    template<typename F1, typename S1, typename F2, typename S2,
             template<typename> class Qual1, template<typename> class Qual2>
    struct basic_common_reference<::ranges::common_pair<F1, S1>, ::ranges::common_pair<F2, S2>, Qual1, Qual2>;

    // A common reference for std::tuples
    template<typename... Ts, typename... Us, template<typename> class Qual1,
             template<typename> class Qual2>
    struct basic_common_reference<::ranges::common_tuple<Ts...>, std::tuple<Us...>, Qual1, Qual2>;

    template<typename... Ts, typename... Us, template<typename> class Qual1,
             template<typename> class Qual2>
    struct basic_common_reference<std::tuple<Ts...>, ::ranges::common_tuple<Us...>, Qual1, Qual2>;

    template<typename... Ts, typename... Us, template<typename> class Qual1,
             template<typename> class Qual2>
    struct basic_common_reference<::ranges::common_tuple<Ts...>, ::ranges::common_tuple<Us...>, Qual1, Qual2>;

RANGES_END_NAMESPACE_VERSION
RANGES_END_NAMESPACE_STD
RANGES_DIAGNOSTIC_POP
#endif // RANGES_CXX_VER > RANGES_CXX_STD_17 && !RANGES_COMMON_TUPLE_IS_STD_TUPLE
/// \endcond

#include <range/v3/detail/epilogue.hpp>

#endif
