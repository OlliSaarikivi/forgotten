#include "stdafx.h"

#include <tchar.h>

#include "Table.h"

void foo(int& x) {
	x = 3;
}

int _tmain(int argc, _TCHAR* argv[]) {
	Table<int, long> a{};
	auto b = a.begin<int, long>();
	auto c = a.begin<int, long>();
	swap(b, c);
	auto x = *b++;
	++c;
	b->col<int>() = 5;
	bool d1 = b == c;
	bool d2 = b != c;
	bool d3 = b < c;
	bool d4 = b > c;
	bool d5 = b <= c;
	bool d6 = b >= c;
	b += 5;
	auto e = b + 5;
	auto f = 5 + b;
	auto y = f[9];
}