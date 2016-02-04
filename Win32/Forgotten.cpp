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

#define ROWS 100

NUMERIC_COL(uint32_t, Eid)

COL(double, PosX)
COL(double, PosY)
COL(uint16_t, Strength)

NAME(Physical)
NAME(Stats)

static Universe<Eid,
	Component<Physical, PosX, PosY>,
	Component<Stats, Strength >> entities;

fibers::condition_variable cond;
fibers::mutex mtx;
static bool entitiesAdded = false;

void addEntities() {
	auto insertStat = entities.component<Stats>().inserter();
	auto insertPhysical = entities.component<Physical>().inserter();
	for (Eid i(0u); i < ROWS; ++i) {
		auto id = entities.newId();
		insertStat(makeRow(id, Strength(10)));
		if (i % 2 == 0)
			insertPhysical(makeRow(id, PosX(i * 0.1), PosY(1.0 / (i + 1))));
	}
	{
		std::unique_lock<fibers::mutex> lk(mtx);
		entitiesAdded = true;
	}
	cond.notify_all();
}

void updateEntities() {
	{
		std::unique_lock<fibers::mutex> lk(mtx);
		while (!entitiesAdded) {
			cond.wait(lk);
		}
	}
	forEach(entities.require<Stats, Physical>(), [&](auto entity) {
		(entity >> Stats_ >> Strength_) *= 2;
		std::cout << "Entity " << (entity >> Stats_ >> Eid_) << " updated\n";
	});
}

int _tmain(int argc, _TCHAR* argv[]) {

	fibers::fiber f1(addEntities);
	fibers::fiber f2(updateEntities);

	f2.join();

	string line;
	std::cin >> line;
}
