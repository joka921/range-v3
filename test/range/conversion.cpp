// Range v3 library
//
//  Copyright Eric Niebler 2014-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <range/v3/action/sort.hpp>
#include <range/v3/core.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/indices.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/zip.hpp>
#include <range/v3/view/reverse.hpp>
#include <range/v3/view/take_while.hpp>

#include "../simple_test.hpp"
#include "../test_utils.hpp"

template<typename T>
struct vector_like
{
private:
    std::vector<T> data_;
public:
    using size_type = std::size_t;
    using allocator_type = std::allocator<T>;

    vector_like() = default;
    template<typename I>
    vector_like(I first, I last)
      : data_(first, last)
    {}
    template<typename I>
    void assign(I first, I last)
    {
        data_.assign(first, last);
    }

    auto begin()
    {
        return data_.begin();
    }
    auto end()
    {
        return data_.end();
    }
    auto begin() const
    {
        return data_.begin();
    }
    auto end() const
    {
        return data_.end();
    }
    size_type size() const
    {
        return data_.size();
    }
    size_type capacity() const
    {
        return data_.capacity();
    }
    size_type max_size() const
    {
        return data_.max_size();
    }
    auto& operator[](size_type n)
    {
        return data_[n];
    }
    auto& operator[](size_type n) const
    {
        return data_[n];
    }

    size_type last_reservation{};
    size_type reservation_count{};

    void reserve(size_type n)
    {
        data_.reserve(n);
        last_reservation = n;
        ++reservation_count;
    }
};

#if RANGES_CXX_DEDUCTION_GUIDES >= RANGES_CXX_DEDUCTION_GUIDES_17
template<typename I>
vector_like(I, I) -> vector_like<ranges::iter_value_t<I>>;

template<typename Rng, typename CI = ranges::range_common_iterator_t<Rng>,
         typename = decltype(std::map{CI{}, CI{}})>
void test_zip_to_map(Rng && rng, int)
{
    using namespace ranges;
#ifdef RANGES_WORKAROUND_MSVC_779708
    auto m = static_cast<Rng &&>(rng) | to<std::map>();
#else  // ^^^ workaround / no workaround vvv
    auto m = static_cast<Rng &&>(rng) | to<std::map>;
#endif // RANGES_WORKAROUND_MSVC_779708
    CPP_assert(same_as<decltype(m), std::map<int, int>>);
}
#endif
template<typename Rng>
void test_zip_to_map(Rng &&, long)
{}

template<typename K, typename V>
struct map_like : std::map<K, V>
{
    template<typename Iter>
    map_like(Iter f, Iter l)
      : std::map<K, V>(f, l)
    {}
};

#if RANGES_CXX_DEDUCTION_GUIDES >= RANGES_CXX_DEDUCTION_GUIDES_17
template<typename Iter>
map_like(Iter, Iter) -> map_like<typename ranges::iter_value_t<Iter>::first_type,
                                 typename ranges::iter_value_t<Iter>::second_type>;
#endif


// A stateful allocator that is used to test that additional arguments are
// passed through by `ranges::to`.
template<typename T>
struct tagged_allocator
{
    using value_type = T;
    int tag = 0;

    tagged_allocator() = default;
    explicit tagged_allocator(int t)
      : tag(t)
    {}
    template<typename U>
    tagged_allocator(tagged_allocator<U> const & other)
      : tag(other.tag)
    {}
    T * allocate(std::size_t n)
    {
        return std::allocator<T>{}.allocate(n);
    }
    void deallocate(T * p, std::size_t n)
    {
        std::allocator<T>{}.deallocate(p, n);
    }
    template<typename U>
    friend bool operator==(tagged_allocator const & a, tagged_allocator<U> const & b)
    {
        return a.tag == b.tag;
    }
    template<typename U>
    friend bool operator!=(tagged_allocator const & a, tagged_allocator<U> const & b)
    {
        return a.tag != b.tag;
    }
};

// A container that can neither be constructed from a range nor from an
// iterator pair, but only appended to via `push_back`. Also stores the
// additional constructor argument.
struct push_back_only
{
    using value_type = int;
    std::vector<int> data_;
    int extra = 0;
    std::size_t reservation_count = 0;

    push_back_only() = default;
    explicit push_back_only(int e)
      : extra(e)
    {}
    void push_back(int i)
    {
        data_.push_back(i);
    }
    std::vector<int>::const_iterator begin() const
    {
        return data_.begin();
    }
    std::vector<int>::const_iterator end() const
    {
        return data_.end();
    }
    std::size_t size() const
    {
        return data_.size();
    }
    std::size_t capacity() const
    {
        return data_.capacity();
    }
    std::size_t max_size() const
    {
        return data_.max_size();
    }
    void reserve(std::size_t n)
    {
        ++reservation_count;
        data_.reserve(n);
    }
};

