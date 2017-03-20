#include <Core\stdafx.h>
#include "Scheduler.h"
#include <Core\Utils.h>

#define UMS_DEQUEUE_TIMEOUT 1

#define CONTEXT_POOL_CAPACITY 16

#define MAX_SCHEDULER_THREADS 8
static_assert(MAX_SCHEDULER_THREADS < std::numeric_limits<SchedulerThreadIndex>::max());
static DWORD numSchedulerThreads;
static thread_local SchedulerThreadIndex threadIndex;
static thread_local char scratch[65536];
static std::atomic<bool> keepRunning;

using TaskStealingDeque = BoundedConcurrentTaskDeque<65536>;

struct alignas(CACHE_LINE_SIZE)SchedulerThread {
	SchedulerThread() {}

	SchedulerThread(PUMS_COMPLETION_LIST completionList) :
		completionList{ completionList } {

		for (SchedulerThreadIndex i = 0; i < threadIndex; ++i) {
			victims[i] = i;
		}
		for (SchedulerThreadIndex i = threadIndex + 1; i < numSchedulerThreads; ++i) {
			victims[i - 1] = i;
		}
	}

	std::minstd_rand rand;
	PUMS_COMPLETION_LIST completionList;
	Task* currentContext;
	array<SchedulerThreadIndex, MAX_SCHEDULER_THREADS - 1> victims;
	TaskStealingDeque readyContexts;
	TaskStealingDeque pooledContexts;
	TaskStealingDeque newTasks;

	using SleepingContext = pair<steady_clock::time_point, Task*>;

	struct IsDeadlineGreater {
		bool operator()(const SchedulerThread::SleepingContext& left, const SchedulerThread::SleepingContext& right) const {
			return left.first > right.first;
		}
	};
	std::priority_queue<SleepingContext, std::vector<SleepingContext>, IsDeadlineGreater> sleepingContexts;
};

array<SchedulerThread, MAX_SCHEDULER_THREADS> schedulerThreads;

SchedulerThread& schedulerThread(SchedulerThreadIndex index = threadIndex) {
	return schedulerThreads[index];
}

template<TaskStealingDeque&(*AccessorFunc)(SchedulerThread&)>
struct WorkStealingBag {
	static void putLocal(Task* task, SchedulerThread& thread = schedulerThread()) {
		AccessorFunc(thread).push(task);
	}

	static Task* getLocal(SchedulerThread& thread = schedulerThread()) {
		return AccessorFunc(thread).pop();
	}

	static Task* steal(SchedulerThread& thread = schedulerThread()) {
		std::shuffle(thread.victims.begin(), thread.victims.begin() + (numSchedulerThreads - 1), thread.rand);
		for (size_t i = 0; i < numSchedulerThreads - 1; ++i) {
			auto victim = thread.victims[i];
			auto task = AccessorFunc(schedulerThreads[victim]).steal();
			if (task)
				return task;
		}
		return nullptr;
	}

	static Task* getLocalOrSteal(SchedulerThread& thread = schedulerThread()) {
		auto task = getLocal(thread);
		if (task)
			return task;
		else
			return steal(thread);
	}
};

auto& readyContexts(SchedulerThread& thread) {
	return thread.readyContexts;
}

auto& pooledContexts(SchedulerThread& thread) {
	return thread.pooledContexts;
}

auto& newTasks(SchedulerThread& thread) {
	return thread.newTasks;
}

Task* getTaskForUmsContext(PUMS_CONTEXT context) {
	PVOID threadInformation;
	CHECK_WIN32("QueryUmsThreadInformation",
		QueryUmsThreadInformation(context, UmsThreadUserContext, &threadInformation, sizeof(PVOID), NULL));
	return static_cast<Task*>(threadInformation);
}

void setTaskForUmsContext(PUMS_CONTEXT context, Task* task) {
	CHECK_WIN32("SetUmsThreadInformation",
		SetUmsThreadInformation(context, UmsThreadUserContext, &task, sizeof(PVOID)));
}

Task* dequeueCompletionList(DWORD timeout) {
	auto& thread = schedulerThread();
	PUMS_CONTEXT contextList;
	if (!DequeueUmsCompletionListItems(thread.completionList, timeout, &contextList)) {
		if (GetLastError() == ERROR_TIMEOUT)
			return nullptr;
		else
			CHECK_WIN32("DequeueUmsCompletionListItems", false);
	}
	TaskStack toDelete;
	Task* reserved = nullptr;
	while (contextList != NULL) {
		auto task = getTaskForUmsContext(contextList);
		BOOLEAN isTerminated;
		CHECK_WIN32("QueryUmsThreadInformation",
			QueryUmsThreadInformation(contextList, UmsThreadIsTerminated, &isTerminated, sizeof(isTerminated), NULL));
		if (isTerminated)
			toDelete.push(task);
		else if (!reserved)
			reserved = task;
		else
			enqueueContext(task);
		contextList = GetNextUmsListItem(contextList);
	}
	while (toDelete.peek()) {
		auto task = toDelete.pop();
		CHECK_WIN32("DeleteUmsThreadContext", DeleteUmsThreadContext(task->context));
	}
	return reserved;
}

