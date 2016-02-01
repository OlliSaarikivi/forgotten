#include "stdafx.h"

#include <tchar.h>

#include <random>

#include "MBPlusTree.h"
#include "MergeJoin.h"
#include "FindJoin.h"

#include "SortedTable.h"
#include "Query.h"

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
	Timer tmr; tmr.reset(); f(); double t = tmr.elapsed(); std::cout << t << "s\t" << name << "\n";
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

#define ROWS 1000

COL(int, Ac)
COL(uint64_t, Bc)
COL(uint64_t, Bc2)
COL(uint64_t, Bc3)
COL(char, Cc)

NAME(SomeRow)
NAME(OtherRow)

int _tmain(int argc, _TCHAR* argv[]) {
	std::default_random_engine e1(0);
	std::uniform_int_distribution<int> randomInt(0, INT_MAX - 1);

	Columnar<int, uint64_t> randomInsert{};
	Columnar<int, uint64_t> sortedInsert{};
	Columnar<int, uint64_t> sortedInterleaved{};
	for (int i = 0; i < ROWS; ++i) {
		randomInsert.pushBack(makeRow(i * 2, uint64_t(i)));
		sortedInsert.pushBack(makeRow(i * 2, uint64_t(i)));
		sortedInterleaved.pushBack(makeRow(i + 1, uint64_t(i)));
	}
	Columnar<int, uint64_t> randomSubset{};
	Columnar<int, uint64_t> sortedSubset{};
	for (int i = 0; i < ROWS; ++i) {
		if ((randomInt(e1) % 1000) > 900) {
			randomSubset.pushBack(sortedInsert[i]);
			sortedSubset.pushBack(sortedInsert[i]);
		}
	}
	for (int i = randomInsert.size() - 1; i > 0; --i) {
		int j = randomInt(e1) % (i + 1);
		swap(randomInsert[j], randomInsert[i]);
	}
	for (int i = randomSubset.size() - 1; i > 0; --i) {
		int j = randomInt(e1) % (i + 1);
		swap(randomSubset[j], randomSubset[i]);
	}

	int x = 3;
	double y = 0.1;
	NamedRows<SomeRow<Row<int&>>> a{ makeRow(x) };
	NamedRows<OtherRow<Row<double&>>> b{ makeRow(y) };
	auto c = concatNamedRows(a, b);

	SortedTable<IntColIndex, int, uint64_t> table{};
	auto query = from(sortedSubset).as<SomeRow>()
		.join(from(table).as<OtherRow>().select())
			.byMerge<IntColIndex, SomeRow, OtherRow>().select();

	for (;;) {
		uint64_t sum;

		table.clear();
		std::cout << randomInsert.size() << "\n";
		Time("table insert", [&]() {
			auto inserter = table.inserter();
			for (auto row : randomInsert) {
				inserter.insert(row);
			}
		});
		std::cout << table.size() << "\n";
		checkSum(begin(table), end(table));
		checkSum(begin(sortedInsert), end(sortedInsert));

		Time("table merge join", [&]() {
			auto iter = query.begin();
			while (iter != query.end()) {
				sum += uint64_t(iter->row<SomeRow>());
				++iter;
			}
		});

		Time("table erase", [&]() {
			auto eraser = table.eraser();
			for (auto row : sortedSubset) {
				eraser.erase(row);
			}
		});

		string line;
		std::cin >> line;
		std::cout << sum << "\n";
	}
}