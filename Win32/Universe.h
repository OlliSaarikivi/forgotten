#pragma once

#include "Name.h"
#include "Table.h"
#include "Index.h"
#include "Query.h"

template<class TId, class... TComponents> class Universe {
	using Index = ColIndex<TId>;

	using Components = typename mpl::vector<TComponents...>::type;

	template<class T> struct ComponentTable;
	template<template<typename> class TName, class... TValues> struct ComponentTable<TName<Row<TValues...>>> {
		using type = Table<TName, Index, TId, TValues...>;
	};

	template<template<typename> class TName> struct NameToIndex {
		using type = mpl::distance<mpl::begin<Components>,
			typename mpl::find_if<Components, typename IsFromTemplate1<TName>::template Check<mpl::_1>>::type>;
	};

	template<size_t... Indices> struct MakeJoin;
	template<> struct MakeJoin<> {};
	template<size_t Index> struct MakeJoin<Index> {
		auto make(tuple<typename ComponentTable<TComponents>::type...>& tables) {
			return from(get<Index>(tables));
		}
	};
	template<size_t Index1, size_t Index2, size_t... Indices> struct MakeJoin<Index1, Index2, Indices...> {
		auto make(tuple<typename ComponentTable<TComponents>::type...>& tables) {
			return from(get<Index1>(tables)).join(MakeJoin<Index2, Indices...>().make(tables)).merge();
		}
	};

	template<class TMakeJoin, class T> struct ExtendMakeJoin;
	template<size_t... Indices, class T> struct ExtendMakeJoin<MakeJoin<Indices...>, T> {
		using type = MakeJoin<Indices..., T::value>;
	};

	tuple<typename ComponentTable<TComponents>::type...> tables;

public:
	template<template<typename> class... TNames> auto begin() {
		using Indices = typename mpl::vector<NameToIndex<TNames>::type...>::type;
		using ThisJoin = typename mpl::fold<Indices, MakeJoin<>, ExtendMakeJoin<mpl::_1, mpl::_2>>::type;
		return ThisJoin().make(tables).begin();
	}

	auto begin() {
		using Indices = typename mpl::range_c<size_t, 0, sizeof...(TComponents)>::type;
		using ThisJoin = typename mpl::fold<Indices, MakeJoin<>, ExtendMakeJoin<mpl::_1, mpl::_2>>::type;
		return ThisJoin().make(tables).begin();
	}

	auto end() {
		return End{};
	}
};