Task* dequeueSleepingContexts() {
	auto& thread = schedulerThread();
	auto now = steady_clock::now();
	Task* reserved = nullptr;
	auto candidate = thread.sleepingContexts.top();
	while (now > candidate.first) {
		thread.sleepingContexts.pop();
		if (!reserved)
			reserved = candidate.second;
		else
			enqueueContext(candidate.second);
	}
	return reserved;
}

DWORD workerThread(LPVOID param) {
	auto task = static_cast<Task*>(param);
	auto context = task->context;
	while (keepRunning.load()) {
		task->func(&task->data);
		auto newTask = WorkStealingBag<newTasks>::getLocalOrSteal();
		if (newTask) {
			auto oldTask = task;
			task = newTask;
			task->context = context;
			setTaskForUmsContext(task->context, task);
			oldTask->context = nullptr;
		} else {
			auto yieldContinuation = createSchedulerContinuation([](Task* currentTask) {
				WorkStealingBag<pooledContexts>::putLocal(currentTask);
			});
			CHECK_WIN32("UmsThreadYield", UmsThreadYield(&yieldContinuation));
			task = getTaskForUmsContext(context);
		}
	}
	return 0;
}

void taskToUmsWorker(Task* task) {
	auto& thread = schedulerThread();

	STARTUPINFOEXW startupInfo = { 0 };
	startupInfo.StartupInfo.cb = sizeof(startupInfo);

	startupInfo.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(&scratch);
	SIZE_T attributeListSize = sizeof(scratch);
	CHECK_WIN32("InitializeProcThreadAttributeList",
		InitializeProcThreadAttributeList(startupInfo.lpAttributeList, 1, 0, &attributeListSize));

	CHECK_WIN32("CreateUmsThreadContext", CreateUmsThreadContext(&task->context));
	setTaskForUmsContext(task->context, task);

	UMS_CREATE_THREAD_ATTRIBUTES umsAttributes;
	umsAttributes.UmsVersion = UMS_VERSION;
	umsAttributes.UmsContext = task->context;
	umsAttributes.UmsCompletionList = thread.completionList;

	auto attributeList = startupInfo.lpAttributeList;
	CHECK_WIN32("UpdateProcThreadAttribute",
		UpdateProcThreadAttribute(attributeList, 0, PROC_THREAD_ATTRIBUTE_UMS_THREAD, &umsAttributes, sizeof(umsAttributes), NULL, NULL));

	CHECK_WIN32("CreateRemoteThreadEx",
		CreateRemoteThreadEx(GetCurrentProcess(), NULL, 4096, &workerThread, task, 0, attributeList, NULL));

	DeleteProcThreadAttributeList(startupInfo.lpAttributeList);
}

void tryExecute(Task* context) {
	if (context)
		executeContext(context);
}

void __stdcall schedulerFunction(UMS_SCHEDULER_REASON reason, ULONG_PTR activationPayload, PVOID schedulerParam) {
	auto& thread = schedulerThread();
	switch (reason) {
	case UmsSchedulerThreadYield:
	{
		auto continuation = static_cast<SchedulerContinuation*>(schedulerParam);
		auto yieldingContext = reinterpret_cast<PUMS_CONTEXT>(activationPayload);
		auto yieldingTask = getTaskForUmsContext(yieldingContext);
		(*continuation)(yieldingTask);
		break;
	}
	case UmsSchedulerThreadBlocked:
		if ((activationPayload & 1) == 0)
			goto SCHEDULE_AFTER_INTERRUPT;
		break;
	}

	tryExecute(dequeueSleepingContexts());
	tryExecute(dequeueCompletionList(0));
	tryExecute(WorkStealingBag<readyContexts>::getLocal());

SCHEDULE:
	{
		bool workerStarted = false;
		bool unblockFailed = false;
		do {
			auto pooledContext = WorkStealingBag<pooledContexts>::getLocal();
			if (pooledContext) {
				auto newTask = WorkStealingBag<newTasks>::getLocalOrSteal();
				if (newTask) {
					newTask->context = pooledContext->context;
					setTaskForUmsContext(newTask->context, newTask);
					pooledContext->context = nullptr;
					executeContext(newTask);
				} else {
					WorkStealingBag<pooledContexts>::putLocal(pooledContext);
				}
			}
			tryExecute(WorkStealingBag<readyContexts>::steal());
			if (!workerStarted && !unblockFailed) {
				auto newTask = WorkStealingBag<newTasks>::getLocalOrSteal();
				if (newTask) {
					taskToUmsWorker(newTask);
					workerStarted = true;
				}
			}
			auto newContext = dequeueCompletionList(workerStarted ? INFINITE : 1);
			if (newContext)
				executeContext(newContext);
			else
				unblockFailed = true;
		} while (keepRunning.load(std::memory_order_relaxed));
	}

SCHEDULE_AFTER_INTERRUPT:
	{
		auto localContext = WorkStealingBag<readyContexts>::getLocal();
		if (localContext)
			executeContext(localContext);
		auto newContext = dequeueCompletionList(INFINITE);
		if (newContext)
			executeContext(newContext);
		else
			goto SCHEDULE;
	}
}