// Like `push_back_only`, but only supports `insert(end, value)` and doesn't
// support `reserve`.
struct insert_only
{
    using value_type = int;
    std::list<int> data_;

    std::list<int>::const_iterator begin() const
    {
        return data_.begin();
    }
    std::list<int>::const_iterator end() const
    {
        return data_.end();
    }
    std::list<int>::iterator insert(std::list<int>::const_iterator pos, int i)
    {
        return data_.insert(pos, i);
    }
};

// A (non-range) type that can be directly constructed from a range and an
// additional argument. `ranges::to` must prefer this constructor.
struct sum_with_offset
{
    int sum = 0;
    template<typename Rng,
             typename = std::enable_if_t<(bool)ranges::input_range<Rng>>>
    sum_with_offset(Rng && rng, int offset)
      : sum(offset)
    {
        for(auto && i : rng)
            sum += i;
    }
};

// A range type that is directly constructible from any range, but also has
// an iterator-pair constructor. `ranges::to` must prefer the direct
// constructor.
struct direct_and_iterators
{
    std::vector<int> data_;
    bool used_direct = false;

    template<typename Rng,
             typename = std::enable_if_t<(bool)ranges::input_range<Rng>>>
    explicit direct_and_iterators(Rng && rng)
      : data_(ranges::begin(rng), ranges::end(rng))
      , used_direct(true)
    {}
    template<typename I>
    direct_and_iterators(I first, I last)
      : data_(first, last)
    {}
    std::vector<int>::const_iterator begin() const
    {
        return data_.begin();
    }
    std::vector<int>::const_iterator end() const
    {
        return data_.end();
    }
};

