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

#include <list>
#include <type_traits>
#include <vector>

#include <range/v3/core.hpp>
#include <range/v3/view/all.hpp>
#include <range/v3/view/owning.hpp>

#include "../simple_test.hpp"
#include "../test_utils.hpp"

int main()
{
    using namespace ranges;

    // An owning_view of a container preserves the container's traversal
    // and sized-ness, but is itself a (move-only) view.
    {
        using OV = owning_view<std::vector<int>>;
        CPP_assert(view_<OV>);
        CPP_assert(random_access_range<OV>);
        CPP_assert(sized_range<OV>);
        CPP_assert(contiguous_range<OV>);
        // An owning_view owns its range, so it is not a borrowed_range.
        CPP_assert(!enable_borrowed_range<OV>);
        // It is move-only.
        CPP_assert(!std::is_copy_constructible<OV>::value);
        CPP_assert(!std::is_copy_assignable<OV>::value);
        CPP_assert(move_constructible<OV>);
        CPP_assert(assignable_from<OV &, OV>);
    }

    // views::all of an rvalue container yields an owning_view that keeps the
    // container alive for as long as the view exists.
    {
        auto ov = views::all(std::vector<int>{1, 1, 1, 2, 3, 4, 4});
        CPP_assert(same_as<decltype(ov), owning_view<std::vector<int>>>);
        CPP_assert(same_as<views::all_t<std::vector<int>>,
                           owning_view<std::vector<int>>>);
        CHECK(ov.size() == 7u);
        CHECK(!ov.empty());
        CHECK(*ov.begin() == 1);
        CHECK(ov.data() == detail::addressof(*ov.begin()));
        check_equal(ov, {1, 1, 1, 2, 3, 4, 4});
    }

    // base() hands back a reference to the owned range with the value category
    // and const-ness of the owning_view.
    {
        owning_view<std::vector<int>> ov{std::vector<int>{4, 5, 6}};
        owning_view<std::vector<int>> const & cov = ov;
        CPP_assert(same_as<decltype(ov.base()), std::vector<int> &>);
        CPP_assert(same_as<decltype(cov.base()), std::vector<int> const &>);
        CPP_assert(same_as<decltype(std::move(ov).base()), std::vector<int> &&>);
        CPP_assert(
            same_as<decltype(std::move(cov).base()), std::vector<int> const &&>);
        CHECK(ov.base().size() == 3u);
        // Moving out of base() leaves the owned range moved-from.
        std::vector<int> stolen = std::move(ov).base();
        CHECK(stolen.size() == 3u);
    }

    // owning_view is default-constructible when its range is, and passing an
    // owning_view through views::all a second time is a no-op decay-copy.
    {
        owning_view<std::vector<int>> ov{};
        CHECK(ov.empty());
    }

    // A non-contiguous container is owned just the same, preserving its
    // (bidirectional) category and sized-ness.
    {
        auto ov = views::all(std::list<int>{1, 2, 3, 4});
        CPP_assert(same_as<decltype(ov), owning_view<std::list<int>>>);
        CPP_assert(bidirectional_range<decltype(ov)>);
        CPP_assert(!random_access_range<decltype(ov)>);
        CPP_assert(sized_range<decltype(ov)>);
        CHECK(ov.size() == 4u);
        check_equal(ov, {1, 2, 3, 4});
    }

#if RANGES_CXX_DEDUCTION_GUIDES >= RANGES_CXX_DEDUCTION_GUIDES_17
    // Class template argument deduction picks the (decayed) owned range type.
    {
        owning_view ov{std::vector<int>{7, 8, 9}};
        CPP_assert(same_as<decltype(ov), owning_view<std::vector<int>>>);
        CHECK(ov.size() == 3u);
    }
#endif

    return test_result();
}
