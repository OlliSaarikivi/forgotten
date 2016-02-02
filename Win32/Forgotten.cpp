#include "stdafx.h"

#include <tchar.h>

#include <random>

#include "MBPlusTree.h"
#include "MergeJoin.h"
#include "FindJoin.h"

#include "Table.h"
#include "FlatTable.h"
#include "Query.h"
#include "Universe.h"

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

template<class TFunc> void Time(string name, const TFunc& f) {
	Timer tmr; tmr.reset(); f(); double t = tmr.elapsed(); std::cout << t << "s\t" << name << "\n";
}

#define ROWS 1000000

KEY_COL(uint32_t, Eid, 0u)

COL(double, PosX)
COL(double, PosY)
COL(uint16_t, Strength)

NAME(Physical)
NAME(Stats)

NAME(SomeRow)
NAME(OtherRow)
NAME(YetAnotherRow)

int _tmain(int argc, _TCHAR* argv[]) {
	Universe<Eid, 
		Physical<Row<PosX, PosY>>,
		Stats<Row<Strength>>> entities;

	forEach(entities, [&](auto rows) {
		(rows >> Physical_).c<PosX>() = 3.5;
	});
}