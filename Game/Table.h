#pragma once

#include "MBPlusTree.h"
#include "Name.h"

template<template<typename> class TName, class TIndex, class... TValues> class Table {

	struct Inserter {
		Table& table;

		Inserter(Table& table) : table(table) {}
		~Inserter() {
			table.tree.insert(table.toInsert.begin(), table.toInsert.end());
			table.toInsert.clear();
		}

		template<class TRow> void operator()(const TRow& row) {
			table.toInsert.pushBack(row);
		}
	};

	struct Eraser {
		Table& table;

		Eraser(Table& table) : table(table) {}
		~Eraser() {
			table.tree.erase(table.toErase.begin(), table.toErase.end());
			table.toErase.clear();
		}

		template<class TRow> void operator()(const TRow& row) {
			table.toErase.insert(row);
		}
	};

	MBPlusTree<TIndex, TValues...> tree;
	Columnar<TValues...> toInsert;
	MBPlusTree<TIndex, TValues...> toErase;
public:

	auto begin() { return NamingIterator<TName, decltype(tree.begin())>{ tree.begin() }; }
	auto end() { return tree.end(); }
	auto finder() { return NamingFinder<TName, decltype(tree.finder())>{ tree.finder() }; }
	auto finderFail() { return tree.finderFail(); }

	Inserter inserter() {
		return Inserter{ *this };
	}

	Eraser eraser() {
		return Eraser{ *this };
	}

	auto& rows() {
		return tree;
	}

	void clear() {
		tree.clear();
	}
};

template<template<typename> class TName, class TIndex, class... TValues> struct NameOf<Table<TName, TIndex, TValues...>> {
	template<class T>
	using type = TName<T>;
};

template<class T> struct IndexOf;
template<template<typename> class TName, class TIndex, class... TValues> struct IndexOf<Table<TName, TIndex, TValues...>> {
	using type = TIndex;
};
