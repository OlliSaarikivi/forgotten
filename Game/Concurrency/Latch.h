#pragma once

#include "Event.h"

struct Latch {
	Latch(uint32_t count);

	void assertLockFree();

	void wait();
	void countDown();
private:
	std::atomic<uint32_t> counter;
	Event isZero;
};