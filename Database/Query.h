#pragma once

#include "FindJoin.h"
#include "FindOuterJoin.h"
#include "FindAntiJoin.h"
#include "MergeJoin.h"
#include "MergeOuterJoin.h"
#include "MergeAntiJoin.h"
#include "NamedRows.h"
#include "FindJoin.h"

#define QUERY_PASSTHROUGH(Q) \
	auto begin() { return Q.begin(); } \
	auto end() { return Q.end(); } \
	auto finder() { return Q.finder(); } \
	auto finderFail() { return Q.finderFail(); }

namespace impl {
	template<class TTable> struct TableRef {
		TTable& table;

		auto begin() { using std::begin; return AsNamedRowsIterator<decltype(begin(table))>(begin(table)); }
		auto end() { using std::end; return end(table); }
		auto finder() { return AsNamedRowsFinder<decltype(table.finder())>(table.finder()); }
		auto finderFail() { return table.finderFail(); }
	};

	template<template<typename> class TName, class TTable> struct AliasTableRef {
		TTable& table;

		auto begin() { using std::begin; return AsNamedRowsIterator<NamingIterator<TName, decltype(begin(table))>>{ { begin(table) } }; }
		auto end() { using std::end; return end(table); }
		auto finder() { return AsNamedRowsFinder<NamingFinder<TName, decltype(table.finder())>>{ { table.finder() } }; }
		auto finderFail() { return table.finderFail(); }
	};

	template<class TLeftIndex, class TLeft, class TRight,
		template<typename, typename, typename, typename, typename> class TIter>
	struct FindJoin {
		TLeft left;
		TRight right;

		auto begin() {
			return TIter
				<TLeftIndex, decltype(left.begin()), decltype(left.end()), decltype(right.finder()), decltype(right.finderFail())>
				(left.begin(), left.end(), right.finder(), right.finderFail());
		}

		auto end() {
			return End{};
		}
	};

	template<class TLeftIndex, class TRightIndex, class TLeft, class TRight,
		template<typename, typename, typename, typename, typename, typename> class TIter>
	struct MergeJoin {
		TLeft left;
		TRight right;

		auto begin() {
			return TIter
				<TLeftIndex, TRightIndex, decltype(left.begin()), decltype(left.end()), decltype(right.begin()), decltype(right.end())>
				(left.begin(), left.end(), right.begin(), right.end());
		}

		auto end() {
			return End{};
		}
	};

	template<template<typename> class TName, class TIndex> struct NamedIndex {
		using Key = typename TIndex::Key;
		template<class TNamedRows> Key operator()(const TNamedRows& rows) {
			return TIndex()(rows.c<TName>());
		}
	};

	template<class TLeft, class TRight,
		template<typename, typename, typename, typename, typename> class TFindJoin,
		template<typename, typename, typename, typename, typename, typename> class TMergeJoin>
	struct JoinBuilder {
		TLeft left;
		TRight right;

		auto merge() {
			return Query<MergeJoin<
				NamedIndex<NameOf<TLeft>::type, IndexOf<TLeft>::type>,
				NamedIndex<NameOf<TRight>::type, IndexOf<TRight>::type>,
				TLeft, TRight, TMergeJoin>>
			{ { left, right } };
		}

		auto find() {
			return Query<FindJoin<
				NamedIndex<NameOf<TLeft>::type, IndexOf<TLeft>::type>,
				TLeft, TRight, TFindJoin>>
			{ { left, right } };
		}
	};

	template<class TLeft> struct Query {
		TLeft left;

		Query(TLeft left) : left(left) {}

		template<template<typename> class TName> auto on() {
			return Query<Scope<TName, TLeft>>{ { left } };
		}

		template<class TIndex> auto index() {
			return Query<IndexAssumption<TIndex, TLeft>>{ { left } };
		}

		template<class TRight> auto join(TRight right) {
			return JoinBuilder<TLeft, TRight, FindJoinIterator, MergeJoinIterator>{ left, right };
		}

		template<class TRight> auto outerJoin(TRight right) {
			return JoinBuilder<TLeft, TRight, FindOuterJoinIterator, MergeOuterJoinIterator>{ left, right };
		}

		template<class TRight> auto antiJoin(TRight right) {
			return JoinBuilder<TLeft, TRight, FindAntiJoinIterator, MergeAntiJoinIterator>{ left, right };
		}

		QUERY_PASSTHROUGH(left)
	};

