#pragma once

namespace impl {
	template<template<typename> class TTemplate>
	struct IsFromTemplate {
		template<class T> struct Check : mpl::bool_<false> {};
		template<class TArg> struct Check<TTemplate<TArg>> : mpl::bool_<true> {};
	};

	template<class T> struct GetOnlyArgument;
	template<template<typename> class TTemplate, class TArg> struct GetOnlyArgument<TTemplate<TArg>> {
		using type = TArg;
	};
}

template<class... TNames> class NamedRows {
	using NameTypes = typename mpl::vector<TNames...>::type;

	template<template<typename> class TName>
	struct TypeForTemplate {
		using type = typename mpl::deref<typename mpl::find_if<NameTypes, typename impl::IsFromTemplate<TName>::template Check<mpl::_1>>::type>::type;
	};

	template<class TName>
	using RowType = typename impl::GetOnlyArgument<TName>::type;
	template<template<typename> class TName>
	using RowForName = RowType<typename TypeForTemplate<TName>::type>;

	tuple<TNames...> rowsTuple;

	template<class TNamed, class T> auto asNamed(const T& t) {
		return TNamed(t);
	}

public:
	NamedRows(const RowType<TNames>&... rows)
		: rowsTuple(asNamed<TNames, RowType<TNames>>(rows)...) {}

	template<template<typename> class TName> auto& row() {
		using Row = RowForName<TName>;
		return static_cast<Row&>(get<TName<Row>>(rowsTuple));
	}

	template<template<typename> class TName> const auto& row() const {
		using Row = RowForName<TName>;
		return static_cast<const Row&>(get<TName<Row>>(rowsTuple));
	}

	template<class TName> const auto& row() const {
		using Row = RowType<TName>;
		return static_cast<const Row&>(get<TName>(rowsTuple));
	}
};

template<class TNamedRows1, class TNamedRows2> struct NamedRowsConcat;
template<class... TNames1, class... TNames2> struct NamedRowsConcat<NamedRows<TNames1...>, NamedRows<TNames2...>> {
	using type = NamedRows<TNames1..., TNames2...>;
};

template<class... TNames1, class... TNames2> auto concatNamedRows(const NamedRows<TNames1...>& left, const NamedRows<TNames2...>& right) {
	return NamedRows<TNames1..., TNames2...>(left.row<TNames1>()..., right.row<TNames2...>());
}

template<class TIter> class AsNamedRowsIterator {
	TIter iter;

public:
	using reference = NamedRows<typename TIter::reference>;

	AsNamedRowsIterator(TIter iter) : iter(iter) {}

	AsNamedRowsIterator& operator++()
	{
		++iter;
		return *this;
	}
	AsNamedRowsIterator operator++(int) {
		auto old = *this;
		this->operator++();
		return old;
	}

	reference operator*() const {
		return reference{ *iter };
	}
	FauxPointer<reference> operator->() const {
		return FauxPointer<reference>{ this->operator*() };
	}

	template<class TSentinel> friend bool operator==(const AsNamedRowsIterator& iter, const TSentinel& sentinel) {
		return iter.iter == sentinel;
	}
	template<class TSentinel> friend bool operator!=(const AsNamedRowsIterator& iter, const TSentinel& sentinel) {
		return iter.iter != sentinel;
	}

	friend void swap(AsNamedRowsIterator& left, AsNamedRowsIterator& right) {
		using std::swap;
		swap(left.iter, right.iter);
	}
};