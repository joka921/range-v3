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

#ifndef RANGES_V3_RANGE_CONVERSION_HPP
#define RANGES_V3_RANGE_CONVERSION_HPP

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <meta/meta.hpp>

#include <range/v3/range_fwd.hpp>

#include <range/v3/action/concepts.hpp>
#include <range/v3/functional/pipeable.hpp>
#include <range/v3/iterator/common_iterator.hpp>
#include <range/v3/range/concepts.hpp>
#include <range/v3/range/traits.hpp>
#include <range/v3/utility/static_const.hpp>

#include <range/v3/detail/prologue.hpp>

// If the standard library supports the C++23 `std::from_range` constructors of
// the containers, `ranges::to` uses them (like `std::ranges::to` does).
#if defined(__cpp_lib_containers_ranges) && __cpp_lib_containers_ranges >= 202202L
#define RANGES_TO_HAS_STD_FROM_RANGE 1
#else
#define RANGES_TO_HAS_STD_FROM_RANGE 0
#endif

namespace ranges
{
    /// \cond
    namespace detail
    {
        struct to_container
        {
            template<typename MetaFn>
            struct fn;

            template<typename MetaFn, typename... Args>
            struct bound_fn;

            template<typename MetaFn, typename Fn>
            struct closure;

            template<typename MetaFn, typename Rng, typename... Args>
            using container_t = meta::invoke<MetaFn, Rng, Args...>;

            template<typename Rng, typename MetaFn>
            friend auto operator|(Rng && rng,
                                  closure<MetaFn, fn<MetaFn>> (*)(to_container))
                -> CPP_broken_friend_ret(container_t<MetaFn, Rng>)(
                    requires invocable<fn<MetaFn>, Rng>)
            {
                return fn<MetaFn>{}(static_cast<Rng &&>(rng));
            }

            template<typename MetaFn, typename Pipeable>
            friend auto operator|(closure<MetaFn, fn<MetaFn>> (*)(to_container),
                                  Pipeable pipe)
                -> CPP_broken_friend_ret(
                    closure<MetaFn, composed<Pipeable, fn<MetaFn>>>)(
                    requires (is_pipeable_v<Pipeable>))
            {
                return closure<MetaFn, composed<Pipeable, fn<MetaFn>>>{
                    compose(static_cast<Pipeable &&>(pipe), fn<MetaFn>{})};
            }
        };

        // A simple, light-weight transform iterator that applies ranges::to
        // to each element in the range. Used by ranges::to to convert a range
        // of ranges into a container of containers.
        template<typename Rng, typename Cont>
        struct to_container_iterator
        {
        private:
            using I = range_cpp17_iterator_t<Rng>;
            using ValueType = range_value_t<Cont>;
            I it_;

        public:
            using difference_type = typename std::iterator_traits<I>::difference_type;
            using value_type = ValueType;
            using reference = ValueType;
            using pointer = typename std::iterator_traits<I>::pointer;
            using iterator_category = typename std::iterator_traits<I>::iterator_category;

            to_container_iterator() = default;
            template<typename OtherIt>
            to_container_iterator(OtherIt it)
              : it_(std::move(it))
            {}
            friend bool operator==(to_container_iterator const & a,
                                   to_container_iterator const & b)
            {
                return a.it_ == b.it_;
            }
            friend bool operator!=(to_container_iterator const & a,
                                   to_container_iterator const & b)
            {
                return !(a == b);
            }
            reference operator*() const
            {
                return to_container::fn<meta::id<ValueType>>{}(*it_);
            }
            to_container_iterator & operator++()
            {
                ++it_;
                return *this;
            }
            to_container_iterator operator++(int)
            {
                auto tmp = *this;
                ++it_;
                return tmp;
            }
            CPP_member
            auto operator--() //
                -> CPP_ret(to_container_iterator &)(
                    requires derived_from<iterator_category,
                                          std::bidirectional_iterator_tag>)
            {
                --it_;
                return *this;
            }
            CPP_member
            auto operator--(int) //
                -> CPP_ret(to_container_iterator)(
                    requires derived_from<iterator_category,
                                          std::bidirectional_iterator_tag>)
            {
                auto tmp = *this;
                --it_;
                return tmp;
            }
            CPP_member
            auto operator+=(difference_type n) //
                -> CPP_ret(to_container_iterator &)(
                    requires derived_from<iterator_category,
                                          std::random_access_iterator_tag>)
            {
                it_ += n;
                return *this;
            }
            CPP_member
            auto operator-=(difference_type n) //
                -> CPP_ret(to_container_iterator &)(
                    requires derived_from<iterator_category,
                                          std::random_access_iterator_tag>)
            {
                it_ -= n;
                return *this;
            }
            CPP_broken_friend_member
            friend auto operator+(to_container_iterator i, difference_type n) //
                -> CPP_broken_friend_ret(to_container_iterator)(
                    requires derived_from<iterator_category,
                                          std::random_access_iterator_tag>)
            {
                return i += n;
            }
            CPP_broken_friend_member
            friend auto operator-(to_container_iterator i, difference_type n) //
                -> CPP_broken_friend_ret(to_container_iterator)(
                    requires derived_from<iterator_category,
                                          std::random_access_iterator_tag>)
            {
                return i -= n;
            }
            CPP_broken_friend_member
            friend auto operator-(difference_type n, to_container_iterator i) //
                -> CPP_broken_friend_ret(to_container_iterator)(
                    requires derived_from<iterator_category,
                                          std::random_access_iterator_tag>)
            {
                return i -= n;
            }
            CPP_broken_friend_member
            friend auto operator-(to_container_iterator const & i,
                                  to_container_iterator const & j) //
                -> CPP_broken_friend_ret(difference_type)(
                    requires derived_from<iterator_category,
                                          std::random_access_iterator_tag>)
            {
                return i.it_ - j.it_;
            }
            CPP_member
            auto operator[](difference_type n) const //
                -> CPP_ret(reference)(
                    requires derived_from<iterator_category,
                                          std::random_access_iterator_tag>)
            {
                return *(*this + n);
            }
        };

