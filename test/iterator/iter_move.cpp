// Range v3 library
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#include <cstddef>
#include <iterator>
#include <vector>

#include <range/v3/iterator/access.hpp>
#include <range/v3/iterator/concepts.hpp>
#include <range/v3/view/chunk.hpp>
#include <range/v3/view/subrange.hpp>

#include "../simple_test.hpp"
#include "../test_utils.hpp"

using namespace ranges;

// A proxy reference that can neither be copied nor moved, like the row
// references of QLever's `IdTable` with libc++.
struct non_movable_ref
{
    int * p_;
    explicit non_movable_ref(int * p)
      : p_{p}
    {}
    non_movable_ref(non_movable_ref const &) = delete;
    operator int() const
    {
        return *p_;
    }
};

// An iterator whose `operator*` returns a `non_movable_ref` prvalue.
struct non_movable_ref_iterator
{
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    int * p_ = nullptr;
    non_movable_ref operator*() const
    {
        return non_movable_ref{p_};
    }
    non_movable_ref_iterator & operator++()
    {
        ++p_;
        return *this;
    }
    non_movable_ref_iterator operator++(int)
    {
        auto tmp = *this;
        ++p_;
        return tmp;
    }
    friend bool operator==(non_movable_ref_iterator a, non_movable_ref_iterator b)
    {
        return a.p_ == b.p_;
    }
    friend bool operator!=(non_movable_ref_iterator a, non_movable_ref_iterator b)
    {
        return a.p_ != b.p_;
    }
};

int main()
{
    // Like `std::ranges::iter_move`, `ranges::iter_move` returns a prvalue `*i`
    // directly, so it doesn't need to move it into a new object.
    using I = non_movable_ref_iterator;
    CPP_assert(same_as<iter_rvalue_reference_t<I>, non_movable_ref>);
    CPP_assert(indirectly_readable<I>);
    CPP_assert(forward_iterator<I>);

    int a[] = {1, 2, 3, 4, 5};
    I it{a};
    CHECK(static_cast<int>(iter_move(it)) == 1);

    // Views that require `indirectly_readable` iterators work with such ranges.
    auto rng = make_subrange(I{a}, I{a + 5});
    std::vector<int> sums;
    for(auto && chunk : rng | views::chunk(2))
    {
        int sum = 0;
        for(auto && x : chunk)
            sum += static_cast<int>(x);
        sums.push_back(sum);
    }
    ::check_equal(sums, {3, 7, 5});

    // An lvalue `*i` is still moved from.
    CPP_assert(same_as<iter_rvalue_reference_t<std::vector<int>::iterator>, int &&>);

    return ::test_result();
}
