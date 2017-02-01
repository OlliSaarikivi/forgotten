#include <Core\stdafx.h>
#include "Scheduler.h"
#include <Core\Utils.h>

const DWORD DequeueTimeout = 50;

#define MAX_SCHEDULER_THREADS 8
static_assert(MAX_SCHEDULER_THREADS < std::numeric_limits<SchedulerThreadIndex>::max());
static size_t numSchedulerThreads;
static thread_local SchedulerThreadIndex threadIndex;

template<class T>
using PerThreadArray = array<T, MAX_SCHEDULER_THREADS>;

static PerThreadArray<std::minstd_rand> randomEngines;
static PerThreadArray<PUMS_COMPLETION_LIST> completionLists;

struct ScalableTaskBag {
	void put(Task* task) {
		localQueues[threadIndex].pushLeft(task);
	}
	Task* get() {
		return localQueues[threadIndex].popLeft();
	}
	Task* steal() {
		array<SchedulerThreadIndex, MAX_SCHEDULER_THREADS - 1> victims;
		for (SchedulerThreadIndex i = 0; i < threadIndex; ++i) {
			victims[i] = i;
		}
		for (SchedulerThreadIndex i = threadIndex + 1; i < numSchedulerThreads; ++i) {
			victims[i - 1] = i;
		}
		std::shuffle(victims.begin(), victims.begin() + (numSchedulerThreads - 1),
			randomEngines[threadIndex]);
	}
private:
	PerThreadArray<ConcurrentTaskDeque> localQueues;
};

static ScalableTaskBag readyContexts;
static ScalableTaskBag newTasks;

Task* getTaskForUmsContext(PUMS_CONTEXT context) {
	PVOID threadInformation;
	CHECK_WIN32(QueryUmsThreadInformation(context, UmsThreadUserContext, &threadInformation, sizeof(threadInformation), NULL));
	return static_cast<Task*>(threadInformation);
}

void __stdcall schedulerFunction(UMS_SCHEDULER_REASON reason, ULONG_PTR activationPayload, PVOID schedulerParam) {
	switch (reason) {
	case UmsSchedulerStartup:
		break;
	case UmsSchedulerThreadBlocked:
		break;
	case UmsSchedulerThreadYield:
		auto yieldingContext = reinterpret_cast<PUMS_CONTEXT>(activationPayload);
		auto continuation = static_cast<SchedulerContinuation*>(schedulerParam);
		auto yieldingTask = getTaskForUmsContext(yieldingContext);
		(*continuation)(yieldingTask);
		break;
	}

}

DWORD becomeSchedulerThread(LPVOID param) {
	static_assert(sizeof(LPVOID) >= sizeof(decltype(threadIndex)));
	threadIndex = reinterpret_cast<decltype(threadIndex)>(param);

	victims[threadIndex] = (threadIndex + 1) % numSchedulerThreads;
	CreateUmsCompletionList(&completionLists[threadIndex]);

	UMS_SCHEDULER_STARTUP_INFO umsInfo;
	umsInfo.UmsVersion = UMS_VERSION;
	umsInfo.CompletionList = completionLists[threadIndex];
	umsInfo.SchedulerProc = &schedulerFunction;
	umsInfo.SchedulerParam = nullptr;

	for (;;) {
		CHECK_WIN32(EnterUmsSchedulingMode(&umsInfo));

		PUMS_CONTEXT contextList;
		if (!DequeueUmsCompletionListItems(completionLists[threadIndex], DequeueTimeout, &contextList)) {
			if (GetLastError() == ERROR_TIMEOUT) {
				// TODO: anything to do?
				continue;
			} else CHECK_WIN32(false);
		}
		while (contextList != NULL) {
			auto task = getTaskForUmsContext(contextList);
			enqueueContext(task);
			contextList = GetNextUmsListItem(contextList);
		}
	}

	return 0; // TODO: something meanignful
}

SchedulerThreadIndex currentSchedulerThread() {
	return threadIndex;
}

void startScheduler() {

	becomeSchedulerThread(reinterpret_cast<LPVOID>(0));

	/*
		auto numThreads = std::thread::hardware_concurrency();
		if (numThreads <= 1)
			numThreads = 2;

		for (decltype(numThreads) index = 0; index < numThreads; ++index) {
			CreateThread(NULL, 0, &becomeSchedulerThread, reinterpret_cast<LPVOID>(index), 0, NULL);
		}*/
}

void executeContext(Task* task) {
	while (!ExecuteUmsThread(task->context)) {
		CHECK_WIN32(GetLastError() == ERROR_RETRY);
	}
}

void enqueueContext(Task* task) {
	readyContexts.put(task);
}

void enqueueTask(Task* task) {
	newTasks.put(task);
}