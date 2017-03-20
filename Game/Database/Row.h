#pragma once

#include <Core\Utils.h>

template<class Task> struct ColActualType { using type = Task; };

// Adapted from boost's BOOST_STRONG_TYPEDEF
#define COL(R, D) \
template<class T> \
struct D##_T_ \
    : boost::totally_ordered1< D##_T_<T> \
    , boost::totally_ordered2< D##_T_<T>, T \
    > > \
{ \
    T t; \
    explicit D##_T_(const T t_) : t(t_) {}; \
    D##_T_(): t() {}; \
    D##_T_(const D##_T_ & t_) : t(t_.t){} \
    D##_T_ & operator=(const D##_T_ & rhs) { t = rhs.t; return *this;} \
    D##_T_ & operator=(const T & rhs) { t = rhs; return *this;} \
    operator const T & () const {return t; } \
    operator T & () { return t; } \
    bool operator==(const D##_T_ & rhs) const { return t == rhs.t; } \
    bool operator<(const D##_T_ & rhs) const { return t < rhs.t; } \
	auto operator->() const { return t; } \
}; \
using D = D##_T_<R>; \
static const TypeWrapper<D> D##_; \
template<> struct ColActualType<D> { using type = R; };

template<class TKey> struct MinKey;
template<class TKey> struct MaxKey;

#define KEY_COL(T, D, L, G) COL(T, D) \
	template<> struct MinKey<D> { D operator()(){ return D{ L }; } }; \
	template<> struct MaxKey<D> { D operator()() { return D{ G }; } };

#define NUMERIC_COL(T, D) KEY_COL(T, D, std::numeric_limits<T>::min(), std::numeric_limits<T>::max())

template<class... TValues> class Row {
public:
	using ValueTypes = typename mpl::vector<typename std::remove_reference<TValues>::type...>::type;
	using ValueTypeSet = typename mpl::set<typename std::remove_reference<TValues>::type...>::type;

private:
	using Types = typename mpl::set<TValues...>::type;

	template<class Task> using StoredType = typename std::conditional<mpl::contains<Types, T&>::type::value, T&, T>::type;

	tuple<TValues...> refs;

	template<class From, class To> struct IsConvertible {
		using type = mpl::bool_<std::is_convertible<From, To>::value>;
	};
	template<class Task> struct HasCompatibleCol {
		static const bool value = mpl::accumulate<ValueTypes, mpl::false_, mpl::or_<mpl::_1, IsConvertible<Task, mpl::_2>>>::type::value;
	};
	template<class Task> struct NumCompatibleCols {
		static const int value = mpl::accumulate<ValueTypes, mpl::int_<0>, mpl::if_<IsConvertible<T, mpl::_2>, mpl::next<mpl::_1>, mpl::_1>>::type::value;
	};

	template<class... Ts> struct SetColumn {
		const Row<Ts...>& from;
		Row& to;
		template<class Task> void operator()(const Task& type) {
			to.c<Task::type>() = from.c<Task::type>();
		}
	};

	struct SwapColumn {
		Row& left;
		Row& right;
		template<class Task> void operator()(const Task& type) {
			using std::swap;
			swap(left.unsafeCol<Task::type>(), right.unsafeCol<Task::type>());
		}
	};

public:
	template<class Task> using HasCol = typename mpl::contains<ValueTypeSet, T>::type;

	Row(TValues&&... values) : refs(std::forward<TValues>(values)...) {}

	Row(const Row& other) = default;

	Row& operator=(const Row& other) = delete;

	template<class Task, typename std::enable_if<!std::is_reference<T>::value && mpl::contains<ValueTypes, T>::type::value, int>::type = 0>
	explicit operator T() { return c<StoredType<T>>(); }

	template<class T, typename std::enable_if<HasCompatibleCol<T>::value, int>::type = 0>
	Row& operator |=(T x) {
		static_assert(NumCompatibleCols<T>::value == 1, "Ambiguous column assign. Value is implicitly convertible to multiple columns.");
		c<typename mpl::deref<typename mpl::find_if<ValueTypes, IsConvertible<T, mpl::_1>>::type>::type>() = x; return *this;
	}

	template<class... Ts> Row& operator|=(const Row<Ts...>& other) {
		using OtherTypes = typename mpl::set<typename std::remove_reference<Ts>::type...>::type;
		using Intersection = typename mpl::remove_if<ValueTypes, mpl::not_<mpl::contains<OtherTypes, mpl::_1>>>::type;
		mpl::for_each<Intersection, TypeWrap<mpl::_1>>(SetColumn<Ts...>{ other, *this });
		return *this;
	}

	template<class T> const T& c() const {
		return get<StoredType<typename std::remove_reference<T>::type>>(refs);
	}
	template<class T, typename std::enable_if<mpl::contains<Types, T&>::type::value, int>::type = 0>
	T& c() {
		return get<StoredType<typename std::remove_reference<T>::type>>(refs);
	}

	template<class TCol>
	auto& operator|(const TCol& name) {
		return c<TCol::type>();
	}

	template<class T>
	T& unsafeCol() {
		return get<StoredType<typename std::remove_reference<T>::type>>(refs);
	}

	template<class... Ts>
	Row<StoredType<typename std::remove_reference<Ts>::type>...> cols() {
		return Row<StoredType<typename std::remove_reference<Ts>::type>...>(c<Ts>()...);
	}

	auto temp() {
		return makeRow(std::remove_reference<TValues>::type(*this)...);
	}

	friend void swap(Row&& left, Row&& right) {
		mpl::for_each<ValueTypes, TypeWrap<mpl::_1>>(SwapColumn{ left, right });
	}

	template<class TCol, class... TCols> auto vec() {
		using Vec = cml::vector<typename ColActualType<TCol>::type, cml::VecCols<Row, TCol, TCols...>>;
		return Vec(*this);
	}
};

namespace impl {
	template<class... TParams> struct MakerRow {
		using type = Row<typename std::conditional<std::is_const<typename std::remove_reference<TParams>::type>::value || std::is_rvalue_reference<TParams>::value, typename std::remove_const<typename std::remove_reference<TParams>::type>::type, typename std::remove_reference<TParams>::type&>::type...>;
	};
}

template<class... Ts> auto makeRow(Ts&&... params) -> typename impl::MakerRow<decltype(params)...>::type {
	return impl::MakerRow<decltype(params)...>::type(std::forward<Ts>(params)...);
}