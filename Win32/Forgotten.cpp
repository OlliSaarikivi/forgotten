#include "stdafx.h"

#include <tchar.h>

#include "BTree.h"

class Timer
{
public:
	Timer() : beg_(clock_::now()) {}
	void reset() { beg_ = clock_::now(); }
	double elapsed() const {
		return boost::chrono::duration_cast<second_>
			(clock_::now() - beg_).count();
	}

private:
	typedef boost::chrono::high_resolution_clock clock_;
	typedef boost::chrono::duration<double, boost::ratio<1> > second_;
	boost::chrono::time_point<clock_> beg_;
};

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

int _tmain(int argc, _TCHAR* argv[]) {
	Timer tmr;

	BTree<ExtractIntCol, IntColLess, int> table{};
	Columnar<int> add{};

	tmr.reset();
	for (int i = 0; i < 100; ++i) {
		for (int j = 0; j < 10; ++j) {
			add.pushBack(makeRow(j*100 + i));
		}
		table.moveInsertSorted(begin(add), end(add));
		add.clear();
	}
	double t = tmr.elapsed();
	std::cout << t << std::endl;

	string line;
	std::cin >> line;
}