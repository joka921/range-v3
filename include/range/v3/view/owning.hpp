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
        Rng rng_ = Rng(); // exposition only
    public:
        owning_view() = default;
        constexpr owning_view(Rng && rng) //
            noexcept(std::is_nothrow_move_constructible<Rng>::value)
          : rng_(detail::move(rng))
        {}
        owning_view(owning_view &&) = default;
        owning_view & operator=(owning_view &&) = default;

        constexpr Rng & base() & noexcept
        {
            return rng_;
        }
        constexpr Rng const & base() const & noexcept
        {
            return rng_;
        }
        constexpr Rng && base() && noexcept
        {
            return detail::move(rng_);
        }
        constexpr Rng const && base() const && noexcept
        {
            return detail::move(rng_);
        }

        constexpr iterator_t<Rng> begin() noexcept(noexcept(ranges::begin(rng_)))
        {
            return ranges::begin(rng_);
        }
        constexpr sentinel_t<Rng> end() noexcept(noexcept(ranges::end(rng_)))
        {
            return ranges::end(rng_);
        }
        CPP_auto_member
        constexpr auto CPP_fun(begin)()(const //
            noexcept(noexcept(ranges::begin(rng_))) //
            requires range<Rng const>)
        {
            return ranges::begin(rng_);
        }
        CPP_auto_member
        constexpr auto CPP_fun(end)()(const //
            noexcept(noexcept(ranges::end(rng_))) //
            requires range<Rng const>)
        {
            return ranges::end(rng_);
        }
        CPP_member
        constexpr auto empty() noexcept(noexcept(ranges::empty(rng_)))
            -> CPP_ret(bool)(
                requires detail::can_empty_<Rng>)
        {
            return ranges::empty(rng_);
        }
        CPP_member
        constexpr auto empty() const noexcept(noexcept(ranges::empty(rng_)))
            -> CPP_ret(bool)(
                requires detail::can_empty_<Rng const>)
        {
            return ranges::empty(rng_);
        }
        CPP_auto_member
        constexpr auto CPP_fun(size)()(
            noexcept(noexcept(ranges::size(rng_))) //
            requires sized_range<Rng>)
        {
            return ranges::size(rng_);
        }
        CPP_auto_member
        constexpr auto CPP_fun(size)()(const //
            noexcept(noexcept(ranges::size(rng_))) //
            requires sized_range<Rng const>)
        {
            return ranges::size(rng_);
        }
        CPP_auto_member
        constexpr auto CPP_fun(data)()(
            noexcept(noexcept(ranges::data(rng_))) //
            requires contiguous_range<Rng>)
        {
            return ranges::data(rng_);
        }
        CPP_auto_member
        constexpr auto CPP_fun(data)()(const //
            noexcept(noexcept(ranges::data(rng_))) //
            requires contiguous_range<Rng const>)
        {
            return ranges::data(rng_);
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
