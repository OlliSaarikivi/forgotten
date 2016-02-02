#pragma once

#include "MBPlusTree.h"
#include "Name.h"

template<template<typename> class TName, class TIndex, class... TValues> class SortedTable {

	struct Inserter {
		SortedTable& table;

		Inserter(SortedTable& table) : table(table) {}
		~Inserter() {
			table.tree.insert(::begin(table.toInsert), ::end(table.toInsert));
			table.toInsert.clear();
		}

		template<class TRow> void insert(const TRow& row) {
			table.toInsert.pushBack(row);
		}
	};

	struct Eraser {
		SortedTable& table;

		Eraser(SortedTable& table) : table(table) {}
		~Eraser() {
			table.tree.erase(::begin(table.toErase), ::end(table.toErase));
			table.toErase.clear();
		}

		template<class TRow> void erase(const TRow& row) {
			table.toErase.insert(row);
		}
	};

	MBPlusTree<TIndex, TValues...> tree;
	Columnar<TValues...> toInsert;
	MBPlusTree<TIndex, TValues...> toErase;
public:

	auto begin() { return NamingIterator<TName, decltype(tree.begin())>{ tree.begin() }; }
	auto end() { return tree.end(); }

	Inserter inserter() {
		return Inserter{ *this };
	}

	Eraser eraser() {
		return Eraser{ *this };
	}

	void clear() {
		tree.clear();
	}
};

template<template<typename> class TName, class TIndex, class... TValues> struct NameOf<SortedTable<TName, TIndex, TValues...>> {
	template<class T>
	using type = TName<T>;
};

template<class T> struct IndexOf;
template<template<typename> class TName, class TIndex, class... TValues> struct IndexOf<SortedTable<TName, TIndex, TValues...>> {
	using type = TIndex;
};