#pragma once

#define COL(T, D) BOOST_STRONG_TYPEDEF(T, D)

template<class... TValues> class Row {
public:
	using ValueTypes = typename mpl::vector<typename std::remove_reference<TValues>::type...>::type;
	using ValueTypeSet = typename mpl::set<typename std::remove_reference<TValues>::type...>::type;

private:
	using Types = typename mpl::set<TValues...>::type;

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

	struct SwapColumn {
		Row& left;
		Row& right;
		template<class T> void operator()(const T& type) {
			using std::swap;
			swap(left.unsafeCol<T::type>(), right.unsafeCol<T::type>());
		}
	};

public:
	template<class T> using HasCol = typename mpl::contains<ValueTypeSet, T>::type;

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

	auto temp() {
		return makeRow(std::remove_reference<TValues>::type(*this)...);
	}

	friend void swap(Row&& left, Row&& right) {
		mpl::for_each<ValueTypes, TypeWrap<mpl::_1>>(SwapColumn{ left, right });
	}
};

namespace impl {
	template<class... TParams> struct MakerRow {
		using type = Row<typename std::conditional<std::is_const<typename std::remove_reference<TParams>::type>::value || std::is_rvalue_reference<TParams>::value, typename std::remove_const<typename std::remove_reference<TParams>::type>::type, typename std::remove_reference<TParams>::type&>::type...>;
	};

	template<class TCol, class... TRows> struct SelectRow;
	template<class TCol, class TRow> struct SelectRow<TCol, TRow>
	{
		auto operator()(TRow&& t) {
			return std::forward<TRow>(t);
		}
	};
	template<class TCol, class TRow, class TRow2, class... TRows> struct SelectRow<TCol, TRow, TRow2, TRows...>
	{
		auto operator()(TRow&& t, TRow2&& t2, TRows&&... ts) {
			return mpl::contains<typename TRow::ValueTypeSet, TCol>::type::value ? t :
				SelectRow<TCol, TRow2, TRows...>()(std::forward<TRow2>(t2), std::forward<TRows>(ts)...);
		}
	};

	template<class TRow, class T> struct ExtendRow;
	template<class... TValues, class T> struct ExtendRow<Row<TValues...>, T> {
		using type = Row<TValues..., T>;
	};
}

template<class... Ts> auto makeRow(Ts&&... params) -> typename impl::MakerRow<decltype(params)...>::type {
	return impl::MakerRow<decltype(params)...>::type(std::forward<Ts>(params)...);
}

template<class TRow> struct JoinRows;
template<class... TValues> struct JoinRows<Row<TValues...>> {
	template<class... TRows> Row<TValues...> operator()(TRows&&... params) {
		return makeRow(impl::SelectRow<typename std::remove_reference<TValues>::type, TRows...>()(std::forward<TRows>(params)...)
			.col<typename std::remove_reference<TValues>::type>()...);
	}
};

template<class TRow1, class TRow2> struct RowUnion;
template<class... TValues1, class... TValues2> class RowUnion<Row<TValues1...>, Row<TValues2...>> {
	using Values = typename mpl::set<typename std::remove_reference<TValues1>::type..., typename std::remove_reference<TValues2>::type...>::type;
	using All = typename mpl::set<TValues1..., TValues2...>::type;
	template<class T> struct IsNotRef {
		using type = mpl::bool_<!std::is_reference<T>::value>;
	};
	using NonRefValues = typename mpl::copy_if<All, IsNotRef<mpl::_1>>::type;
	template<class T> struct ColType {
		using type = typename std::conditional<mpl::contains<NonRefValues, T>::type::value, T, T&>::type;
	};
	using Fixed = typename mpl::transform< Values, ColType<mpl::_1>, mpl::inserter<mpl::set<>::type, mpl::insert<mpl::_1, mpl::_2>> >::type;
public:
	using type = typename mpl::fold<Fixed, Row<>, impl::ExtendRow<mpl::_1, mpl::_2>>::type;
};