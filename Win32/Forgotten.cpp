#include "stdafx.h"

#include <tchar.h>

#include "BTree.h"

struct ExtractIntCol {
	using KeyType = int32_t;
	using Less = std::less<KeyType>;
	static const KeyType LeastKey = 0;
	template<class TRow> KeyType operator()(const TRow& row) {
		return row.col<int>();
	}
};

struct IntColLess {
	template<class TLeft, class TRight> bool operator()(const TLeft& left, const TRight& right) {
		return left.col<int>() < right.col<int>();
	}
};

COL(int, Speed)
COL(int, Acceleration)
COL(int, Weight)
COL(unique_ptr<int>, Resource)

int _tmain(int argc, _TCHAR* argv[]) {
	BTree<ExtractIntCol, IntColLess, int, long> table{};
	Columnar<int, long> add{};

	for (int i = 0; i < 10; ++i) {
		for (int j = 0; j < 10; ++j) {
			add.pushBack(makeRow(j*10 + i, (long)i));
		}

		table.moveInsertSorted(begin(add), end(add));
		add.clear();
	}

	auto iter = begin(table);
	auto tableEnd = end(table);
	while (iter != tableEnd) {
		std::cout << int(*iter) << " " << long(*iter) << "\n";
		++iter;
	}

	for (auto row : table) {
		std::cout << int(row) << " " << long(row) << "\n";
	}

	string line;
	std::cin >> line;
}