        template<typename Rng, typename Cont>
        using to_container_iterator_t =
            enable_if_t<(bool)range<Rng>, to_container_iterator<Rng, Cont>>;

        // A light-weight, non-owning range over a range of ranges `Rng` whose
        // elements are the elements of `Rng`, each converted via `ranges::to`
        // to `range_value_t<Cont>`. This is the equivalent of
        // `rng | views::transform([](auto&& r) { return to<range_value_t<Cont>>(r); })`
        // that `std::ranges::to` uses to convert a range of ranges into a
        // container of containers.
        template<typename Rng, typename Cont>
        struct to_container_range
        {
        private:
            using iterator = to_container_iterator<Rng, Cont>;
            meta::_t<std::remove_reference<Rng>> * rng_;

        public:
            explicit to_container_range(meta::_t<std::remove_reference<Rng>> & rng)
              : rng_(&rng)
            {}
            iterator begin() const
            {
                return iterator{ranges::begin(*rng_)};
            }
            iterator end() const
            {
                return iterator{ranges::end(*rng_)};
            }
            template(typename R = Rng)(
                requires sized_range<R>)
            range_size_t<R> size() const
            {
                return ranges::size(*rng_);
            }
        };

        // clang-format off
        /// \concept range_and_not_view_
        /// \brief The \c range_and_not_view_ concept
        template(typename Rng)(
        concept (range_and_not_view_)(Rng),
            range<Rng> AND (!view_<Rng>));

        /// \concept range_and_not_view
        /// \brief The \c range_and_not_view concept
        template<typename Rng>
        CPP_concept range_and_not_view =
            CPP_concept_ref(range_and_not_view_, Rng);

        /// \concept convertible_to_cont_impl_
        /// \brief The \c convertible_to_cont_impl_ concept
        template(typename Rng, typename Cont)(
        concept (convertible_to_cont_impl_)(Rng, Cont),
            constructible_from<range_value_t<Cont>, range_reference_t<Rng>> AND
            constructible_from<
                Cont,
                range_cpp17_iterator_t<Rng>,
                range_cpp17_iterator_t<Rng>>
        );
        /// \concept convertible_to_cont
        /// \brief The \c convertible_to_cont concept
        template<typename Rng, typename Cont>
        CPP_concept convertible_to_cont = //
            range_and_not_view<Cont> && //
            move_constructible<Cont> && //
            CPP_concept_ref(detail::convertible_to_cont_impl_, Rng, Cont);

        /// \concept convertible_to_cont_cont_impl_
        /// \brief The \c convertible_to_cont_cont_impl_ concept
        template(typename Rng, typename Cont)(
        concept (convertible_to_cont_cont_impl_)(Rng, Cont),
            range_and_not_view<range_value_t<Cont>> AND
            // Test that each element of the input range can be ranges::to<>
            // to the output container.
            invocable<
                to_container::fn<meta::id<range_value_t<Cont>>>,
                range_reference_t<Rng>> AND
            constructible_from<
                Cont,
                to_container_iterator_t<Rng, Cont>,
                to_container_iterator_t<Rng, Cont>>
        );
        /// \concept convertible_to_cont_cont
        /// \brief The \c convertible_to_cont_cont concept
        template<typename Rng, typename Cont>
        CPP_concept convertible_to_cont_cont = //
            range<Cont> && //
            (!view_<Cont>) && //
            move_constructible<Cont> && //
            CPP_concept_ref(detail::convertible_to_cont_cont_impl_, Rng, Cont);

