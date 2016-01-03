#include "stdafx.h"

#include <tchar.h>

#include "BTree.h"

struct ExtractIntCol {
	using KeyType = int32_t;
	template<class TRow> KeyType operator()(const TRow& row) {
		return row.col<int>();
	}
};

struct IntColLess {
	template<class TLeft, class TRight> bool operator()(const TLeft& left, const TRight& right) {
		return left.col<int>() < right.col<int>();
	}
};

int _tmain(int argc, _TCHAR* argv[]) {
	//int z = 5;
	//auto y = makeRow(z, (short)10);
	//auto a = Columnar<int, short>{};
	//a.pushBack(y);
	//auto b = a[0];
	//b.set<short>(3);
	//b.col<short>() = 3;

	//auto t = BTree<ExtractIntCol, std::less<ExtractIntCol::KeyType>, IntColLess, int, long>{};
	//auto h = t.begin();
	//t.insert(makeRow((int)3, (long)4));

	int a;
	short b;
	auto r = makeRow(a, b);

	(int&)r = 3;
}