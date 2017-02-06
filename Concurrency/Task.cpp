#include <Core\stdafx.h>
#include "Task.h"
#include "Scheduler.h"

Task::Task(void(*func)(void*)) :
	leftTagged{ nullptr },
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

void ConcurrentTaskDeque::assertLockFree() {
	/*if (!anchor.is_lock_free())
		FORGOTTEN_ERROR(L"ConcurrentTaskDeque is not lock-free on this architecture. DWCAS not supported?");*/
}

Task* ConcurrentTaskDeque::popLeft() {
	for (;;) {
		auto current = anchor.load();
		auto left = current.left.get();
		if (!left)
			return nullptr;
		if (left == current.right.get()) {
			auto low = (current.right.tag() >> 1) + 1;
			auto high = (current.left.tag() >> 1) + (low >> 5);
			if (anchor.compareExchange(current, { { nullptr, high << 1 }, { nullptr, low << 1 } }))
				return left;
		} else if (current.left.tag() & 1) {
			stabilizeLeft(current);
		} else if (current.right.tag() & 1) {
			stabilizeRight(current);
		} else {
			auto next = left->rightTagged.load();
			auto low = (current.right.tag() >> 1) + 1;
			auto high = (current.left.tag() >> 1) + (low >> 5);
			if (anchor.compareExchange(current, { { next.get(), high << 1 }, { current.right.get(), low << 1 } }))
				return left;
		}
	}
}

Task* ConcurrentTaskDeque::popRight() {
	for (;;) {
		auto current = anchor.load();
		auto right = current.right.get();
		if (!right)
			return nullptr;
		if (right == current.left.get()) {
			auto low = (current.right.tag() >> 1) + 1;
			auto high = (current.left.tag() >> 1) + (low >> 5);
			if (anchor.compareExchange(current, { { nullptr, high << 1 }, { nullptr, low << 1 } }))
				return right;
		} else if (current.left.tag() & 1) {
			stabilizeLeft(current);
		} else if (current.right.tag() & 1) {
			stabilizeRight(current);
		} else {
			auto prev = right->leftTagged.load();
			auto low = (current.right.tag() >> 1) + 1;
			auto high = (current.left.tag() >> 1) + (low >> 5);
			if (anchor.compareExchange(current, { { current.left.get(), high << 1 },{ prev.get(), low << 1 } }))
				return right;
		}
	}
}

void ConcurrentTaskDeque::pushLeft(Task* task) {
	task->rightTagged = nullptr;
	task->leftTagged = nullptr;
	for (;;) {
		auto current = anchor.load();
		if (!current.left.get()) {
			auto low = (current.right.tag() >> 1) + 1;
			auto high = (current.left.tag() >> 1) + (low >> 5);
			if (anchor.compareExchange(current, { { task, high << 1 },{ task, low << 1 } }))
				return;
		} else if (current.left.tag() & 1) {
			stabilizeLeft(current);
		} else if (current.right.tag() & 1) {
			stabilizeRight(current);
		} else {
			task->rightTagged = current.left.get();
			auto low = (current.right.tag() >> 1) + 1;
			auto high = (current.left.tag() >> 1) + (low >> 5);
			Anchor update{ { task, (high << 1) & 1 },{ current.right.get(), low << 1 } };
			if (anchor.compareExchange(current, update)) {
				stabilizeLeft(update);
				return;
			}
		}
	}
}

void ConcurrentTaskDeque::pushRight(Task* task) {
	task->rightTagged = nullptr;
	task->leftTagged = nullptr;
	for (;;) {
		auto current = anchor.load();
		if (!current.right.get()) {
			auto low = (current.right.tag() >> 1) + 1;
			auto high = (current.left.tag() >> 1) + (low >> 5);
			if (anchor.compareExchange(current, { { task, high << 1 },{ task, low << 1 } }))
				return;
		} else if (current.left.tag() & 1) {
			stabilizeLeft(current);
		} else if (current.right.tag() & 1) {
			stabilizeRight(current);
		} else {
			task->leftTagged = current.right.get();
			auto low = (current.right.tag() >> 1) + 1;
			auto high = (current.left.tag() >> 1) + (low >> 5);
			Anchor update{ { current.left.get(), high << 1 },{ task, (low << 1) & 1 } };
			if (anchor.compareExchange(current, update)) {
				stabilizeRight(update);
				return;
			}
		}
	}
}

void ConcurrentTaskDeque::stabilizeLeft(Anchor& current) {
	auto next = current.left.get()->rightTagged.load();
	if (anchor.load() != current)
		return;
	auto nextprev = next.get()->leftTagged.load();
	if (nextprev != current.left.get()) {
		if (anchor.load() != current)
			return;
		if (!next.get()->leftTagged.compare_exchange_strong(nextprev, { current.left.get(), nextprev.tag() + 1 }))
			return;
	}
	auto low = (current.right.tag() >> 1) + 1;
	auto high = (current.left.tag() >> 1) + (low >> 5);
	anchor.compareExchange(current, { { current.left.get(), high << 1 },{ current.right.get(), low << 1 } });
}

void ConcurrentTaskDeque::stabilizeRight(Anchor& current) {
	auto prev = current.right.get()->leftTagged.load();
	if (anchor.load() != current)
		return;
	auto prevnext = prev.get()->rightTagged.load();
	if (prevnext != current.right.get()) {
		if (anchor.load() != current)
			return;
		if (!prev.get()->rightTagged.compare_exchange_strong(prevnext, { current.right.get(), prevnext.tag() + 1 }))
			return;
	}
	auto low = (current.right.tag() >> 1) + 1;
	auto high = (current.left.tag() >> 1) + (low >> 5);
	anchor.compareExchange(current, { { current.left.get(), high << 1 },{ current.right.get(), low << 1 } });
}

void ConcurrentTaskStack::assertLockFree() {
	if (!top.is_lock_free())
		FORGOTTEN_ERROR(L"ConcurrentTaskStack is not lock-free on this architecture. DWCAS not supported?");
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