        /// \concept to_container_reserve
        /// \brief The \c to_container_reserve concept
        // Note: `c.assign(first, last)` assigns the elements of the range to
        // existing elements of the container (and for input iterators also
        // move-assigns elements when inserting), so the value type of the
        // container has to be assignable from the elements of the range and
        // move-assignable. Without these checks, `ranges::to` would select the
        // `reserve` + `assign` strategy and then fail to compile for element
        // types that are only constructible from the elements of the range
        // (e.g. types with `const` or reference members, or types that are
        // only explicitly constructible from the elements of the range).
        // Note: Without concepts (C++14/17), all the operands of `&&` in a
        // `CPP_concept` are instantiated, so the assignability checks are
        // wrapped in a class template that only accesses `range_value_t<C>`
        // if `C` is a range.
        template<typename C, typename I, bool = (bool)range<C>>
        struct to_container_assignable : std::false_type
        {};
        template<typename C, typename I>
        struct to_container_assignable<C, I, true>
          : meta::bool_<
                std::is_assignable<range_value_t<C> &, iter_reference_t<I>>::value &&
                std::is_move_assignable<range_value_t<C>>::value>
        {};
        template<typename C, typename I, typename R>
        CPP_concept to_container_reserve = //
            reservable_with_assign<C, I> && //
            sized_range<R> && //
            to_container_assignable<C, I>::value;

        /// \concept to_reservable_container
        /// \brief The exposition-only \c reservable-container concept of
        /// `std::ranges::to`.
        template<typename C>
        CPP_concept to_reservable_container = //
            sized_range<C> && //
            CPP_requires_ref(ranges::reservable_, C);

        /// \concept to_emplace_back_appendable_
        /// \brief The \c to_emplace_back_appendable_ concept
        template<typename C, typename Ref>
        CPP_requires(to_emplace_back_appendable_,
            requires(C & c, Ref && ref) //
            (
                c.emplace_back(static_cast<Ref &&>(ref))
            ));
        /// \concept to_push_back_appendable_
        /// \brief The \c to_push_back_appendable_ concept
        template<typename C, typename Ref>
        CPP_requires(to_push_back_appendable_,
            requires(C & c, Ref && ref) //
            (
                c.push_back(static_cast<Ref &&>(ref))
            ));
        /// \concept to_emplace_appendable_
        /// \brief The \c to_emplace_appendable_ concept
        template<typename C, typename Ref>
        CPP_requires(to_emplace_appendable_,
            requires(C & c, Ref && ref) //
            (
                c.emplace(c.end(), static_cast<Ref &&>(ref))
            ));
        /// \concept to_insert_appendable_
        /// \brief The \c to_insert_appendable_ concept
        template<typename C, typename Ref>
        CPP_requires(to_insert_appendable_,
            requires(C & c, Ref && ref) //
            (
                c.insert(c.end(), static_cast<Ref &&>(ref))
            ));
        /// \concept to_container_appendable
        /// \brief The exposition-only \c container-appendable concept of
        /// `std::ranges::to`.
        template<typename C, typename Ref>
        CPP_concept to_container_appendable =
            CPP_requires_ref(detail::to_emplace_back_appendable_, C, Ref) ||
            CPP_requires_ref(detail::to_push_back_appendable_, C, Ref) ||
            CPP_requires_ref(detail::to_emplace_appendable_, C, Ref) ||
            CPP_requires_ref(detail::to_insert_appendable_, C, Ref);
        // clang-format on

        template<typename MetaFn, typename Rng, typename... Args>
        using container_t = meta::invoke<MetaFn, Rng, Args...>;

        // The strategy that `ranges::to<C>(rng, args...)` uses to construct the
        // container. The order of the alternatives is the order in which they
        // are tried, see [range.utility.conv.to] in the C++23 standard.
        enum class to_kind
        {
            none,
            // C(rng, args...)
            direct,
            // C(std::from_range, rng, args...)
            from_range,
            // C c(args...); c.reserve(size(rng)); c.assign(begin, end);
            // (A range-v3 specific optimization of the next alternative.)
            reserve_assign,
            // C(begin(rng), end(rng), args...)
            iterators,
            // C c(args...); (c.reserve(size(rng));) + append each element
            append,
            // to<C>(rng | transform(to<range_value_t<C>>), args...)
            nested,
        };

