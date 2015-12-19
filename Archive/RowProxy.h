#pragma once

#include "Row.h"

namespace impl {
	template<typename T, typename R>
	R& ExpandPackIgnore(R& r) { return r; }

	template<typename T, typename... TList>
	struct Contains;
	template<typename T>
	struct Contains<T> { static const bool value = false; };
	template<typename T, typename THead, typename... TRest>
	struct Contains<T, THead, TRest...>
	{
		static const bool value = std::is_same<T, THead>::value || Contains<T, TRest...>::value;
	};

	template<typename... TList>
	struct ContainsHelper
	{
		template<typename T>
		using Actual = Contains<T, TList...>;
	};

	template<typename TConsts, typename TCol>
	struct ProxySelector;
	template<typename... TConstCols, typename TCol>
	struct ProxySelector<Row<TConstCols...>, TCol>
	{
		using ConstsHelper = ContainsHelper<TConstCols...>;
		using type = typename TCol::template ProxyTypeHelper<ConstsHelper::template Actual<TCol>::value>::type;
	};
}

template<typename TRow, typename TConsts>
struct RowProxy;
template<typename... TCols, typename... TConsts>
struct RowProxy<Row<TCols...>, Row<TConsts...>> : impl::ProxySelector<Row<TConsts...>, TCols>::type...
{
	using RowType = Row<TCols...>;
	using ConstsType = Row<TConsts...>;
	template<typename TCol>
	using ColProxyType = typename impl::ProxySelector<Row<TConsts...>, TCol>::type;

	template<typename... TInitializers>
	RowProxy(TInitializers&... inits) : ColProxyType<TCols>(inits)... {}
	template<typename... TLefts, typename... TRights>
	RowProxy(pair<TLefts, TRights>&... inits) : ColProxyType<TCols>(inits.first, inits.second)... {}

	template<typename TInitializer>
	static RowProxy ConstructFromOne(TInitializer&& init)
	{
		return RowProxy{ impl::ExpandPackIgnore<TColumnProxies, TInitializer>(std::forward<TInitializer>(init))... };
	}

	template<typename TLeft, typename TRight>
	static RowProxy ConstructFromTwo(TLeft&& left, TRight&& right)
	{
		return RowProxy{ pair<impl::ExpandPackIgnore<TColumnProxies, TLeft>(std::forward<TLeft>(left)), impl::ExpandPackIgnore<TColumnProxies, TRight>(std::forward<TRight>(right))>... };
	}
};
