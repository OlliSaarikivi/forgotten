#include "stdafx.h"

#include <tchar.h>

#include <random>

#include "BTree.h"
#include "Join.h"

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
	using KeyType = int;
	using Less = std::less<KeyType>;
	static const KeyType LeastKey = INT_MIN;
	template<class TRow> KeyType operator()(const TRow& row) {
		return row.col<int>();
	}
};

template<class TFunc> void Time(string name, const TFunc& f) {
	Timer tmr; tmr.reset(); f(); double t = tmr.elapsed(); std::cout << name << ": " << t << "s\n";
}

#define ROWS 10000000

COL(int, Ac)
COL(uint64_t, Bc)
COL(uint64_t, Bc2)
COL(uint64_t, Bc3)
COL(char, Cc)

int _tmain(int argc, _TCHAR* argv[]) {
	std::default_random_engine e1(0);
	std::uniform_int_distribution<int> randomInt(0, INT_MAX - 1);

	Columnar<int, uint64_t> randomInsert{};
	for (int i = 0; i < ROWS; ++i) {
		randomInsert.pushBack(makeRow(i, uint64_t(i)));
	}
	Columnar<int, uint64_t> randomSubset{};
	for (int i = 0; i < ROWS; ++i) {
		if ((randomInt(e1) % 100) > 90)
			randomSubset.pushBack(randomInsert[ROWS - 1 - i]);
	}

	std::cout << randomInsert.size() << " " << randomSubset.size() << "\n";

	for (;;) {
		BTree<ExtractIntCol, int, uint64_t> table{};
		for (auto row : randomInsert) {
			table.insert(row);
		}

		BTree<ExtractIntCol, int, uint64_t> subsetTable{};
		for (auto row : randomSubset) {
			subsetTable.insert(row);
		}

		uint64_t sum;
		for (int i = 0; i < 10; ++i) {
			Time("scan", [&]() {
				for (auto row : table) {
					sum += row.col<uint64_t>();
				}
			});
		}
		for (int i = 0; i < 10; ++i) {
			Time("join", [&]() {
				auto iter = mergeJoin(table, subsetTable);
				while (iter != End{}) {
					auto row = *iter;
					sum += row.col<uint64_t>();
					++iter;
				}
			});
		}

		std::cout << sum;
		string line;
		std::cin >> line;
	}
}