        template<to_kind K>
        using to_kind_t = std::integral_constant<to_kind, K>;

        // Step 1 of [range.utility.conv.to] is used if `C` is not an
        // input_range or if the elements of `Rng` can be converted to the
        // value type of `C`. As an extension to the standard, step 1 is also
        // used if the elements of `Rng` can only explicitly be converted to
        // the value type of `C`, but are not ranges themselves (range-v3 has
        // always supported this).
        template<typename C, typename Rng, bool = (bool)input_range<C>>
        struct to_use_step_one : std::true_type
        {};
        template<typename C, typename Rng>
        struct to_use_step_one<C, Rng, true>
          : meta::bool_<(bool)convertible_to<range_reference_t<Rng>,
                                             range_value_t<C>> ||
                        (!(bool)input_range<range_reference_t<Rng>> &&
                         (bool)constructible_from<range_value_t<C>,
                                                  range_reference_t<Rng>>)>
        {};

        template<typename C, typename Rng, typename... Args>
        struct to_step_one_kind
        {
        private:
            using I = range_cpp17_iterator_t<Rng>;
            static constexpr bool direct_ = constructible_from<C, Rng, Args...>;
#if RANGES_TO_HAS_STD_FROM_RANGE
            static constexpr bool from_range_ =
                constructible_from<C, std::from_range_t const &, Rng, Args...>;
#else
            static constexpr bool from_range_ = false;
#endif
            static constexpr bool iterators_ = constructible_from<C, I, I, Args...>;
            static constexpr bool reserve_assign_ =
                iterators_ && constructible_from<C, Args...> &&
                to_container_reserve<C, I, Rng>;
            static constexpr bool append_ =
                constructible_from<C, Args...> &&
                to_container_appendable<C, range_reference_t<Rng>>;
            // If the `reserve` + `assign` strategy cannot be used (e.g. because
            // the elements are not assignable), then for a sized range that is
            // not a forward range, the iterator-pair constructor cannot
            // determine the number of elements in advance. Prefer `reserve` +
            // appending each element in this case (a range-v3 specific
            // deviation from the order of the standard).
            static constexpr bool reserve_append_ =
                append_ && sized_range<Rng> && !forward_range<Rng> &&
                to_reservable_container<C>;

        public:
            static constexpr to_kind value =
                !to_use_step_one<C, Rng>::value ? to_kind::none
                : direct_                       ? to_kind::direct
                : from_range_                   ? to_kind::from_range
                : reserve_assign_               ? to_kind::reserve_assign
                : reserve_append_               ? to_kind::append
                : iterators_                    ? to_kind::iterators
                : append_                       ? to_kind::append
                                                : to_kind::none;
        };

        template<typename C, typename Rng, typename... Args>
        struct to_kind_of;

        // Step 2 of [range.utility.conv.to]: Convert each element of a range of
        // ranges to `range_value_t<C>` and then convert the result to `C`.
        template<typename C, typename Rng, bool = (bool)input_range<C>>
        struct to_use_step_two : std::false_type
        {};
        template<typename C, typename Rng>
        struct to_use_step_two<C, Rng, true>
          : meta::bool_<(bool)input_range<range_reference_t<Rng>> &&
                        !(bool)convertible_to<range_reference_t<Rng>,
                                              range_value_t<C>>>
        {};

        template<typename C, typename Rng, typename ArgsList,
                 bool = to_use_step_two<C, Rng>::value>
        struct to_step_two_kind : to_kind_t<to_kind::none>
        {};
        template<typename C, typename Rng, typename... Args>
        struct to_step_two_kind<C, Rng, meta::list<Args...>, true>
          : to_kind_t<(invocable<to_container::fn<meta::id<range_value_t<C>>>,
                                 range_reference_t<Rng>> &&
                       to_kind_of<C, to_container_range<Rng, C>, Args...>::value !=
                           to_kind::none)
                          ? to_kind::nested
                          : to_kind::none>
        {};

        template<typename C, typename Rng, typename... Args>
        struct to_kind_of
          : to_kind_t<(view_<C> || !move_constructible<C>) ? to_kind::none
                      : to_step_one_kind<C, Rng, Args...>::value != to_kind::none
                          ? to_step_one_kind<C, Rng, Args...>::value
                          : to_step_two_kind<C, Rng, meta::list<Args...>>::value>
        {};

