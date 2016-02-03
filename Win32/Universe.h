#pragma once

#include "Name.h"
#include "Table.h"
#include "Index.h"
#include "Query.h"

template<template<typename> class TName, class... TValues> struct Component {};

template<class TId, class... TComponents> class Universe {
	using Index = ColIndex<TId>;

	template<class T> struct WrappedNameOf;
	template<template<typename> class TName, class... TValues> struct WrappedNameOf<Component<TName, TValues...>> {
		using type = NameWrapper<TName>;
	};

	using Components = typename mpl::vector<typename WrappedNameOf<TComponents>::type...>::type;

	template<class T> struct ComponentTable;
	template<template<typename> class TName, class... TValues> struct ComponentTable<Component<TName, TValues...>> {
		using type = Table<TName, Index, TId, TValues...>;
	};

	template<template<typename> class TName> struct NameToIndex {
		using type = mpl::distance<typename mpl::begin<Components>::type,
			typename mpl::find_if<Components, std::is_same<mpl::_1, NameWrapper<TName>>>::type>;
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

	template<size_t... Indices> struct MakeOuterJoin;
	template<> struct MakeOuterJoin<> {};
	template<size_t Index> struct MakeOuterJoin<Index> {
		template<class TQuery> auto make(TQuery query, tuple<typename ComponentTable<TComponents>::type...>& tables) {
			return query.outerJoin(from(get<Index>(tables))).merge();
		}
	};
	template<size_t Index1, size_t Index2, size_t... Indices> struct MakeOuterJoin<Index1, Index2, Indices...> {
		template<class TQuery> auto make(TQuery query, tuple<typename ComponentTable<TComponents>::type...>& tables) {
			return MakeOuterJoin<Index2, Indices...>().make(query, tables).outerJoin(from(get<Index1>(tables))).merge();
		}
	};

	template<size_t... Indices> struct MakeAntiJoin;
	template<> struct MakeAntiJoin<> {};
	template<size_t Index> struct MakeAntiJoin<Index> {
		template<class TQuery> auto make(TQuery query, tuple<typename ComponentTable<TComponents>::type...>& tables) {
			return query.antiJoin(from(get<Index>(tables))).merge();
		}
	};
	template<size_t Index1, size_t Index2, size_t... Indices> struct MakeAntiJoin<Index1, Index2, Indices...> {
		template<class TQuery> auto make(TQuery query, tuple<typename ComponentTable<TComponents>::type...>& tables) {
			return MakeAntiJoin<Index2, Indices...>().make(query, tables).antiJoin(from(get<Index1>(tables))).merge();
		}
	};

	template<class TMakeJoin, class T> struct ExtendMakeJoin;
	template<size_t... Indices, class T> struct ExtendMakeJoin<MakeJoin<Indices...>, T> {
		using type = MakeJoin<Indices..., T::value>;
	};

	template<class TMakeJoin, class T> struct ExtendMakeOuterJoin;
	template<size_t... Indices, class T> struct ExtendMakeOuterJoin<MakeOuterJoin<Indices...>, T> {
		using type = MakeOuterJoin<Indices..., T::value>;
	};

	template<class TMakeJoin, class T> struct ExtendMakeAntiJoin;
	template<size_t... Indices, class T> struct ExtendMakeAntiJoin<MakeAntiJoin<Indices...>, T> {
		using type = MakeAntiJoin<Indices..., T::value>;
	};

	template<template<typename> class... TNames> struct ComponentIncludes {
		Universe& universe;

		template<class TQuery> auto in(TQuery outerQuery) {
			using Indices = typename mpl::vector<typename NameToIndex<TNames>::type...>::type;
			using ThisJoin = typename mpl::fold<Indices, MakeOuterJoin<>, ExtendMakeOuterJoin<mpl::_1, mpl::_2>>::type;
			auto query = ThisJoin().make(outerQuery, universe.tables);
			return impl::UniverseQuery<Universe, decltype(query)>{ universe, query };
		}
	};

	template<template<typename> class... TNames> struct ComponentExcludes {
		Universe& universe;

		template<class TQuery> auto in(TQuery outerQuery) {
			using Indices = typename mpl::vector<typename NameToIndex<TNames>::type...>::type;
			using ThisJoin = typename mpl::fold<Indices, MakeAntiJoin<>, ExtendMakeAntiJoin<mpl::_1, mpl::_2>>::type;
			auto query = ThisJoin().make(outerQuery, universe.tables);
			return impl::UniverseQuery<Universe, decltype(query)>{ universe, query };
		}
	};

	struct Eraser {
		Universe& universe;

		struct DoErase {
			Universe& universe;

			template<class T> void operator()(const T& type) {
				get<typename T::type>(universe.tables).rows().erase(universe.toErase.begin(), universe.toErase.end());
			}
		};

		Eraser(Universe& universe) : universe(universe) {}
		~Eraser() {
			using TableTypes = typename mpl::vector<typename ComponentTable<TComponents>::type...>::type;
			mpl::for_each<TableTypes, TypeWrap<mpl::_1>>(DoErase{ universe });
			universe.toErase.clear();
		}

		template<class TRow> void operator()(const TRow& row) {
			universe.toErase.pushBack(row);
		}
	};

	tuple<typename ComponentTable<TComponents>::type...> tables;
	Columnar<TId> toErase;
	TId nextId;

public:
	Universe() : nextId(MinKey<TId>()()) {}

	template<template<typename> class... TNames> auto require() {
		using Indices = typename mpl::vector<typename NameToIndex<TNames>::type...>::type;
		using ThisJoin = typename mpl::fold<Indices, MakeJoin<>, ExtendMakeJoin<mpl::_1, mpl::_2>>::type;
		auto query = ThisJoin().make(tables);
		return impl::UniverseQuery<Universe, decltype(query)>{ *this, query };
	}

	auto requireAll() {
		using Indices = typename mpl::range_c<size_t, 0, sizeof...(TComponents)>::type;
		using ThisJoin = typename mpl::fold<Indices, MakeJoin<>, ExtendMakeJoin<mpl::_1, mpl::_2>>::type;
		auto query = ThisJoin().make(tables);
		return impl::UniverseQuery<Universe, decltype(query)>{ *this, query };
	}

	template<template<typename> class... TNames> auto include() {
		return ComponentIncludes<TNames...>{ *this };
	}

	template<template<typename> class... TNames> auto exclude() {
		return ComponentExcludes<TNames...>{ *this };
	}

	template<template<typename> class TName> auto& component() {
		return get<NameToIndex<TName>::type::value>(tables);
	}

	auto eraser() {
		return Eraser{ *this };
	}

	TId newId() {
		assert(nextId != MaxKey<TId>()());
		return TId(nextId++);
	}
};

namespace impl {
	template<class TUniverse, class TLeft> struct UniverseQuery : Query<TLeft> {
		TUniverse& universe;

		UniverseQuery(TUniverse& universe, TLeft left) : Query<TLeft>(left), universe(universe) {}

		template<template<typename> class... TNames> auto include() {
			return universe.include<TNames...>().in(left);
		}

		template<template<typename> class... TNames> auto exclude() {
			return universe.exclude<TNames...>().in(left);
		}
	};
}