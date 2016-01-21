#pragma once

// Adapted from BOOST_STRONG_TYPEDEF to support unique_ptr
#define COL(T, D) BOOST_STRONG_TYPEDEF(T, D)

template<class... TValues> class Row {
	using ValueTypes = typename mpl::vector<typename std::remove_reference<TValues>::type...>::type;
	using ValueTypeSet = typename mpl::set<typename std::remove_reference<TValues>::type...>::type;
	using Types = typename mpl::vector<TValues...>::type;

	template<class T> using StoredType = typename std::conditional<mpl::contains<Types, T&>::type::value, T&, T>::type;

	tuple<TValues...> refs;

	template<class From, class To> struct IsConvertible {
		using type = mpl::bool_<std::is_convertible<From, To>::value>;
	};
	template<class T> struct HasCompatibleCol {
		static const bool value = mpl::accumulate<ValueTypes, mpl::false_, mpl::or_<mpl::_1, IsConvertible<T, mpl::_2>>>::type::value;
	};
	template<class T> struct NumCompatibleCols {
		static const int value = mpl::accumulate<ValueTypes, mpl::int_<0>, mpl::if_<IsConvertible<T, mpl::_2>, mpl::next<mpl::_1>, mpl::_1>>::type::value;
	};

	template<class... Ts> struct SetColumn {
		const Row<Ts...>& from;
		Row& to;
		template<class T> void operator()(const T& type) {
			to.col<T::type>() = from.col<T::type>();
		}
	};

public:
	Row(TValues&&... values) : refs(std::forward<TValues>(values)...) {}

	Row(const Row& other) = default;

	Row& operator=(const Row& other) = delete;

	template<class T, typename std::enable_if<!std::is_reference<T>::value && mpl::contains<ValueTypes, T>::type::value, int>::type = 0>
	explicit operator T() { return col<StoredType<T>>(); }

	template<class T, typename std::enable_if<HasCompatibleCol<T>::value, int>::type = 0>
	Row& operator |=(T x) {
		static_assert(NumCompatibleCols<T>::value == 1, "Ambiguous column assign. Value is implicitly convertible to multiple columns.");
		col<typename mpl::deref<typename mpl::find_if<ValueTypes, IsConvertible<T, mpl::_1>>::type>::type>() = x; return *this;
	}

	template<class... Ts> Row& operator|=(const Row<Ts...>& other) {
		using OtherTypes = typename mpl::set<typename std::remove_reference<Ts>::type...>::type;
		using Intersection = typename mpl::remove_if<ValueTypes, mpl::not_<mpl::contains<OtherTypes, mpl::_1>>>::type;
		mpl::for_each<Intersection, TypeWrap<mpl::_1>>(SetColumn<Ts...>{ other, *this });
		return *this;
	}

	template<class T> const T& col() const {
		return get<StoredType<typename std::remove_reference<T>::type>>(refs);
	}
	template<class T, typename std::enable_if<mpl::contains<Types, T&>::type::value, int>::type = 0>
	T& col() {
		return get<StoredType<typename std::remove_reference<T>::type>>(refs);
	}

	template<class T>
	T& unsafeCol() {
		return get<StoredType<typename std::remove_reference<T>::type>>(refs);
	}

	template<class... Ts>
	Row<StoredType<typename std::remove_reference<Ts>::type>...> cols() {
		return Row<StoredType<typename std::remove_reference<Ts>::type>...>(col<Ts>()...);
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

template<class TRow, class T> struct ExtendRow;
template<class... TValues, class T> struct ExtendRow<Row<TValues...>, T> {
	using type = Row<TValues..., T>;
};

template<class TRow1, class TRow2> struct RowUnion;
template<class... TValues1, class... TValues2> class RowUnion<Row<TValues1...>, Row<TValues2...>> {
	using AllValues = typename mpl::vector<TValues1..., TValues2...>::type;
	using Unique = typename mpl::unique<AllValues, std::is_same<mpl::_1, mpl::_2>>::type;
public:
	using type = typename mpl::fold<Unique, Row<>, ExtendRow<mpl::_1, mpl::_2>>::type;
};