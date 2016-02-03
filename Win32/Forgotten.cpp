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

#define ROWS 10000000

NUMERIC_COL(uint32_t, Eid)

COL(double, PosX)
COL(double, PosY)
COL(uint16_t, Strength)

NAME(Physical)
NAME(Stats)

int _tmain(int argc, _TCHAR* argv[]) {
	Universe<Eid, 
		Component<Physical, PosX, PosY>,
		Component<Stats, Strength>> entities;

	auto& stats = entities.component<Stats>();
	auto& physicals = entities.component<Physical>();

	Time("component insert", [&]() {
		auto insertStat = entities.component<Stats>().inserter();
		auto insertPhysical = entities.component<Physical>().inserter();
		for (Eid i(0u); i < ROWS; ++i) {
			auto id = entities.newId();
			insertStat(makeRow(id, Strength(10)));
			if (i % 2 == 0)
				insertPhysical(makeRow(id, PosX(i * 0.1), PosY(1.0 / (i + 1))));
		}
	});

	auto query = from(entities.component<Stats>()).join(alias<Stats>()(entities.component<Physical>())).find();
	auto blah = query.begin();

	int allTheStrength = 0;
	Time("exclude", [&]() {
		forEach(entities.require<Stats>().exclude<Physical>(), [&](auto entity) {
			allTheStrength += (entity >> Stats_ >> Strength_);
		});
	});

	Time("exclude", [&]() {
		forEach(entities.require<Stats>().exclude<Physical>(), [&](auto entity) {
			allTheStrength += (entity >> Stats_ >> Strength_);
		});
	});

	Time("require both", [&]() {
		forEach(entities.require<Stats, Physical>(), [&](auto entity) {
			allTheStrength += (entity >> Stats_ >> Strength_);
		});
	});

	Time("include", [&]() {
		auto eraseEntity = entities.eraser();
		forEach(entities.require<Stats>().include<Physical>(), [&](auto entity) {
			allTheStrength += (entity >> Stats_ >> Strength_);
		});
	});

	Time("just require", [&]() {
		auto eraseEntity = entities.eraser();
		forEach(entities.require<Stats>(), [&](auto entity) {
			allTheStrength += (entity >> Stats_ >> Strength_);
		});
	});

	string line;
	std::cin >> line;
	std::cout << allTheStrength;
}
