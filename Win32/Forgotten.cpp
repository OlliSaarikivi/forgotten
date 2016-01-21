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
	using KeyType = int32_t;
	using Less = std::less<KeyType>;
	static const KeyType LeastKey = 0;
	template<class TRow> KeyType operator()(const TRow& row) {
		return row.col<int>();
	}
};

#define TIME(N,B) { Timer tmr; tmr.reset(); B; double t = tmr.elapsed(); std::cout << N << ": " << t << "s\n"; }

#define ROWS 1000

COL(int, Ac)
COL(uint64_t, Bc)
COL(uint64_t, Bc2)
COL(uint64_t, Bc3)
COL(char, Cc)

int _tmain(int argc, _TCHAR* argv[]) {
	std::default_random_engine e1(0);
	std::uniform_int_distribution<int> randomInt(0, 1000 - 1);


	Columnar<int, uint64_t> randomInsert{};
	for (int i = 0; i < ROWS; ++i) {
		randomInsert.pushBack(makeRow(randomInt(e1), uint64_t(i)));
	}
	Columnar<int, uint64_t> randomDelete{};
	for (int i = 0; i < ROWS; ++i) {
		if ((randomInt(e1) % 100) > 10)
			randomDelete.pushBack(randomInsert[ROWS-1-i]);
	}

	std::cout << randomInsert.size() << " " << randomDelete.size() << "\n";

	for (;;) {

		map<int, uint64_t> map{};
		TIME("map_insert",
			for (auto row : randomInsert) {
				map.emplace(int(row), uint64_t(row));
			}
		);

		BTree<ExtractIntCol, int, uint64_t> table{};
		TIME("tableInsert",
			for (auto row : randomInsert) {
				table.insert(row);
			}
		);
		table.validate();
		table.printStats();

		//for (int i = 0; i < 10; ++i) {
		//	TIME("map_update",
		//		for (auto& entry : map) {
		//			entry.second *= entry.first;
		//		}
		//	);
		//}

		//for (int i = 0; i < 10; ++i) {
		//	TIME("tableUpdate",
		//		for (auto row : table) {
		//			row.col<uint64_t>() *= int(row);
		//		}
		//	);
		//}


		//for (int i = 0; i < 20; ++i) {
		//	auto iter = MergeJoinIterator<ExtractIntCol, decltype(begin(table)), End, decltype(begin(table2)), End>(begin(table), End{}, begin(table2), End{});
		//	while (iter != End{}) {
		//		++iter;
		//	}
		//}


		TIME("map_delete",
			for (auto row : randomDelete) {
				map.erase(int(row));
			}
		);

		{ Timer tmr; tmr.reset();
		BTree<ExtractIntCol, int, uint64_t> deleteTable{};
		for (auto row : randomDelete) {
			deleteTable.insert(row);
			//table.erase(row);
		}
		Columnar<int, uint64_t> reverse;
		for (auto row : deleteTable) {
			reverse.pushBack(row);
		}
		//for (auto row : deleteTable) {
		//	table.erase(row);
		//}
		table.eraseSorted(begin(deleteTable), end(deleteTable));
		//for (unsigned i = 0; i < reverse.size(); ++i) {
		//	table.erase(reverse[reverse.size() - 1 - i]);
		//}
		double t = tmr.elapsed(); std::cout << "tableDelete: " << t << "s\n"; }
		table.validate();
		table.printStats();



		for (int i = 0; i < 10; ++i) {
			TIME("map_update",
				for (auto& entry : map) {
					entry.second *= entry.first;
				}
			);
		}

		for (int i = 0; i < 10; ++i) {
			TIME("tableUpdate",
				for (auto row : table) {
					row.col<uint64_t>() *= int(row);
				}
			);
		}

		string line;
		std::cin >> line;
	}
}