void test_additional_arguments()
{
    using namespace ranges;
    using alloc_t = tagged_allocator<int>;
    using vec_t = std::vector<int, alloc_t>;
    auto rng = views::iota(0, 5);

    // Direct call with an explicit container type.
    {
        auto v = ranges::to<vec_t>(rng, alloc_t{42});
        CPP_assert(same_as<decltype(v), vec_t>);
        CHECK(v.get_allocator().tag == 42);
        check_equal(v, {0, 1, 2, 3, 4});
    }

    // Pipe syntax with an explicit container type.
    {
        auto v = rng | ranges::to<vec_t>(alloc_t{43});
        CPP_assert(same_as<decltype(v), vec_t>);
        CHECK(v.get_allocator().tag == 43);
        check_equal(v, {0, 1, 2, 3, 4});
    }

    // The closure can be stored and reused (as lvalue and rvalue).
    {
        alloc_t const alloc{44};
        auto closure = ranges::to<vec_t>(alloc);
        auto v1 = rng | closure;
        auto v2 = closure(rng);
        auto v3 = std::move(closure)(rng);
        CPP_assert(same_as<decltype(v1), vec_t>);
        CPP_assert(same_as<decltype(v2), vec_t>);
        CPP_assert(same_as<decltype(v3), vec_t>);
        CHECK(v1.get_allocator().tag == 44);
        CHECK(v2.get_allocator().tag == 44);
        CHECK(v3.get_allocator().tag == 44);
        check_equal(v1, {0, 1, 2, 3, 4});
        check_equal(v2, {0, 1, 2, 3, 4});
        check_equal(v3, {0, 1, 2, 3, 4});
    }

    // The closure can be composed with other pipeables.
    {
        auto v = views::iota(0, 5) | views::reverse |
                 (ranges::to<vec_t>(alloc_t{45}) | actions::sort);
        CPP_assert(same_as<decltype(v), vec_t>);
        CHECK(v.get_allocator().tag == 45);
        check_equal(v, {0, 1, 2, 3, 4});
    }

    // Non-common and non-sized ranges.
    {
        auto r = views::iota(0) | views::take_while([](int i) { return i < 5; });
        CPP_assert(!common_range<decltype(r)>);
        auto v = ranges::to<vec_t>(r, alloc_t{46});
        CHECK(v.get_allocator().tag == 46);
        check_equal(v, {0, 1, 2, 3, 4});
        auto l = r | ranges::to<std::list<int, alloc_t>>(alloc_t{47});
        CHECK(l.get_allocator().tag == 47);
        check_equal(l, {0, 1, 2, 3, 4});
    }

    // Additional arguments that are not allocators, e.g. a comparator.
    {
        auto cmp = std::greater<int>{};
        auto s = ranges::to<std::set<int, std::greater<int>>>(rng, cmp);
        CPP_assert(same_as<decltype(s), std::set<int, std::greater<int>>>);
        check_equal(s, {4, 3, 2, 1, 0});
        auto m = views::zip(rng, rng) |
                 ranges::to<std::map<int, int, std::greater<int>>>(cmp);
        CPP_assert(same_as<decltype(m), std::map<int, int, std::greater<int>>>);
        CHECK(m.begin()->first == 4);
        CHECK(m.size() == 5u);
    }

    // Containers of containers: The additional arguments are only passed to
    // the outer container (like for `std::ranges::to`).
    {
        using outer_alloc_t = tagged_allocator<std::vector<int>>;
        auto r = views::ints(1, 4) |
                 views::transform([](int i) { return views::ints(i, i + 3); });
        auto m = ranges::to<std::vector<std::vector<int>, outer_alloc_t>>(
            r, outer_alloc_t{48});
        CPP_assert(same_as<decltype(m), std::vector<std::vector<int>, outer_alloc_t>>);
        CHECK(m.get_allocator().tag == 48);
        CHECK(m.size() == 3u);
        check_equal(m[0], {1, 2, 3});
        check_equal(m[1], {2, 3, 4});
        check_equal(m[2], {3, 4, 5});

        auto m2 = r | ranges::to<std::vector<std::vector<int>, outer_alloc_t>>(
                          outer_alloc_t{49});
        CHECK(m2.get_allocator().tag == 49);
        CHECK(m2 == m);
    }

    // initializer_list with additional arguments.
    {
        auto v = ranges::to<vec_t>({1, 2, 3}, alloc_t{50});
        CPP_assert(same_as<decltype(v), vec_t>);
        CHECK(v.get_allocator().tag == 50);
        check_equal(v, {1, 2, 3});
    }

#if RANGES_CXX_DEDUCTION_GUIDES >= RANGES_CXX_DEDUCTION_GUIDES_17
    // `to_vector` also accepts additional arguments.
    {
        auto v = to_vector(rng, alloc_t{51});
        CPP_assert(same_as<decltype(v), vec_t>);
        CHECK(v.get_allocator().tag == 51);
    }

    // Deduction of the template arguments of the container from the range and
    // the additional arguments.
    {
        auto v1 = ranges::to<std::vector>(rng, alloc_t{52});
        CPP_assert(same_as<decltype(v1), vec_t>);
        CHECK(v1.get_allocator().tag == 52);
        check_equal(v1, {0, 1, 2, 3, 4});

        auto v2 = rng | ranges::to<std::vector>(alloc_t{53});
        CPP_assert(same_as<decltype(v2), vec_t>);
        CHECK(v2.get_allocator().tag == 53);
        check_equal(v2, {0, 1, 2, 3, 4});

        auto s = rng | ranges::to<std::set>(std::greater<int>{});
        CPP_assert(same_as<decltype(s), std::set<int, std::greater<int>>>);
        check_equal(s, {4, 3, 2, 1, 0});

        // `std::vector(rng)` deduces `std::vector<int>` via the copy deduction
        // candidate.
        std::vector<int> const src{1, 2, 3};
        auto v3 = ranges::to<std::vector>(src);
        CPP_assert(same_as<decltype(v3), std::vector<int>>);
        CHECK(v3 == src);
    }
#endif
}

// Test the decrement operators of the internal iterator that is used to
// convert a range of ranges into a container of containers. The postfix
// decrement used to increment the underlying iterator and return a dangling
// reference.
void test_to_container_iterator_decrement()
{
    using namespace ranges;
    using inner_t = std::vector<int>;
    std::vector<inner_t> const src{{1}, {2, 2}, {3, 3, 3}};
    using it_t =
        detail::to_container_iterator<std::vector<inner_t> const &, std::list<inner_t>>;
    CPP_assert(same_as<decltype(std::declval<it_t &>()--), it_t>);
    CPP_assert(same_as<decltype(--std::declval<it_t &>()), it_t &>);

    it_t it{src.end()};
    auto const old = it--;
    CHECK(old == it_t{src.end()});
    CHECK(it == it_t{src.begin() + 2});
    check_equal(*it, {3, 3, 3});

    auto const old2 = it--;
    check_equal(*old2, {3, 3, 3});
    check_equal(*it, {2, 2});

    CHECK(&--it == &it);
    CHECK(it == it_t{src.begin()});
    check_equal(*it, {1});
}

