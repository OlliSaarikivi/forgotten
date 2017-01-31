#pragma once

#include <Core\Utils.h>
#include "Spinlock.h"

template<size_t CurrentByte, class... Types>
struct MemberPosHelper;
template<size_t CurrentByte, class Type>
struct MemberPosHelper<CurrentByte, Type> {
	static const size_t value = CurrentByte + (CurrentByte % alignof(Type) == 0 ? 0 : (alignof(Type)-(CurrentByte % alignof(Type))));
};
template<size_t CurrentByte, class Type, class... Types>
struct MemberPosHelper<CurrentByte, Type, Types...> {
	static const size_t value = MemberPosHelper<MemberPosHelper<CurrentByte, Type>::value + sizeof(Type), Types...>::value;
};

template<class... Types>
constexpr size_t lastMemberStartPos() {
	return MemberPosHelper<0, Types...>::value;
}

template<class T>
struct TaggedPointer {
	T* ptr;
	uintptr_t tag;
};

// Still need to use _aligned_malloc
struct alignas(CACHE_LINE_SIZE)Task {
	Task(void(*func)(void*));

	std::atomic<Task*> left;
	std::atomic<Task*> right;
	PUMS_CONTEXT context;
	void(*func)(void*);

	static const int dataStartByte = lastMemberStartPos<std::atomic<Task*>, std::atomic<Task*>, PUMS_CONTEXT, void*, char[1]>();
	static const int dataSize = CACHE_LINE_SIZE - dataStartByte;
	static_assert(dataSize > sizeof(void*));

	char data[dataSize];
};
static const size_t taskSize = sizeof(Task);
static_assert(taskSize == CACHE_LINE_SIZE);

template<class T>
struct FlaggedPointer {
	FlaggedPointer(T* ptr) : ptr{ ptr } {}
	FlaggedPointer(T* ptr, uintptr_t flag) : ptr{ ptr | flag } {}

	uintptr_t flag() {
		return (ptr & 1);
	}

	void clearFlag() {
		ptr = ptr & -2;
	}

	void toggleFlag() {
		ptr = ptr ^ 1;
	}

	T* get() const {
		return reinterpret_cast<T*>(ptr & -2);
	}

private:
	uintptr_t ptr;
};

struct alignas(16) Anchor {
	FlaggedPointer<Task> left = nullptr;
	FlaggedPointer<Task> right = nullptr;

	friend bool operator==(const Anchor& v1, const Anchor& v2) {
		return v1.left == v2.left && v1.right == v2.right;
	}
	friend bool operator!=(const Anchor& v1, const Anchor& v2) {
		return !(v1 == v2);
	}
};
static const size_t anchorSize = sizeof(Anchor);
static_assert(anchorSize == 16); // Make sure we can use DWCAS

// ABA problem is ignored. Instead we'll be VERY careful with Task allocation.
struct ConcurrentQueueStack {
	ConcurrentQueueStack() : anchor{ Anchor() } {
		if (!anchor.is_lock_free())
			ERROR(L"ConcurrentQueueStack is not lock-free on this architecture. DWCAS not supported?");
	}

	Task* popLeft() {
		for (;;) {
			auto current = anchor.load();
			auto left = current.left.get();
			if (!left) return nullptr;
			if (left == current.right.get()) {
				if (anchor.compare_exchange_strong(current, Anchor{ nullptr, nullptr }))
					return left;
			} else if (current.left.flag()) {
				stabilizeLeft(current);
			} else if (current.right.flag()) {
				stabilizeRight(current);
			} else {
				auto next = left->right.load();
				if (anchor.compare_exchange_strong(current, Anchor{ next, current.right }))
					return left;
			}
		}
	}

	Task* popRight() {
		for (;;) {
			auto current = anchor.load();
			auto right = current.right.get();
			if (!right) return nullptr;
			if (right == current.left.get()) {
				if (anchor.compare_exchange_strong(current, Anchor{ nullptr, nullptr }))
					return right;
			} else if (current.left.flag()) {
				stabilizeLeft(current);
			} else if (current.right.flag()) {
				stabilizeRight(current);
			} else {
				auto prev = right->left.load();
				if (anchor.compare_exchange_strong(current, Anchor{ current.left, prev }))
					return right;
			}
		}
	}

	void pushLeft(Task* task) {
		for (;;) {
			auto current = anchor.load();
			if (!current.left.get()) {
				if (anchor.compare_exchange_strong(current, Anchor{ task, task })) return;
			} else if (current.left.flag()) {
				stabilizeLeft(current);
			} else if (current.right.flag()) {
				stabilizeRight(current);
			} else {
				task->right = current.left.get();
				Anchor update{ { task, true }, current.right };
				if (anchor.compare_exchange_strong(current, update)) {
					stabilizeRight(update);
					return;
				}
			}
		}
	}

	void pushRight(Task* task) {
		for (;;) {
			auto current = anchor.load();
			if (!current.right.get()) {
				if (anchor.compare_exchange_strong(current, Anchor{ task, task })) return;
			} else if (current.left.flag()) {
				stabilizeLeft(current);
			} else if (current.right.flag()) {
				stabilizeRight(current);
			} else {
				task->left = current.right.get();
				Anchor update{ current.left, { task, true } };
				if (anchor.compare_exchange_strong(current, update)) {
					stabilizeLeft(update);
					return;
				}
			}
		}
	}

	void stabilizeLeft(Anchor& current) {
		auto next = current.left.get()->right.load();
		if (anchor.load() != current) return;
		auto nextprev = next->left.load();
		if (nextprev != current.left.get()) {
			if (anchor.load() != current) return;
			if (!next->left.compare_exchange_strong(nextprev, current.left.get())) return;
		}
		anchor.compare_exchange_strong(current, Anchor{ current.left.get(), current.right.get() });
	}

	void stabilizeRight(Anchor& current) {
		auto prev = current.right.get()->left.load();
		if (anchor.load() != current) return;
		auto prevnext = prev->right.load();
		if (prevnext != current.right.get()) {
			if (anchor.load() != current) return;
			if (!prev->right.compare_exchange_strong(prevnext, current.right.get())) return;
		}
		anchor.compare_exchange_strong(current, Anchor{ current.left.get(), current.right.get() });
	}

private:
	std::atomic<Anchor> anchor;
};

struct ScalableQueueStack {


private:
	unsigned numThreads;
};
