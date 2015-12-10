#pragma once

#include "Row.h"

template<typename... TColumnProxies>
struct RowProxy : TColumnProxies...
{
	template<typename... TInitializers>
	RowProxy(TInitializers&... inits) : TColumnProxies(inits)... {}

	template<typename T, typename R>
	R& ExpandPackIgnore(R& r) { return r; }

	template<typename TInitializer>
	static RowProxy ConstructFromOne(TInitializer& init)
	{
		return RowProxy{ExpandPackIgnore<TColumnProxies, TInitializer>(init)...};
	}

	/*template<typtypename... TColumnProxies>
	struct ConstructProxyFromOneHelper
	{
		template<typename THead, typename... TTail>
		static RowProxy Construct(THead& head, TTail&... tail)
		{
			return ConstructProxyFromOneHelper::
		}
	};*/
};

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

template<typename TProxy, typename TConst, typename TRow>
struct ProxySelector;
template<typename... TConstColumns, typename TCol, typename... TPCols>
struct ProxySelector<RowProxy<TPCols...>, Row<TConstColumns...>, Row<TCol>>
{
	using ConstsHelper = ContainsHelper<TConstColumns...>;
	using type = RowProxy<TPCols..., typename TCol::template ProxyTypeHelper<ConstsHelper::template Actual<TCol>::value>::type>;
};
template<typename... TConstColumns, typename TCol, typename... TCols, typename... TPCols>
struct ProxySelector<RowProxy<TPCols...>, Row<TConstColumns...>, Row<TCol, TCols...>>
{
	using ConstsHelper = ContainsHelper<TConstColumns...>;
	using type = typename ProxySelector < RowProxy<TPCols..., typename TCol::template ProxyTypeHelper<ConstsHelper::template Actual<TCol>::value>::type>,
		Row<TConstColumns...>, Row < TCols... >> ::type;
};