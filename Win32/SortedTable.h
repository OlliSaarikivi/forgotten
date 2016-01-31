#pragma once

#include "MBPlusTree.h"

template<class TIndex, class... TValues> class SortedTable : public MBPlusTree<TIndex, TValues...> {

	struct Inserter {
		SortedTable& table;

		Inserter(SortedTable& table) : table(table) {}
		~Inserter() {
			table.insert(::begin(table.toInsert), ::end(table.toInsert));
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
			table.erase(::begin(table.toErase), ::end(table.toErase));
			table.toErase.clear();
		}

		template<class TRow> void erase(const TRow& row) {
			table.toErase.insert(row);
		}
	};

	Columnar<TValues...> toInsert;
	MBPlusTree<TIndex, TValues...> toErase;
public:

	Inserter inserter() {
		return Inserter{ *this };
	}

	Eraser eraser() {
		return Eraser{ *this };
	}
};
