#pragma once

#include "MBPlusTree.h"
#include "Name.h"

template<template<typename> class TName, class TIndex, class... TValues> class FlatTable {
	using Index = TIndex;
	using Key = typename Index::Key;
	using GetKey = Index;

	struct Inserter {
		FlatTable& table;

		Inserter(FlatTable& table) : table(table) {}

		template<class TRow> void operator()(const TRow& row) {
			table.insert(row);
		}
	};

	Columnar<TValues...> rows;
public:

	auto begin() { return NamingIterator<TName, decltype(rows.begin())>{ rows.begin() }; }
	auto end() { return rows.end(); }

	Inserter inserter() {
		return Inserter{ *this };
	}

	template<class TRow> void insert(const TRow& row) {
		assert(rows.size() == 0 || GetKey()(row) >= GetKey()(rows[rows.size() - 1]));
		rows.pushBack(row);
	}

	void clear() {
		rows.clear();
	}
};

template<template<typename> class TName, class TIndex, class... TValues> struct NameOf<FlatTable<TName, TIndex, TValues...>> {
	template<class Task>
	using type = TName<Task>;
};

template<class Task> struct IndexOf;
template<template<typename> class TName, class TIndex, class... TValues> struct IndexOf<FlatTable<TName, TIndex, TValues...>> {
	using type = TIndex;
};