#include <Core\stdafx.h>
#include "Scheduler.h"
#include <Core\Utils.h>

#define MAX_SCHEULER_THREADS 8
static const DWORD DequeueTimeout = 50;

struct CACHE_ALIGN SchedulerThread {
	PUMS_COMPLETION_LIST completionList;
	TaskQueue readyQueue;
};

static SchedulerThread schedulerThreads[MAX_SCHEULER_THREADS];
static thread_local unsigned threadIndex;

inline SchedulerThread& schedulerThread(unsigned index = threadIndex) {
	return schedulerThreads[threadIndex];
}

Task* getTaskForUmsContext(PUMS_CONTEXT context) {
	PVOID threadInformation;
	CHECK(QueryUmsThreadInformation(context, UmsThreadUserContext, &threadInformation, sizeof(threadInformation), NULL));
	return static_cast<Task*>(threadInformation);
}

void __stdcall schedulerFunction(UMS_SCHEDULER_REASON reason, ULONG_PTR activationPayload, PVOID SchedulerParam) {
	switch (reason) {
	case UmsSchedulerStartup:
		break;
	case UmsSchedulerThreadBlocked:
		break;
	case UmsSchedulerThreadYield:
		// auto yieldingContext = reinterpret_cast<PUMS_CONTEXT>(activationPayload);
		// Find something new to execute
		break;
	}
}

DWORD becomeSchedulerThread(LPVOID param) {
	static_assert(sizeof(LPVOID) >= sizeof(decltype(threadIndex)));
	threadIndex = reinterpret_cast<decltype(threadIndex)>(param);
	auto thread = schedulerThread();

	CreateUmsCompletionList(&thread.completionList);

	UMS_SCHEDULER_STARTUP_INFO umsInfo;
	umsInfo.UmsVersion = UMS_VERSION;
	umsInfo.CompletionList = thread.completionList;
	umsInfo.SchedulerProc = &schedulerFunction;
	umsInfo.SchedulerParam = nullptr;

	CHECK(!EnterUmsSchedulingMode(&umsInfo));

	PUMS_CONTEXT contextList;
	for (;;) {
		if (!DequeueUmsCompletionListItems(thread.completionList, DequeueTimeout, &contextList)) {
			if (GetLastError() == ERROR_TIMEOUT) {
				// TODO: anything to do?
				continue;
			} else CHECK(false);
		}
		while (contextList != NULL) {
			auto task = getTaskForUmsContext(contextList);

		}
	}

	return 0; // TODO: something meanignful
}

void startScheduler() {

	becomeSchedulerThread(reinterpret_cast<LPVOID>(0));

	std::cout << "Blah\n";
	/*
		auto numThreads = std::thread::hardware_concurrency();
		if (numThreads == 0)
			numThreads = 1;

		for (decltype(numThreads) index = 0; index < numThreads; ++index) {
			CreateThread(NULL, 0, &becomeSchedulerThread, reinterpret_cast<LPVOID>(index), 0, NULL);
		}*/
}

void runTask(unique_ptr<Task> task) {

}