void test_construction_strategies()
{
    using namespace ranges;
    auto rng = views::iota(0, 5);

    // Direct construction `C(rng, args...)`, also for non-range types.
    {
        auto s = ranges::to<sum_with_offset>(rng, 100);
        CHECK(s.sum == 110);
        auto s2 = rng | ranges::to<sum_with_offset>(1000);
        CHECK(s2.sum == 1010);
    }

    // Direct construction is preferred over the iterator-pair constructor.
    {
        auto d = ranges::to<direct_and_iterators>(rng);
        CHECK(d.used_direct);
        check_equal(d, {0, 1, 2, 3, 4});
    }

    // Converting a container of the same type moves (or copies) it.
    {
        std::vector<int> v{1, 2, 3};
        int const * p = v.data();
        auto v2 = ranges::to<std::vector<int>>(std::move(v));
        check_equal(v2, {1, 2, 3});
        CHECK(v2.data() == p);

        std::string const str = "hello";
        auto str2 = ranges::to<std::string>(str);
        CHECK(str2 == str);
    }

    // Containers that only support appending (`push_back`), with reservation
    // for sized ranges and additional constructor arguments.
    {
        auto p = ranges::to<push_back_only>(rng, 7);
        CHECK(p.extra == 7);
        CHECK(p.reservation_count == 1u);
        check_equal(p, {0, 1, 2, 3, 4});

        auto p2 = rng | views::take_while([](int i) { return i < 3; }) |
                  ranges::to<push_back_only>();
        CHECK(p2.extra == 0);
        CHECK(p2.reservation_count == 0u);
        check_equal(p2, {0, 1, 2});
    }

    // Containers that only support `insert(end, value)`.
    {
        auto i = rng | ranges::to<insert_only>();
        check_equal(i, {0, 1, 2, 3, 4});
        auto i2 = ranges::to<insert_only>(rng);
        check_equal(i2, {0, 1, 2, 3, 4});
    }

    // Nested conversion where the outer container only supports appending.
    {
        auto r = views::ints(1, 3) |
                 views::transform([](int i) { return views::ints(i, i + 2); });
        auto l = ranges::to<std::list<push_back_only>>(r);
        CHECK(l.size() == 2u);
        check_equal(l.front(), {1, 2});
        check_equal(l.back(), {2, 3});
    }

    // Overload resolution and SFINAE-friendliness.
    {
        using alloc_t = tagged_allocator<int>;
        using vec_t = std::vector<int, alloc_t>;
        using range_t = decltype(rng);
        // A range as first argument results in a direct conversion, anything
        // else results in a closure.
        CPP_assert(same_as<decltype(ranges::to<vec_t>(rng, alloc_t{})), vec_t>);
        CPP_assert(!same_as<decltype(ranges::to<vec_t>(alloc_t{})), vec_t>);
        CPP_assert(invocable<decltype(ranges::to<vec_t>(alloc_t{})), range_t>);
        // Wrong additional arguments.
        CPP_assert(!invocable<decltype(ranges::to<vec_t>(std::string{})), range_t>);
        CPP_assert(
            !invocable<detail::to_container::fn<meta::id<vec_t>>, range_t, int *>);
        // Elements that can't be converted.
        CPP_assert(!invocable<detail::to_container::fn<meta::id<std::vector<int *>>>,
                              range_t>);
        // Views can't be the target of `ranges::to`.
        CPP_assert(!invocable<detail::to_container::fn<meta::id<range_t>>, range_t>);
    }
}

