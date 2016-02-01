#pragma once

#include "MergeJoin.h"
#include "FindJoin.h"

namespace impl {
	template<class TTable> struct TableRef {
		TTable& table;

		auto begin() { return table.begin(); }
		auto end() { return table.end(); }
	};

	template<class TTable> auto makeTableRef(TTable& table) {
		return TableRef<TTable>{ table };
	}

	template<class TIndex, class TLeft, class TRight> struct MergeJoin {
		TLeft left;
		TRight right;

		auto begin() {
			return MergeJoinIterator
				<TIndex, decltype(::begin(left)), decltype(::end(left)), decltype(::begin(right)), decltype(::end(right))>
				(::begin(left), ::end(left), ::begin(right), ::end(right));
		}

		auto end() {
			return End{};
		}
	};

	template<class TLeft, class TRight> struct JoinBuilder {
		TLeft left;
		TRight right;

		template<class TIndex> auto byMerge() {
			return QueryBuilder<MergeJoin<TIndex, TLeft, TRight>>{ { left, right } };
		}
	};

	template<class TLeft, class TRight> auto makeJoinBuilder(TLeft left, TRight right) {
		return JoinBuilder<TLeft, TRight>{ left, right };
	}

	template<class TLeft> struct QueryBuilder {
		TLeft left;

		template<class TRight> auto join(TRight& right) {
			return makeJoinBuilder(left, makeTableRef(right));
		}

		TLeft select() {
			return left;
		}
	};
}

template<class TTable> auto from(TTable& table) {
	return impl::QueryBuilder<impl::TableRef<TTable>>{ impl::makeTableRef(table) };
}