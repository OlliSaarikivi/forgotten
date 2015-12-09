#pragma once

#include "Row.h"

template<typename... TColumnProxies>
struct RowProxy : TColumnProxies...
{
	template<typename... TInitializers>
	RowProxy(TInitializers&... inits) : TColumnProxies(inits)... {}
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
struct ContainsHelper {
	template<typename T>
	using Actual = Contains<T, TList...>;
};

template<typename... TConstColumns>
struct ProxySelector
{
	using ConstsHelper = ContainsHelper<TConstColumns...>;
	template<typename... TColumns>
	using type = RowProxy<typename TColumns::template ProxyTypeHelper<ConstsHelper::template Actual<TColumns>::value>::type...>;
};
