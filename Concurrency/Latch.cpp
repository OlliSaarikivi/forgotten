#include <Core\stdafx.h>
#include "Latch.h"

Latch::Latch(uint32_t count) : counter{ count } {
	if (count == 0)
		isZero.signal();
}

void Latch::assertLockFree() {
	if (!counter.is_lock_free())
		FORGOTTEN_ERROR(L"Event is not lock-free on this architecture. DWCAS not supported?");
}

void Latch::wait() {
	isZero.wait();
}

void Latch::countDown() {
	auto oldCount = counter.fetch_sub(1);
	if (oldCount == 1)
		isZero.signal();
}
