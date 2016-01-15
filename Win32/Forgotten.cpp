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

#define ROWS 1000000

COL(int, Ac)
COL(uint64_t, Bc)
COL(uint64_t, Bc2)
COL(uint64_t, Bc3)
COL(char, Cc)

int _tmain(int argc, _TCHAR* argv[]) {
	for (;;) {
		Timer tmr;

		vector<Row<Ac>> vec{};
		TIME("vec_build",
			vec.reserve(ROWS);
		for (int i = 0; i < ROWS; ++i) {
			vec.push_back(makeRow(Ac(i)));
		}
		);
		TIME("vec_sum",
			int sum = 0;
		for (auto& row : vec)
			sum += Ac(row);
		std::cout << sum << "\n";
		);

		Columnar<Ac> mine{};
		TIME("mineBuild",
			mine.reserve(ROWS);
		for (int i = 0; i < ROWS; ++i) {
			mine.pushBack(makeRow(Ac(i)));
		}
		);
		TIME("mineSum",
			int sum = 0;
		for (auto& row : mine)
			sum += Ac(row);
		std::cout << sum << "\n";
		);

		map<int, uint64_t> map{};
		TIME("map_build",
		for (int i = 0; i < ROWS/10; ++i) {
			for (int j = 0; j < 10; ++j) {
				map.emplace(j * ROWS/10 + i, uint64_t(j));
			}
		}
		);

		TIME("map_sum",
		uint64_t sum = 0;
		for (auto x : map) {
			sum += x.second;
		}
		std::cout << sum << std::endl;
		);

		BTree<ExtractIntCol, int, uint64_t> tableAppend{};
		TIME("tableBuildAppend",
			for (int i = 0; i < ROWS; ++i) {
				tableAppend.unsafeAppend(makeRow(i, uint64_t(i)));
			}
		);
		tableAppend.validate();

		BTree<ExtractIntCol, int, uint64_t> tableSorted{};
		Columnar<int, uint64_t> toAdd{};
		TIME("tableBuildSorted",
			for (int j = 0; j < 10; ++j) {
				for (int i = 0; i < ROWS / 10; ++i) {
					toAdd.pushBack(makeRow(j * ROWS / 10 + i, uint64_t(j)));
				}
				tableSorted.insertSorted(begin(toAdd), end(toAdd));
				toAdd.clear();
			}
		);
		tableSorted.validate();

		TIME("tableSumSorted",
			uint64_t sum = 0;
		for (auto x : tableSorted) {
			sum += uint64_t(x);
		}
		std::cout << sum << std::endl;
		);

		BTree<ExtractIntCol, int, uint64_t> table{};
		TIME("tableBuild",
		for (int i = 0; i < ROWS/10; ++i) {
			for (int j = 0; j < 10; ++j) {
				table.insert(makeRow(j * ROWS/10 + i, uint64_t(j)));
			}
		}
		);
		table.validate();

		TIME("tableSum",
		uint64_t sum = 0;
		for (auto x : table) {
			sum += uint64_t(x);
		}
		std::cout << sum << std::endl;
		);

		string line;
		std::cin >> line;
	}
}