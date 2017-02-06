#pragma once

#include <Core\Utils.h>
#include "DWAtomic.h"

template<class T>
struct TaggedPointer {
	TaggedPointer() {}
	TaggedPointer(T* ptr) : ptr{ reinterpret_cast<uintptr_t>(ptr) } {}
	TaggedPointer(T* ptr, uintptr_t tag) : ptr{ reinterpret_cast<uintptr_t>(ptr) | (tag & 0x3F) } {}

	uintptr_t tag() {
		return (ptr & 0x3F);
	}

	T* get() const {
		return reinterpret_cast<T*>(ptr & ~0x3F);
	}

	friend bool operator==(const TaggedPointer& v1, const TaggedPointer& v2) {
		return v1.ptr == v2.ptr;
	}
	friend bool operator!=(const TaggedPointer& v1, const TaggedPointer& v2) {
		return !(v1 == v2);
	}

private:
	uintptr_t ptr;
};

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

// Still need to use _aligned_malloc
struct alignas(CACHE_LINE_SIZE)Task {
	Task(void(*func)(void*));

	void * operator new(std::size_t size);
	void operator delete(void* ptr);

	std::atomic<TaggedPointer<Task>> leftTagged;
	union {
		std::atomic<TaggedPointer<Task>> rightTagged;
		Task* right;
	};
	PUMS_CONTEXT context;
	void(*func)(void*);
	char data[CACHE_LINE_SIZE - lastMemberStartPos<void*, void*, void*, void*, char[1]>()];
};
static_assert(sizeof(Task) == CACHE_LINE_SIZE);

struct alignas(16) Anchor {
	TaggedPointer<Task> left = nullptr;
	TaggedPointer<Task> right = nullptr;

	friend bool operator==(const Anchor& v1, const Anchor& v2) {
		return v1.left == v2.left && v1.right == v2.right;
	}
	friend bool operator!=(const Anchor& v1, const Anchor& v2) {
		return !(v1 == v2);
	}
};
static const size_t anchorSize = sizeof(Anchor);
static_assert(anchorSize == 16); // Make sure we can use DWCAS

// Memory reuse problem is handled by only deallocating Tasks at quiescent states,
// e.g., deallocate frame N tasks when all threads have started on N+1.

struct ConcurrentTaskDeque {
	void assertLockFree();

	Task* popLeft();
	Task* popRight();
	void pushLeft(Task* task);
	void pushRight(Task* task);

	void stabilizeLeft(Anchor& current);
	void stabilizeRight(Anchor& current);
private:
	DWAtomic<Anchor> anchor = Anchor();
};

struct ConcurrentTaskStack {
	void assertLockFree();

	Task* pop();
	void push(Task* task);
	Task* peek();
private:
	std::atomic<TaggedPointer<Task>> top = nullptr;
};

struct TaskStack {
	Task* pop();
	void push(Task* task);
	Task* peek();
private:
	Task* top = nullptr;
};