        // Computes the `to_kind` for `ranges::to` with the container (meta-)
        // function `MetaFn`, the range type `Rng` and the additional
        // constructor arguments `Args...`. Is `to_kind::none` if `Rng` is not
        // an `input_range`.
        template<typename MetaFn, typename Rng, typename ArgsList,
                 bool = (bool)input_range<Rng>>
        struct to_kind_for : to_kind_t<to_kind::none>
        {};
        template<typename MetaFn, typename Rng, typename... Args>
        struct to_kind_for<MetaFn, Rng, meta::list<Args...>, true>
          : to_kind_of<container_t<MetaFn, Rng, Args...>, Rng, Args...>
        {};

        template<typename MetaFn, typename Rng, typename... Args>
        CPP_concept to_container_convertible =
            to_kind_for<MetaFn, Rng, meta::list<Args...>>::value != to_kind::none;

        struct RANGES_STRUCT_WITH_ADL_BARRIER(to_container_closure_base)
        {
            // clang-format off
            template(typename Rng, typename MetaFn, typename Fn)(
                requires input_range<Rng> AND invocable<Fn, Rng>)
            friend constexpr auto
            operator|(Rng && rng, to_container::closure<MetaFn, Fn> fn)
            {
                return static_cast<Fn &&>(fn)(static_cast<Rng &&>(rng));
            }

            template<typename MetaFn, typename Fn, typename Pipeable>
            friend constexpr auto operator|(to_container::closure<MetaFn, Fn> sh,
                                            Pipeable pipe)
                -> CPP_broken_friend_ret(
                    to_container::closure<MetaFn, composed<Pipeable, Fn>>)(
                    requires is_pipeable_v<Pipeable>)
            {
                return to_container::closure<MetaFn, composed<Pipeable, Fn>>{
                    compose(static_cast<Pipeable &&>(pipe), static_cast<Fn &&>(sh))};
            }
            // clang-format on
        };

        template<typename MetaFn, typename Fn>
        struct to_container::closure
          : to_container_closure_base
          , Fn
        {
            closure() = default;
            constexpr explicit closure(Fn fn)
              : Fn(static_cast<Fn &&>(fn))
            {}
        };

        template<typename MetaFn>
        struct to_container::fn
        {
        private:
            template<typename C, typename Rng, typename... Args>
            static C impl(to_kind_t<to_kind::direct>, Rng && rng, Args &&... args)
            {
                return C(static_cast<Rng &&>(rng), static_cast<Args &&>(args)...);
            }
#if RANGES_TO_HAS_STD_FROM_RANGE
            template<typename C, typename Rng, typename... Args>
            static C impl(to_kind_t<to_kind::from_range>, Rng && rng,
                          Args &&... args)
            {
                return C(std::from_range, static_cast<Rng &&>(rng),
                         static_cast<Args &&>(args)...);
            }
#endif
            template<typename C, typename Rng, typename... Args>
            static C impl(to_kind_t<to_kind::reserve_assign>, Rng && rng,
                          Args &&... args)
            {
                using I = range_cpp17_iterator_t<Rng>;
                C c(static_cast<Args &&>(args)...);
                auto const rng_size = ranges::size(rng);
                using size_type = decltype(c.max_size());
                using Common = common_type_t<range_size_t<Rng>, size_type>;
                RANGES_EXPECT(static_cast<Common>(rng_size) <=
                              static_cast<Common>(c.max_size()));
                c.reserve(static_cast<size_type>(rng_size));
                c.assign(I{ranges::begin(rng)}, I{ranges::end(rng)});
                return c;
            }
            template<typename C, typename Rng, typename... Args>
            static C impl(to_kind_t<to_kind::iterators>, Rng && rng,
                          Args &&... args)
            {
                using I = range_cpp17_iterator_t<Rng>;
                return C(I{ranges::begin(rng)}, I{ranges::end(rng)},
                         static_cast<Args &&>(args)...);
            }

            template(typename C, typename Ref)(
                requires CPP_requires_ref(detail::to_emplace_back_appendable_, C, Ref))
            static void append(C & c, Ref && ref, priority_tag<3>)
            {
                c.emplace_back(static_cast<Ref &&>(ref));
            }
            template(typename C, typename Ref)(
                requires CPP_requires_ref(detail::to_push_back_appendable_, C, Ref))
            static void append(C & c, Ref && ref, priority_tag<2>)
            {
                c.push_back(static_cast<Ref &&>(ref));
            }
            template(typename C, typename Ref)(
                requires CPP_requires_ref(detail::to_emplace_appendable_, C, Ref))
            static void append(C & c, Ref && ref, priority_tag<1>)
            {
                c.emplace(c.end(), static_cast<Ref &&>(ref));
            }
            template(typename C, typename Ref)(
                requires CPP_requires_ref(detail::to_insert_appendable_, C, Ref))
            static void append(C & c, Ref && ref, priority_tag<0>)
            {
                c.insert(c.end(), static_cast<Ref &&>(ref));
            }

