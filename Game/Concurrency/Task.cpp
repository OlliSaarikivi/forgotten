#include <Core\stdafx.h>
#include "Task.h"
#include "Scheduler.h"

Task::Task(void(*func)(void*)) :
	right{ nullptr },
	context{ NULL },
	func{ func } {}

void* Task::operator new(std::size_t size) {
	while (true) {
		auto ptr = _aligned_malloc(size, alignof(Task));
		if (ptr)
			return ptr;
		std::new_handler globalHandler = std::set_new_handler(0);
		std::set_new_handler(globalHandler);
		if (globalHandler)
			(*globalHandler)();
		else
			throw std::bad_alloc();
	}
}

void Task::operator delete(void * ptr) {
	_aligned_free(ptr);
}

void ConcurrentTaskStack::assertLockFree() {
	if (!top.is_lock_free())
		FORGOTTEN_ERROR(L"ConcurrentTaskStack is not lock-free on this architecture.");
}

Task* ConcurrentTaskStack::pop() {
	auto current = top.load();
	do {
		if (!current.get())
			return nullptr;
	} while (!top.compare_exchange_strong(current, { current.get()->right, current.tag() + 1 }));
	return current.get();
}

void ConcurrentTaskStack::push(Task* task) {
	auto current = top.load();
	do {
		task->right = current.get();
	} while (!top.compare_exchange_strong(current, { task, current.tag() + 1 }));
}

Task* ConcurrentTaskStack::peek() {
	auto current = top.load();
	return current.get();
}

Task* TaskStack::pop() {
	auto result = top;
	if (result)
		top = result->right;
	return result;
}

void TaskStack::push(Task* task) {
	task->right = top;
	top = task;
}

Task* TaskStack::peek() {
	return top;
}