DWORD becomeSchedulerThread(LPVOID param) {
	static_assert(sizeof(LPVOID) >= sizeof(decltype(threadIndex)));
	threadIndex = reinterpret_cast<decltype(threadIndex)>(param);
	auto& thread = schedulerThread();

	CreateUmsCompletionList(&thread.completionList);

	UMS_SCHEDULER_STARTUP_INFO umsInfo;
	umsInfo.UmsVersion = UMS_VERSION;
	umsInfo.CompletionList = thread.completionList;
	umsInfo.SchedulerProc = &schedulerFunction;
	umsInfo.SchedulerParam = nullptr;

	CHECK_WIN32("EnterUmsSchedulingMode", EnterUmsSchedulingMode(&umsInfo));

	return 0; // TODO: something meanignful
}

SchedulerThreadIndex currentSchedulerThread() {
	return threadIndex;
}

void startScheduler(void(*mainTask)()) {
	keepRunning.store(true);

	auto mainTaskStruct = createTask([mainTask]() {
		mainTask();
		keepRunning.store(false);
	});
	enqueueTask(mainTaskStruct.get());

	DWORD_PTR processAffinityMask, systemAffinityMask;
	GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask);
	array<DWORD_PTR, MAX_SCHEDULER_THREADS> affinityMasks;
	unsigned shift = 0;
	unsigned numAffinityMasks = 0;
	while (processAffinityMask != 0 && numAffinityMasks < MAX_SCHEDULER_THREADS) {
		while (!(processAffinityMask & 1)) {
			processAffinityMask >>= 1;
			++shift;
		}
		affinityMasks[numAffinityMasks] = 1ui64 << shift;
		processAffinityMask -= 1;
		++numAffinityMasks;
	}

	if (numAffinityMasks > 0)
		numSchedulerThreads = numAffinityMasks;
	else
		numSchedulerThreads = std::max(1u, std::thread::hardware_concurrency());

	HANDLE handles[MAX_SCHEDULER_THREADS];
	for (decltype(numSchedulerThreads) index = 0; index < numSchedulerThreads; ++index) {
		handles[index] = CHECK_WIN32("CreateThread",
			CreateThread(NULL, 0, &becomeSchedulerThread, reinterpret_cast<LPVOID>((uintptr_t)index), 0, NULL));
	}
	for (decltype(numAffinityMasks) index = 0; index < numAffinityMasks; ++index) {
		SetThreadAffinityMask(handles[index], affinityMasks[index]);
	}
	auto waitResult = WaitForMultipleObjects(numSchedulerThreads, handles, true, INFINITE);
	CHECK_WIN32("WaitForMultipleObjects", WAIT_OBJECT_0 <= waitResult && waitResult <= (WAIT_OBJECT_0 + numSchedulerThreads - 1));
}

void executeContext(Task* task) {
	schedulerThread().currentContext = task;
	while (!ExecuteUmsThread(task->context))
		CHECK_WIN32("ExecuteUmsThread", GetLastError() == ERROR_RETRY);
}

void enqueueContext(Task* task) {
	WorkStealingBag<readyContexts>::putLocal(task);
}

void enqueueTask(Task* task) {
	WorkStealingBag<newTasks>::putLocal(task);
}

void sleepUntil(steady_clock::time_point deadline) {
	auto yieldContinuation = createSchedulerContinuation([](Task* currentTask) {
		auto& thread = schedulerThread();
		thread.sleepingContexts.emplace(currentTask);
	});
	CHECK_WIN32("UmsThreadYield", UmsThreadYield(&yieldContinuation));
}

void sleepFor(steady_clock::duration duration) {
	sleepUntil(steady_clock::now() + duration);
}
