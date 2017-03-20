#include <Core\stdafx.h>
#include "Event.h"

void Event::assertLockFree() {
	if (!signaled.is_lock_free())
		FORGOTTEN_ERROR(L"Event is not lock-free on this architecture.");
}

void Event::wait() {
	if (!signaled.load()) {
		auto yieldContinuation = createSchedulerContinuation([this](Task* currentTask) {
			waiting.push(currentTask);
			if (signaled.load()) {
				auto nextTask = waiting.pop();
				if (nextTask)
					executeContext(nextTask);
			}
		});
		CHECK_WIN32("UmsThreadYield", UmsThreadYield(&yieldContinuation));
	}
}

void Event::signal() {
	signaled.store(true);
	Task* task;
	while ((task = waiting.pop())) {
		enqueueContext(task);
	}
}
