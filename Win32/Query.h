#pragma once

#include "MergeJoin.h"
#include "NamedRows.h"
#include "FindJoin.h"

namespace impl {
	template<class TTable> struct TableRef {
		TTable& table;

		auto begin() { return ::begin(table); }
		auto end() { return ::end(table); }
	};

	template<class TTable> auto makeTableRef(TTable& table) {
		return TableRef<TTable>{ table };
	}

	template<template<typename> class TName, class T> struct WithName {
		T source;

		auto begin() { return NamingIterator<TName, decltype(source.begin())>{ source.begin() }; }
		auto end() { return source.end(); }
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
		using GetKey = NamedIndex;
		static const Key LeastKey = TIndex::LeastKey;
		template<class TNamedRows> Key operator()(const TNamedRows& rows) {
			return typename TIndex::GetKey()(rows.row<TName>());
		}
	};

	template<class TLeft, class TRight> struct JoinBuilder {
		TLeft left;
		TRight right;

		template<class TIndex, template<typename> class TLeftName, template<typename> class TRightName> auto byMerge() {
			return QueryBuilder<MergeJoin<NamedIndex<TLeftName, TIndex>, NamedIndex<TRightName, TIndex>, TLeft, TRight>>{ { left, right } };
		}
	};

	template<class TLeft, class TRight> auto makeJoinBuilder(TLeft left, TRight right) {
		return JoinBuilder<TLeft, TRight>{ left, right };
	}

	template<class TLeft> struct QueryBuilder {
		TLeft left;

		template<template<typename> class TName> auto as() {
			return QueryBuilder<WithName<TName, TLeft>>{ { left } };
		}

		template<class TRight> auto join(TRight right) {
			return makeJoinBuilder(left, right);
		}

		TLeft select() {
			return left;
		}
	};
}

template<class TTable> auto from(TTable& table) {
	return impl::QueryBuilder<impl::TableRef<TTable>>{ impl::makeTableRef(table) };
}