int main()
{
    using namespace ranges;

    {
        auto lst0 = views::ints | views::transform([](int i) { return i * i; }) |
                    views::take(10) | to<std::list>();
        CPP_assert(same_as<decltype(lst0), std::list<int>>);
        ::check_equal(lst0, {0, 1, 4, 9, 16, 25, 36, 49, 64, 81});
    }

#ifndef RANGES_WORKAROUND_MSVC_779708 // "workaround" is a misnomer; there's no
                                      // workaround.
    {
        auto lst1 = views::ints | views::transform([](int i) { return i * i; }) |
                    views::take(10) | to<std::list>;
        CPP_assert(same_as<decltype(lst1), std::list<int>>);
        ::check_equal(lst1, {0, 1, 4, 9, 16, 25, 36, 49, 64, 81});
    }
#endif // RANGES_WORKAROUND_MSVC_779708

    {
        auto vec0 = views::ints | views::transform([](int i) { return i * i; }) |
                    views::take(10) | to_vector | actions::sort(std::greater<int>{});
        CPP_assert(same_as<decltype(vec0), std::vector<int>>);
        ::check_equal(vec0, {81, 64, 49, 36, 25, 16, 9, 4, 1, 0});
    }

    {
        auto vec1 = views::ints | views::transform([](int i) { return i * i; }) |
                    views::take(10) | to<std::vector<long>>() |
                    actions::sort(std::greater<long>{});
        CPP_assert(same_as<decltype(vec1), std::vector<long>>);
        ::check_equal(vec1, {81, 64, 49, 36, 25, 16, 9, 4, 1, 0});
    }

#ifndef RANGES_WORKAROUND_MSVC_779708
    {
        auto vec2 = views::ints | views::transform([](int i) { return i * i; }) |
                    views::take(10) | to<std::vector<long>> |
                    actions::sort(std::greater<long>{});
        CPP_assert(same_as<decltype(vec2), std::vector<long>>);
        ::check_equal(vec2, {81, 64, 49, 36, 25, 16, 9, 4, 1, 0});
    }
#endif // RANGES_WORKAROUND_MSVC_779708

    {
        const std::size_t N = 4096;
        auto vl = views::iota(0, int{N}) | to<vector_like<int>>();
        CPP_assert(same_as<decltype(vl), vector_like<int>>);
        CHECK(vl.reservation_count == std::size_t{1});
        CHECK(vl.last_reservation == N);
    }

    // https://github.com/ericniebler/range-v3/issues/1145
    {
        auto r1 = views::indices(std::uintmax_t{100});
        auto r2 = views::zip(r1, r1);

#ifdef RANGES_WORKAROUND_MSVC_779708
        auto m = r2 | ranges::to<std::map<std::uintmax_t, std::uintmax_t>>();
#else // ^^^ workaround / no workaround vvv
        auto m = r2 | ranges::to<std::map<std::uintmax_t, std::uintmax_t>>;
#endif // RANGES_WORKAROUND_MSVC_779708
        CPP_assert(same_as<decltype(m), std::map<std::uintmax_t, std::uintmax_t>>);
    }

    // Transform a range-of-ranges into a container of containers
    {
        auto r = views::ints(1, 4) |
                 views::transform([](int i) { return views::ints(i, i + 3); });

        auto m = r | ranges::to<std::vector<std::vector<int>>>();
        CPP_assert(same_as<decltype(m), std::vector<std::vector<int>>>);
        CHECK(m.size() == 3u);
        check_equal(m[0], {1, 2, 3});
        check_equal(m[1], {2, 3, 4});
        check_equal(m[2], {3, 4, 5});
    }

    // Use ranges::to in a closure with an action
    {
#ifdef RANGES_WORKAROUND_MSVC_779708
        auto closure = ranges::to<std::vector>() | actions::sort;
#else // ^^^ workaround / no workaround vvv
        auto closure = ranges::to<std::vector> | actions::sort;
#endif // RANGES_WORKAROUND_MSVC_779708

        auto r = views::ints(1, 4) | views::reverse;
        auto m = r | closure;

        CPP_assert(same_as<decltype(m), std::vector<int>>);
        CHECK(m.size() == 3u);
        check_equal(m, {1, 2, 3});
    }

    test_zip_to_map(views::zip(views::ints, views::iota(0, 10)), 0);

    // https://github.com/ericniebler/range-v3/issues/1544
    {
        std::vector<std::vector<int>> d;
        auto m = views::transform(d, views::all);
        auto v = ranges::to<std::vector<std::vector<int>>>(m);
        check_equal(d, v);
    }

    {
        std::vector<std::pair<int, int>> v = {{1, 2}, {3, 4}};
        auto m1 = ranges::to<map_like<int, int>>(v);
        auto m2 = v | ranges::to<map_like<int, int>>();

        CPP_assert(same_as<decltype(m1), map_like<int, int>>);
        CPP_assert(same_as<decltype(m2), map_like<int, int>>);
        check_equal(m1, std::map<int, int>{{1, 2}, {3, 4}});
        check_equal(m1, m2);

#if RANGES_CXX_DEDUCTION_GUIDES >= RANGES_CXX_DEDUCTION_GUIDES_17
        auto m3 = ranges::to<map_like>(v);
        auto m4 = v | ranges::to<map_like>();
        CPP_assert(same_as<decltype(m3), map_like<int, int>>);
        CPP_assert(same_as<decltype(m4), map_like<int, int>>);
        check_equal(m1, m3);
        check_equal(m1, m4);
#endif
    }

    test_additional_arguments();
    test_construction_strategies();
    test_to_container_iterator_decrement();

    return ::test_result();
}
