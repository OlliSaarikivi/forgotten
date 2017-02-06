#include <Core\stdafx.h>
#include "Mutex.h"

void Mutex::assertLockFree() {
	if (!count.is_lock_free() || !handOff.is_lock_free())
		FORGOTTEN_ERROR(L"Mutex is not lock-free on this architecture. DWCAS not supported?");
}

void Mutex::lock() {
	auto oldCount = count.fetch_add(1);
	if (oldCount != 0) {
		auto yieldContinuation = createSchedulerContinuation([this](Task* currentTask) {
			waiting.push(currentTask);
			auto old = handOff.load();
			if (old.thread != NotASchedulerThread && waiting.peek())
				if (handOff.compare_exchange_strong(old, { NotASchedulerThread, static_cast<uint16_t>(old.version + 1) })) {
					auto nextTask = waiting.pop();
					executeContext(nextTask);
				}
		});
		CHECK_WIN32(L"UmsThreadYield", UmsThreadYield(&yieldContinuation));
	}
}

bool Mutex::tryLock() {
	CounterType zero = 0;
	if (count.compare_exchange_strong(zero, 1))
		return true;
	else {
		auto old = handOff.load();
		if (old.thread == NotASchedulerThread)
			return false;
		if (handOff.compare_exchange_strong(old, { NotASchedulerThread, static_cast<uint16_t>(old.version + 1) })) {
			count.fetch_add(1);
			return true;
		}
	}
	return false;
}

void Mutex::unlock() {
	auto oldCount = count.fetch_sub(1);
	if (oldCount != 1) {
		for (;;) {
			auto nextTask = waiting.pop();
			if (nextTask) {
				enqueueContext(nextTask);
				return;
			} else {
				auto newHandOff = TaggedSchedulerThreadIndex{ currentSchedulerThread(), static_cast<uint16_t>(handOff.load().version + 1) };
				handOff.store(newHandOff);
				if (!waiting.peek())
					return;
				if (!handOff.compare_exchange_strong(newHandOff, { NotASchedulerThread, static_cast<uint16_t>(newHandOff.version + 1) }))
					return;
			}
		}
	}
}