	template<template<typename> class TName, class Task> struct Scope {
		Task query;
		QUERY_PASSTHROUGH(query)
	};

	template<class TIndex, class Task> struct IndexAssumption {
		Task query;
		QUERY_PASSTHROUGH(query)
	};

	template<template<typename> class TName> struct AliasBuilder {
		template<class TTable> auto operator()(TTable& table) {
			return impl::Query<impl::AliasTableRef<TName, TTable>>{ { table } };
		}
	};
}

template<class TTable> auto from(TTable& table) {
	return impl::Query<impl::TableRef<TTable>>{ { table } };
}

template<template<typename> class TName> auto alias() {
	return impl::AliasBuilder<TName>{};
}

// NameOf specializations
template<class TTable> struct NameOf<impl::TableRef<TTable>> : NameOf<TTable> {};

template<template<typename> class TName, class TTable> struct NameOf<impl::AliasTableRef<TName, TTable>> {
	template<class Task>
	using type = TName<Task>;
};

template<class TLeftIndex, class TLeft, class TRight, template<typename, typename, typename, typename, typename> class TIter>
struct NameOf<impl::FindJoin<TLeftIndex, TLeft, TRight, TIter>> : NameOf<TLeft> {};

template<class TLeftIndex, class TRightIndex, class TLeft, class TRight>
struct NameOf<impl::MergeJoin<TLeftIndex, TRightIndex, TLeft, TRight, MergeJoinIterator>> : NameOf<TRight> {};

template<class TLeftIndex, class TRightIndex, class TLeft, class TRight>
struct NameOf<impl::MergeJoin<TLeftIndex, TRightIndex, TLeft, TRight, MergeOuterJoinIterator>> : NameOf<TLeft> {};

template<class TLeftIndex, class TRightIndex, class TLeft, class TRight>
struct NameOf<impl::MergeJoin<TLeftIndex, TRightIndex, TLeft, TRight, MergeAntiJoinIterator>> : NameOf<TLeft> {};

template<class TLeft> struct NameOf<impl::Query<TLeft>> : NameOf<TLeft> {};

template<template<typename> class TName, class Task> struct NameOf<impl::Scope<TName, Task>> {
	template<class Task>
	using type = TName<Task>;
};

template<class TIndex, class TLeft> struct NameOf<impl::IndexAssumption<TIndex, TLeft>> : NameOf<TLeft> {};

// IndexOf specializations
template<class TTable> struct IndexOf<impl::TableRef<TTable>> : IndexOf<TTable> {};

template<template<typename> class TName, class TTable> struct IndexOf<impl::AliasTableRef<TName, TTable>> : IndexOf<TTable> {};

template<class TLeftIndex, class TLeft, class TRight, template<typename, typename, typename, typename, typename> class TIter>
struct IndexOf<impl::FindJoin<TLeftIndex, TLeft, TRight, TIter>> : IndexOf<TLeft> {};

template<class TLeftIndex, class TRightIndex, class TLeft, class TRight>
struct IndexOf<impl::MergeJoin<TLeftIndex, TRightIndex, TLeft, TRight, MergeJoinIterator>> : IndexOf<TRight> {};

template<class TLeftIndex, class TRightIndex, class TLeft, class TRight>
struct IndexOf<impl::MergeJoin<TLeftIndex, TRightIndex, TLeft, TRight, MergeOuterJoinIterator>> : IndexOf<TLeft> {};

template<class TLeftIndex, class TRightIndex, class TLeft, class TRight>
struct IndexOf<impl::MergeJoin<TLeftIndex, TRightIndex, TLeft, TRight, MergeAntiJoinIterator>> : IndexOf<TLeft> {};

template<class TLeft> struct IndexOf<impl::Query<TLeft>> : IndexOf<TLeft> {};

template<template<typename> class TName, class Task> struct IndexOf<impl::Scope<TName, Task>> : IndexOf<Task> {};

template<class TIndex, class TLeft> struct IndexOf<impl::IndexAssumption<TIndex, TLeft>> {
	using type = TIndex;
};
