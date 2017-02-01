#include <Core\stdafx.h>
#include "Task.h"

Task::Task(void(*func)(void*)) :
	left{ nullptr },
	right{ nullptr },
	context{ NULL },
	func{ func } {}

void ConcurrentTaskDeque::assertLockFree() {
	if (!anchor.is_lock_free())
		FORGOTTEN_ERROR(L"ConcurrentTaskDeque is not lock-free on this architecture. DWCAS not supported?");
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
			if (anchor.compare_exchange_strong(current, { { nullptr, high << 1 }, { nullptr, low << 1 } }))
				return left;
		} else if (current.left.tag() & 1) {
			stabilizeLeft(current);
		} else if (current.right.tag() & 1) {
			stabilizeRight(current);
		} else {
			auto next = left->rightTagged.load();
			auto low = (current.right.tag() >> 1) + 1;
			auto high = (current.left.tag() >> 1) + (low >> 5);
			if (anchor.compare_exchange_strong(current, { { next.get(), high << 1 }, { current.right.get(), low << 1 } }))
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
			if (anchor.compare_exchange_strong(current, { { nullptr, high << 1 }, { nullptr, low << 1 } }))
				return right;
		} else if (current.left.tag() & 1) {
			stabilizeLeft(current);
		} else if (current.right.tag() & 1) {
			stabilizeRight(current);
		} else {
			auto prev = right->leftTagged.load();
			auto low = (current.right.tag() >> 1) + 1;
			auto high = (current.left.tag() >> 1) + (low >> 5);
			if (anchor.compare_exchange_strong(current, { { current.left.get(), high << 1 },{ prev.get(), low << 1 } }))
				return right;
		}
	}
}

void ConcurrentTaskDeque::pushLeft(Task* task) {
	for (;;) {
		auto current = anchor.load();
		if (!current.left.get()) {
			auto low = (current.right.tag() >> 1) + 1;
			auto high = (current.left.tag() >> 1) + (low >> 5);
			if (anchor.compare_exchange_strong(current, { { task, high << 1 },{ task, low << 1 } }))
				return;
		} else if (current.left.tag() & 1) {
			stabilizeLeft(current);
		} else if (current.right.tag() & 1) {
			stabilizeRight(current);
		} else {
			task->right = current.left.get();
			auto low = (current.right.tag() >> 1) + 1;
			auto high = (current.left.tag() >> 1) + (low >> 5);
			Anchor update{ { task, (high << 1) & 1 },{ current.right.get(), low << 1 } };
			if (anchor.compare_exchange_strong(current, update)) {
				stabilizeRight(update);
				return;
			}
		}
	}
}

void ConcurrentTaskDeque::pushRight(Task* task) {
	for (;;) {
		auto current = anchor.load();
		if (!current.right.get()) {
			auto low = (current.right.tag() >> 1) + 1;
			auto high = (current.left.tag() >> 1) + (low >> 5);
			if (anchor.compare_exchange_strong(current, { { task, high << 1 },{ task, low << 1 } }))
				return;
		} else if (current.left.tag() & 1) {
			stabilizeLeft(current);
		} else if (current.right.tag() & 1) {
			stabilizeRight(current);
		} else {
			task->left = current.right.get();
			auto low = (current.right.tag() >> 1) + 1;
			auto high = (current.left.tag() >> 1) + (low >> 5);
			Anchor update{ { current.left.get(), high << 1 },{ task, (low << 1) & 1 } };
			if (anchor.compare_exchange_strong(current, update)) {
				stabilizeLeft(update);
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
	anchor.compare_exchange_strong(current, { { current.left.get(), high << 1 },{ current.right.get(), low << 1 } });
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
	anchor.compare_exchange_strong(current, { { current.left.get(), high << 1 },{ current.right.get(), low << 1 } });
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
