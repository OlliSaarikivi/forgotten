#pragma once

#include "MergeJoin.h"
#include "NamedRows.h"
#include "FindJoin.h"

namespace impl {
	template<class TTable> struct TableRef {
		TTable& table;

		auto begin() { using std::begin; return AsNamedRowsIterator<decltype(begin(table))>(begin(table)); }
		auto end() { using std::end; return end(table); }
	};

	template<class TTable> auto makeTableRef(TTable& table) {
		return TableRef<TTable>{ table };
	}

	template<template<typename> class TName, class TTable> struct AliasTableRef {
		TTable& table;

		auto begin() { using std::begin; return AsNamedRowsIterator<NamingIterator<TName, decltype(begin(table))>>{ { begin(table) } }; }
		auto end() { using std::end; return end(table); }
	};

	template<template<typename> class TName, class T> struct WithName {
		T query;

		auto begin() { return AsNamedRowsIterator<TName, decltype(source.begin())>{ query.begin() }; }
		auto end() { return query.end(); }
	};

	template<class TLeftIndex, class TRightIndex, class TLeft, class TRight> struct MergeJoin {
		TLeft left;
		TRight right;

		auto begin() {
			return MergeJoinIterator
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

	template<class TLeft, class TRight> struct JoinBuilder {
		TLeft left;
		TRight right;

		auto merge() {
			return Query<MergeJoin<
				NamedIndex<NameOf<TLeft>::type, IndexOf<TLeft>::type>,
				NamedIndex<NameOf<TRight>::type, IndexOf<TRight>::type>,
				TLeft, TRight>>
			{ { left, right } };
		}
	};

	template<class TLeft, class TRight> auto makeJoinBuilder(TLeft left, TRight right) {
		return JoinBuilder<TLeft, TRight>{ left, right };
	}

	template<class TLeft> struct Query {
		TLeft left;

		template<template<typename> class TName> auto on() {
			return Query<Scope<TName, TLeft>>{ { left } };
		}

		template<class TIndex> auto index() {
			return Query<IndexAssumption<TIndex, TLeft>>{ { left } };
		}

		template<class TRight> auto join(TRight right) {
			return makeJoinBuilder(*this, right);
		}

		auto begin() { return left.begin(); }
		auto end() { return left.end(); }
	};

	struct QueryStart {
		template<class TRight> auto join(TRight right) {
			return Query<TRight>{ right };
		}
	};

	template<template<typename> class TName, class T> struct Scope {
		T query;

		auto begin() { return query.begin(); }
		auto end() { return query.end(); }
	};

	template<class TIndex, class T> struct IndexAssumption {
		T query;

		auto begin() { return query.begin(); }
		auto end() { return query.end(); }
	};

	template<template<typename> class TName> struct AliasBuilder {
		template<class TTable> auto from(TTable& table) {
			return impl::Query<impl::AliasTableRef<TName, TTable>>{ { table } };
		}
	};
}

template<class TTable> auto from(TTable& table) {
	return impl::Query<impl::TableRef<TTable>>{ impl::makeTableRef(table) };
}

template<template<typename> class TName> auto alias() {
	return impl::AliasBuilder<TName>{};
}

auto query() {
	return impl::QueryStart{};
}

// NameOf specializations
template<class TTable> struct NameOf<impl::TableRef<TTable>> : NameOf<TTable> {};

template<template<typename> class TName, class TTable> struct NameOf<impl::AliasTableRef<TName, TTable>> {
	template<class T>
	using type = TName<T>;
};

template<class TLeftIndex, class TRightIndex, class TLeft, class TRight>
struct NameOf<impl::MergeJoin<TLeftIndex, TRightIndex, TLeft, TRight>> : NameOf<TRight> {};

template<class TLeft> struct NameOf<impl::Query<TLeft>> : NameOf<TLeft> {};

template<template<typename> class TName, class T> struct NameOf<impl::Scope<TName, T>> {
	template<class T>
	using type = TName<T>;
};

template<class TIndex, class TLeft> struct NameOf<impl::IndexAssumption<TIndex, TLeft>> : NameOf<TLeft> {};

// IndexOf specializations
template<class TTable> struct IndexOf<impl::TableRef<TTable>> : IndexOf<TTable> {};

template<template<typename> class TName, class TTable> struct IndexOf<impl::AliasTableRef<TName, TTable>> : IndexOf<TTable> {};

template<class TLeftIndex, class TRightIndex, class TLeft, class TRight>
struct IndexOf<impl::MergeJoin<TLeftIndex, TRightIndex, TLeft, TRight>> : IndexOf<TRight> {};

template<class TLeft> struct IndexOf<impl::Query<TLeft>> : IndexOf<TLeft> {};

template<template<typename> class TName, class T> struct IndexOf<impl::Scope<TName, T>> : IndexOf<T> {};

template<class TIndex, class TLeft> struct IndexOf<impl::IndexAssumption<TIndex, TLeft>> {
	using type = TIndex;
};