            template<typename C, typename Rng>
            static void maybe_reserve(C &, Rng &, std::false_type)
            {}
            template<typename C, typename Rng>
            static void maybe_reserve(C & c, Rng & rng, std::true_type)
            {
                c.reserve(static_cast<range_size_t<C>>(ranges::size(rng)));
            }

            template<typename C, typename Rng, typename... Args>
            static C impl(to_kind_t<to_kind::append>, Rng && rng, Args &&... args)
            {
                C c(static_cast<Args &&>(args)...);
                maybe_reserve(
                    c,
                    rng,
                    meta::bool_<(bool)sized_range<Rng> &&
                                (bool)to_reservable_container<C>>{});
                auto first = ranges::begin(rng);
                auto const last = ranges::end(rng);
                for(; first != last; ++first)
                    append(c, *first, priority_tag<3>{});
                return c;
            }
            template<typename C, typename Rng, typename... Args>
            static C impl(to_kind_t<to_kind::nested>, Rng && rng, Args &&... args)
            {
                using R = to_container_range<Rng, C>;
                return impl<C>(to_kind_of<C, R, Args...>{},
                               R{rng},
                               static_cast<Args &&>(args)...);
            }

        public:
            template(typename Rng, typename... Args)(
                requires input_range<Rng> AND
                    to_container_convertible<MetaFn, Rng, Args...>)
            container_t<MetaFn, Rng, Args...> operator()(Rng && rng,
                                                         Args &&... args) const
            {
                static_assert(!is_infinite<Rng>::value,
                              "Attempt to convert an infinite range to a container.");
                using cont_t = container_t<MetaFn, Rng, Args...>;
                return impl<cont_t>(to_kind_for<MetaFn, Rng, meta::list<Args...>>{},
                                    static_cast<Rng &&>(rng),
                                    static_cast<Args &&>(args)...);
            }
        };

        // The function object that is stored inside the closure that is
        // returned by `ranges::to<C>(args...)`. Stores (decayed copies of) the
        // additional arguments `args...` and passes them to the container's
        // constructor when called with a range.
        template<typename MetaFn, typename... Args>
        struct to_container::bound_fn
        {
        private:
            std::tuple<Args...> args_;

            template<typename Rng, typename Tuple, std::size_t... Is>
            static auto call_(Rng && rng, Tuple && args,
                              meta::index_sequence<Is...>)
                -> container_t<MetaFn, Rng, decltype(std::get<Is>(
                                                static_cast<Tuple &&>(args)))...>
            {
                return to_container::fn<MetaFn>{}(
                    static_cast<Rng &&>(rng),
                    std::get<Is>(static_cast<Tuple &&>(args))...);
            }

        public:
            bound_fn() = default;
            template<typename... Ts>
            constexpr explicit bound_fn(meta::id<void>, Ts &&... ts)
              : args_(static_cast<Ts &&>(ts)...)
            {}

            template(typename Rng)(
                requires invocable<to_container::fn<MetaFn>, Rng, Args const &...>)
            auto operator()(Rng && rng) const & //
                -> container_t<MetaFn, Rng, Args const &...>
            {
                return call_(static_cast<Rng &&>(rng), args_,
                             meta::make_index_sequence<sizeof...(Args)>{});
            }
            template(typename Rng)(
                requires invocable<to_container::fn<MetaFn>, Rng, Args...>)
            auto operator()(Rng && rng) && //
                -> container_t<MetaFn, Rng, Args...>
            {
                return call_(static_cast<Rng &&>(rng), std::move(args_),
                             meta::make_index_sequence<sizeof...(Args)>{});
            }
        };

        template<typename MetaFn, typename Fn>
        using to_container_closure = to_container::closure<MetaFn, Fn>;

        template<typename MetaFn>
        using to_container_fn = to_container_closure<MetaFn, to_container::fn<MetaFn>>;

        template<typename MetaFn, typename... Args>
        using to_container_bound_fn =
            to_container_closure<MetaFn, to_container::bound_fn<MetaFn, Args...>>;

