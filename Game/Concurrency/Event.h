#pragma once

#include "Task.h"
#include "Scheduler.h"

struct Event {
	void assertLockFree();

	void wait();
	void signal();

private:
	ConcurrentTaskStack waiting;
	std::atomic<bool> signaled = ATOMIC_FLAG_INIT;
};