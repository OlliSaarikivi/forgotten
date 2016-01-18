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

#define TIME(N,B) { Timer tmr; tmr.reset(); B; double t = tmr.elapsed(); std::cout << N << ": " << t << "s\n"; }

#define ROWS 10000000

COL(int, Ac)
COL(uint64_t, Bc)
COL(uint64_t, Bc2)
COL(uint64_t, Bc3)
COL(char, Cc)

int _tmain(int argc, _TCHAR* argv[]) {
	Columnar<int, uint64_t> randomInsert{};
	for (int i = 0; i < ROWS; ++i) {
		randomInsert.pushBack(makeRow(rand(), uint64_t(i)));
	}
	//Columnar<int, uint64_t> randomDelete{};
	//for (int i = 0; i < ROWS; ++i) {
	//	//if (rand() % 100 > 10)
	//		randomDelete.pushBack(randomInsert[ROWS-1-i]);
	//}

	//std::cout << randomDelete.size() << "\n";

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
	table.printStats();
	table.validate();

	int64_t greatSum = 0;
	for (;;) {
		Timer tmr;

		TIME("map_update",
			for (auto& entry : map) {
				entry.second *= entry.first;
			}
		);

		//TIME("map_delete",
		//	for (auto row : randomDelete) {
		//		map.erase(int(row));
		//	}
		//);

		TIME("tableUpdate",
			for (auto row : table) {
				row.col<uint64_t>() *= int(row);
			}
		);

		//TIME("tableDelete",
		//	for (auto row : randomDelete) {
		//		table.erase(row);
		//	}
		//);
		//table.printStats();
		//table.validate();

		string line;
		std::cin >> line;
		std::cout << greatSum << "\n";
	}
}