        template<template<typename...> class ContT>
        struct from_range
        {
#if RANGES_CXX_DEDUCTION_GUIDES >= RANGES_CXX_DEDUCTION_GUIDES_17
            // Deduce the template arguments of `ContT` like `std::ranges::to`
            // does: First try `ContT(rng, args...)`, ...
            template<typename Rng, typename... Args>
            static auto from_rng_(priority_tag<3>) //
                -> decltype(ContT(std::declval<Rng>(), std::declval<Args>()...));
#if RANGES_TO_HAS_STD_FROM_RANGE
            // ... then `ContT(std::from_range, rng, args...)`, ...
            template<typename Rng, typename... Args>
            static auto from_rng_(priority_tag<2>) //
                -> decltype(ContT(std::from_range, std::declval<Rng>(),
                                  std::declval<Args>()...));
#endif
            // ... then `ContT(first, last, args...)`.
            template<typename Rng, typename... Args>
            static auto from_rng_(priority_tag<1>) //
                -> decltype(ContT(std::declval<range_cpp17_iterator_t<Rng>>(),
                                  std::declval<range_cpp17_iterator_t<Rng>>(),
                                  std::declval<Args>()...));
            // No deduction guide. Fallback to instantiating with the
            // range's value type.
            template<typename Rng, typename... Args>
            static auto from_rng_(priority_tag<0>) //
                -> meta::invoke<meta::quote<ContT>, range_value_t<Rng>>;

            template<typename Rng, typename... Args>
            using invoke =
                decltype(from_range::from_rng_<Rng, Args...>(priority_tag<3>{}));
#else
            template<typename Rng, typename... Args>
            using invoke = meta::invoke<meta::quote<ContT>, range_value_t<Rng>>;
#endif
        };
    } // namespace detail
    /// \endcond

    /// \addtogroup group-range
    /// @{

    /// \ingroup group-range
    RANGES_INLINE_VARIABLE(detail::to_container_fn<detail::from_range<std::vector>>,
                           to_vector)

    /// \cond
    namespace _to_
    {
        /// \endcond

        /// \brief For initializing a container of the specified type with the elements
        /// of a range. Behaves like `std::ranges::to` from C++23:
        ///
        /// * `ranges::to<C>(rng, args...)` constructs a `C` from the elements of `rng`,
        ///   passing the additional arguments `args...` (e.g. an allocator) to the
        ///   constructor of `C`.
        /// * `ranges::to<C>(args...)` returns a closure, s.t.
        ///   `rng | ranges::to<C>(args...)` is equivalent to the above.
        /// * If `C` is a class template (e.g. `std::vector`), its template arguments
        ///   are deduced from the range and the arguments.
        ///
        /// The container is constructed by the first of the following that is valid:
        /// `C(rng, args...)`, `C(std::from_range, rng, args...)` (if the standard
        /// library supports it), `C(first, last, args...)`, and finally default
        /// construction via `C(args...)` followed by appending each element. A range
        /// of ranges is converted to a container of containers by recursively
        /// converting each element.
        template<template<typename...> class ContT>
        auto to(RANGES_HIDDEN_DETAIL(detail::to_container = {}))
            -> detail::to_container_fn<detail::from_range<ContT>>
        {
            return {};
        }

        /// \overload
        template(template<typename...> class ContT, typename Arg0, typename... Args)(
            requires (!input_range<Arg0>))
        auto to(Arg0 && arg0, Args &&... args)
            -> detail::to_container_bound_fn<detail::from_range<ContT>,
                                             detail::decay_t<Arg0>,
                                             detail::decay_t<Args>...>
        {
            using fn_t = detail::to_container::bound_fn<detail::from_range<ContT>,
                                                        detail::decay_t<Arg0>,
                                                        detail::decay_t<Args>...>;
            return detail::to_container_bound_fn<detail::from_range<ContT>,
                                                 detail::decay_t<Arg0>,
                                                 detail::decay_t<Args>...>{
                fn_t{meta::id<void>{},
                     static_cast<Arg0 &&>(arg0),
                     static_cast<Args &&>(args)...}};
        }

        /// \overload
        template(template<typename...> class ContT, typename Rng, typename... Args)(
            requires input_range<Rng> AND
                detail::to_container_convertible<detail::from_range<ContT>, Rng,
                                                 Args...>)
        auto to(Rng && rng, Args &&... args)
            -> detail::container_t<detail::from_range<ContT>, Rng, Args...>
        {
            return detail::to_container::fn<detail::from_range<ContT>>{}(
                static_cast<Rng &&>(rng), static_cast<Args &&>(args)...);
        }

        /// \overload
        template<typename Cont>
        auto to(RANGES_HIDDEN_DETAIL(detail::to_container = {}))
            -> detail::to_container_fn<meta::id<Cont>>
        {
            return {};
        }

