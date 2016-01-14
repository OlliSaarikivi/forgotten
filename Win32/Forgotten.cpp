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

		vector<Row<Ac, Bc, Bc2, Bc3, Cc>> vec{};
		TIME("vec_build",
			vec.reserve(ROWS);
			for (int i = 0; i < ROWS; ++i) {
				vec.push_back(makeRow(Ac(i), Bc(i), Bc2(i), Bc3(i), Cc(i)));
			}
		);
		TIME("vec_sum",
			int sum = 0;
		for (auto& row : vec)
			sum += Ac(row);
		std::cout << sum << "\n";
		);

		Columnar<Ac, Bc, Bc2, Bc3, Cc> mine{};
		TIME("mineBuild",
			mine.reserve(ROWS);
			for (int i = 0; i < ROWS; ++i) {
				mine.pushBack(makeRow(Ac(i), Bc(i), Bc2(i), Bc3(i), Cc(i)));
			}
		);
		TIME("mineSum",
			int sum = 0;
		for (auto& row : mine)
			sum += Ac(row);
		std::cout << sum << "\n";
		);

		string line;
		std::cin >> line;
	}
}