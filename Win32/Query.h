#pragma once

#include "MergeJoin.h"
#include "FindJoin.h"

template<class TLeft, class TRight> struct Join {
	TLeft left;
	TRight right;

	auto begin() {
		return mergeJoin(left, right);
	}

	auto end() {
		return End{};
	}
};

template<class TLeft> struct QueryBuilder {
	TLeft left;

	template<class TRight> auto join(TRight& right) {
		return from(Join<TLeft, TRight>>{ left, std::ref(right) });
	}

	TLeft select() {
		return left;
	}
};

template<class TTable> auto from(TTable& table) {
	return QueryBuilder<TTable>{ std::ref(table) };
}