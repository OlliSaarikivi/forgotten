#pragma once

template<class T, class TRow> T& col(TRow row) { return row.col<T>(); }

template<class... TValues> class Row {
	using ValueTypes = typename mpl::vector<typename std::remove_reference<TValues>::type...>::type;
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

	template<class TFrom> struct SetCol {
		const TFrom& from;
		Row& to;
		template<class T> void operator()(const T& type) {
			to.col<T::type>() = from.col<T::type>();
		}
	};

	template<class T> const T& col() const {
		return get<StoredType<typename std::remove_reference<T>::type>>(refs);
	}
	template<class T> T& col() {
		return get<StoredType<typename std::remove_reference<T>::type>>(refs);
	}

public:
	Row(TValues... values) : refs(values...) {}

	template<class T, typename std::enable_if<mpl::contains<ValueTypes, T>::type::value, int>::type = 0>
	explicit operator const T&() const { return col<T>(); }
	template<class T, typename std::enable_if<mpl::contains<ValueTypes, T>::type::value, int>::type = 0>
	explicit operator T&() { return col<T>(); }

	template<class T, typename std::enable_if<HasCompatibleCol<T>::value, int>::type = 0>
	Row& operator |=(T x) {
		static_assert(NumCompatibleCols<T>::value == 1, "Ambiguous column assign. Value is implicitly convertible to multiple columns.");
		col<typename mpl::deref<typename mpl::find_if<ValueTypes, IsConvertible<T, mpl::_1>>::type>::type>() = x; return *this;
	}
	template<class TTo, class TFrom> void set(TFrom x) { col<TTo>() = x; }

	template<class T> void setAll(const T& other) {
		mpl::for_each<ValueTypes, TypeWrap<mpl::_1>>(SetCol<T>{ other, *this });
	}
};

namespace impl {
	template<class... TParams> struct MakerRow {
		using type = Row<typename std::conditional<std::is_rvalue_reference<TParams>::value, typename std::remove_reference<TParams>::type, typename std::remove_reference<TParams>::type&>::type...>;
	};
}

template<class... Ts> auto makeRow(Ts&&... params) -> typename impl::MakerRow<decltype(params)...>::type {
	return impl::MakerRow<decltype(params)...>::type(std::forward<Ts>(params)...);
}