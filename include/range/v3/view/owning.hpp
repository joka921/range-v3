/// \file
// Range v3 library
//
//  Copyright Johannes Kalmbach 2026-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef RANGES_V3_VIEW_OWNING_HPP
#define RANGES_V3_VIEW_OWNING_HPP

#include <type_traits>
#include <utility>

#include <concepts/concepts.hpp>

#include <range/v3/range_fwd.hpp>

#include <range/v3/range/access.hpp>
#include <range/v3/range/concepts.hpp>
#include <range/v3/range/primitives.hpp>
#include <range/v3/range/traits.hpp>
#include <range/v3/utility/move.hpp>
#include <range/v3/view/interface.hpp>

#include <range/v3/detail/prologue.hpp>

namespace ranges
{
    /// \addtogroup group-views
    /// @{

    /// \cond
    namespace detail
    {
        // A wrapper around a `T` that conditionally value-initializes it.
        // `owning_view` uses this for its exposition-only member: its default
        // constructor is unconditionally `= default` (rather than constrained
        // on `default_initializable<Rng>` as in C++20, which cannot be
        // expressed on a defaulted constructor in this dialect). A plain
        // `Rng rng_ = Rng();` member would therefore force `Rng` to be
        // default-constructible even when `owning_view` is never default
        // constructed. `value_init_holder` gives the member the value-
        // initializing `= T()` initializer only when `T` is default-
        // constructible, and a plain `T t;` otherwise. As a result
        // `owning_view` value-initializes its underlying range for
        // default-constructible `Rng` (matching the C++20 `V r_ = V();`
        // behavior), and is simply *not* default-constructible for
        // non-default-constructible, move-only ranges (instead of a hard
        // error).
        template<typename T, typename = void>
        struct value_init_holder
        {
            T t;
        };
        template<typename T>
        struct value_init_holder<
            T, std::enable_if_t<std::is_default_constructible<T>::value>>
        {
            T t = T();
        };
    } // namespace detail
    /// \endcond

    template<typename Rng>
    struct owning_view : view_interface<owning_view<Rng>, range_cardinality<Rng>::value>
    {
    private:
        CPP_assert(range<Rng>);
        CPP_assert(movable<Rng>);
        static_assert(std::is_object<Rng>::value, "");
        static_assert(!detail::is_initializer_list_<Rng>,
                      "The template parameter of owning_view may not be an "
                      "initializer_list");
        detail::value_init_holder<Rng> rng_; // exposition only
    public:
        // `owning_view()` is unconditionally defaulted. Because `rng_` is a
        // `value_init_holder` (see above), this default constructor value-
        // initializes the underlying range when `Rng` is default-constructible
        // (matching the C++20 `V r_ = V();` behavior), and is implicitly
        // defined as deleted when `Rng` is not default-constructible (rather
        // than being a hard error, so move-only ranges can still be wrapped).
        owning_view() = default;
        constexpr owning_view(Rng && rng) //
            noexcept(std::is_nothrow_move_constructible<Rng>::value)
          : rng_{detail::move(rng)}
        {}
        owning_view(owning_view &&) = default;
        owning_view & operator=(owning_view &&) = default;

        constexpr Rng & base() & noexcept
        {
            return rng_.t;
        }
        constexpr Rng const & base() const & noexcept
        {
            return rng_.t;
        }
        constexpr Rng && base() && noexcept
        {
            return detail::move(rng_.t);
        }
        constexpr Rng const && base() const && noexcept
        {
            return detail::move(rng_.t);
        }

        constexpr iterator_t<Rng> begin() noexcept(noexcept(ranges::begin(rng_.t)))
        {
            return ranges::begin(rng_.t);
        }
        constexpr sentinel_t<Rng> end() noexcept(noexcept(ranges::end(rng_.t)))
        {
            return ranges::end(rng_.t);
        }
        CPP_auto_member
        constexpr auto CPP_fun(begin)()(const //
            noexcept(noexcept(ranges::begin(rng_.t))) //
            requires range<Rng const>)
        {
            return ranges::begin(rng_.t);
        }
        CPP_auto_member
        constexpr auto CPP_fun(end)()(const //
            noexcept(noexcept(ranges::end(rng_.t))) //
            requires range<Rng const>)
        {
            return ranges::end(rng_.t);
        }
        CPP_member
        constexpr auto empty() noexcept(noexcept(ranges::empty(rng_.t)))
            -> CPP_ret(bool)(
                requires detail::can_empty_<Rng>)
        {
            return ranges::empty(rng_.t);
        }
        CPP_member
        constexpr auto empty() const noexcept(noexcept(ranges::empty(rng_.t)))
            -> CPP_ret(bool)(
                requires detail::can_empty_<Rng const>)
        {
            return ranges::empty(rng_.t);
        }
        CPP_auto_member
        constexpr auto CPP_fun(size)()(
            noexcept(noexcept(ranges::size(rng_.t))) //
            requires sized_range<Rng>)
        {
            return ranges::size(rng_.t);
        }
        CPP_auto_member
        constexpr auto CPP_fun(size)()(const //
            noexcept(noexcept(ranges::size(rng_.t))) //
            requires sized_range<Rng const>)
        {
            return ranges::size(rng_.t);
        }
        CPP_auto_member
        constexpr auto CPP_fun(data)()(
            noexcept(noexcept(ranges::data(rng_.t))) //
            requires contiguous_range<Rng>)
        {
            return ranges::data(rng_.t);
        }
        CPP_auto_member
        constexpr auto CPP_fun(data)()(const //
            noexcept(noexcept(ranges::data(rng_.t))) //
            requires contiguous_range<Rng const>)
        {
            return ranges::data(rng_.t);
        }
    };

#if RANGES_CXX_DEDUCTION_GUIDES >= RANGES_CXX_DEDUCTION_GUIDES_17
    template(typename Rng)(
        requires range<Rng>)
    owning_view(Rng &&) //
        -> owning_view<detail::decay_t<Rng>>;
#endif

    namespace cpp20
    {
        template(typename Rng)(
            requires range<Rng> AND movable<Rng> AND
                (!detail::is_initializer_list_<Rng>)) //
            using owning_view = ranges::owning_view<Rng>;
    }
} // namespace ranges

#include <range/v3/detail/satisfy_boost_range.hpp>
RANGES_SATISFY_BOOST_RANGE(::ranges::owning_view)

#include <range/v3/detail/epilogue.hpp>

#endif
