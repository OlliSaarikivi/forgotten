#include "stdafx.h"

#include <tchar.h>

#include <random>

#include "BTree.h"
#include "MergeJoin.h"
#include "FindJoin.h"

#include "ThreeLevelBTree.h"
#include "LazyTree.h"

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

struct IntColIndex {
	using Key = int;
	using GetKey = IntColIndex;
	static const Key LeastKey = INT_MIN;
	template<class TRow> Key operator()(const TRow& row) {
		return row.col<int>();
	}
};

template<class TFunc> void Time(string name, const TFunc& f) {
	Timer tmr; tmr.reset(); f(); double t = tmr.elapsed(); std::cout << name << ": " << t << "s\n";
}

template<class TIter> void checkSum(TIter rangeBegin, TIter rangeEnd) {
	int count = 0;
	int sum = 0;
	int orderless = 0;
	int sign = 1;
	while (rangeBegin != rangeEnd) {
		auto value = int(*rangeBegin++);
		sum += sign * value;
		sign = -sign;
		orderless += value;
		++count;
	}
	std::cout << "Checksum: " << sum << "\t Orderless: " << orderless << "\tCount: " << count << "\n";
}

#define ROWS 1000000

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
	for (int i = randomInsert.size() - 1; i > 0; --i) {
		int j = randomInt(e1) % (i + 1);
		swap(randomInsert[j], randomInsert[i]);
	}
	Columnar<int, uint64_t> randomSubset{};
	for (int i = 0; i < ROWS; ++i) {
		if ((randomInt(e1) % 1000) > 900)
			randomSubset.pushBack(randomInsert[i]);
	}
	for (int i = randomSubset.size() - 1; i > 0; --i) {
		int j = randomInt(e1) % (i + 1);
		swap(randomSubset[j], randomSubset[i]);
	}

	for (;;) {

		map<int, uint64_t> map{};

		Time("map insert", [&]() {
			for (auto row : randomInsert) {
				map.emplace(int(row), uint64_t(row));
			}
		});

		BTree<ExtractIntCol, int, uint64_t> table{};

		Time("orig insert", [&]() {
			for (auto row : randomInsert) {
				table.insert(row);
			}
		});

		LazyTree<IntColIndex, int, uint64_t> lazyTable{};

		Time("new insert", [&]() {
			for (auto row : randomInsert) {
				lazyTable.insert(row);
			}
		});

		uint64_t sum;
		for (int i = 0; i < 5; ++i) {
			Time("map scan", [&]() {
				for (auto row : map) {
					sum += row.second;
				}
			});
		}

		for (int i = 0; i < 5; ++i) {
			Time("orig scan", [&]() {
				for (auto row : table) {
					sum += uint64_t(row);
				}
			});
		}

		for (int i = 0; i < 5; ++i) {
			Time("lazy scan", [&]() {
				for (auto row : lazyTable) {
					sum += uint64_t(row);
				}
			});
		}

		Time("map erase", [&]() {
			for (auto row : randomSubset) {
				map.erase(int(row));
			}
		});

		Time("orig erase", [&]() {
			for (auto row : randomSubset) {
				table.erase(row);
			}
		});

		Time("lazy erase", [&]() {
			for (auto row : randomSubset) {
				lazyTable.erase(row);
			}
		});

		for (int i = 0; i < 5; ++i) {
			Time("lazy scan", [&]() {
				for (auto row : lazyTable) {
					sum += uint64_t(row);
				}
			});
		}

		checkSum(begin(table), end(table));
		checkSum(begin(lazyTable), end(lazyTable));


		string line;
		std::cin >> line;
		std::cout << sum << "\n";
	}
}