        /// \overload
        template(typename Cont, typename Arg0, typename... Args)(
            requires (!input_range<Arg0>))
        auto to(Arg0 && arg0, Args &&... args)
            -> detail::to_container_bound_fn<meta::id<Cont>,
                                             detail::decay_t<Arg0>,
                                             detail::decay_t<Args>...>
        {
            using fn_t = detail::to_container::bound_fn<meta::id<Cont>,
                                                        detail::decay_t<Arg0>,
                                                        detail::decay_t<Args>...>;
            return detail::to_container_bound_fn<meta::id<Cont>,
                                                 detail::decay_t<Arg0>,
                                                 detail::decay_t<Args>...>{
                fn_t{meta::id<void>{},
                     static_cast<Arg0 &&>(arg0),
                     static_cast<Args &&>(args)...}};
        }

        /// \overload
        template(typename Cont, typename Rng, typename... Args)(
            requires input_range<Rng> AND
                detail::to_container_convertible<meta::id<Cont>, Rng, Args...>)
        auto to(Rng && rng, Args &&... args) -> Cont
        {
            return detail::to_container::fn<meta::id<Cont>>{}(
                static_cast<Rng &&>(rng), static_cast<Args &&>(args)...);
        }

        /// \cond
        // Slightly odd initializer_list overloads, undocumented for now.
        template(template<typename...> class ContT, typename T, typename... Args)(
            requires detail::to_container_convertible<meta::id<ContT<T>>,
                                                      std::initializer_list<T>,
                                                      Args...>)
        auto to(std::initializer_list<T> il, Args &&... args) -> ContT<T>
        {
            return detail::to_container::fn<meta::id<ContT<T>>>{}(
                il, static_cast<Args &&>(args)...);
        }
        template(typename Cont, typename T, typename... Args)(
            requires detail::to_container_convertible<meta::id<Cont>,
                                                      std::initializer_list<T>,
                                                      Args...>)
        auto to(std::initializer_list<T> il, Args &&... args) -> Cont
        {
            return detail::to_container::fn<meta::id<Cont>>{}(
                il, static_cast<Args &&>(args)...);
        }
        /// \endcond

        /// \cond
    } // namespace _to_
    using namespace _to_;
    /// \endcond
    /// @}

    ////////////////////////////////////////////////////////////////////////////
    /// \cond
    namespace _to_
    {
        // The old name "ranges::to_" is now deprecated:
        template<template<typename...> class ContT>
        RANGES_DEPRECATED("Please use ranges::to (no underscore) instead.")
        detail::to_container_fn<detail::from_range<ContT>> to_(detail::to_container = {})
        {
            return {};
        }
        template(template<typename...> class ContT, typename Rng)(
            requires range<Rng> AND
                detail::convertible_to_cont<Rng, ContT<range_value_t<Rng>>>)
        RANGES_DEPRECATED("Please use ranges::to (no underscore) instead.")
        ContT<range_value_t<Rng>> to_(Rng && rng)
        {
            return static_cast<Rng &&>(rng) | ranges::to_<ContT>();
        }
        template(template<typename...> class ContT, typename T)(
            requires detail::convertible_to_cont<std::initializer_list<T>, ContT<T>>)
        RANGES_DEPRECATED("Please use ranges::to (no underscore) instead.")
        ContT<T> to_(std::initializer_list<T> il)
        {
            return il | ranges::to_<ContT>();
        }
        template<typename Cont>
        RANGES_DEPRECATED("Please use ranges::to (no underscore) instead.")
        detail::to_container_fn<meta::id<Cont>> to_(detail::to_container = {})
        {
            return {};
        }
        template(typename Cont, typename Rng)(
            requires range<Rng> AND detail::convertible_to_cont<Rng, Cont>)
        RANGES_DEPRECATED("Please use ranges::to (no underscore) instead.")
        Cont to_(Rng && rng)
        {
            return static_cast<Rng &&>(rng) | ranges::to_<Cont>();
        }
        template(typename Cont, typename T)(
            requires detail::convertible_to_cont<std::initializer_list<T>, Cont>)
        RANGES_DEPRECATED("Please use ranges::to (no underscore) instead.")
        Cont to_(std::initializer_list<T> list)
        {
            return list | ranges::to_<Cont>();
        }
    } // namespace _to_
    /// \endcond

    template<typename MetaFn, typename Fn>
    RANGES_INLINE_VAR constexpr bool
        is_pipeable_v<detail::to_container_closure<MetaFn, Fn>> = true;
} // namespace ranges

#undef RANGES_TO_HAS_STD_FROM_RANGE

#include <range/v3/detail/epilogue.hpp>

#endif
