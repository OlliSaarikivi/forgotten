#pragma once

#include "Task.h"
#include "Scheduler.h"

struct alignas(4) TaggedSchedulerThreadIndex {
	SchedulerThreadIndex thread;
	uint16_t version;
};
static_assert(sizeof(TaggedSchedulerThreadIndex) == 4);

struct Mutex {
	void assertLockFree();

	void lock();
	bool tryLock();
	void unlock();

private:
	ConcurrentTaskStack waiting;
	std::atomic<TaggedSchedulerThreadIndex> handOff = TaggedSchedulerThreadIndex{ NotASchedulerThread, 0 };
	using CounterType = int;
	std::atomic<CounterType> count = 0;
};