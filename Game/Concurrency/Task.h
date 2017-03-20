#pragma once

#include <Core\Utils.h>

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

	Task* right;
	PUMS_CONTEXT context;
	void(*func)(void*);
	char data[CACHE_LINE_SIZE - lastMemberStartPos<void*, void*, void*, char[1]>()];
};
static_assert(sizeof(Task) == CACHE_LINE_SIZE);

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

template<unsigned Capacity = 4096u>
struct alignas(CACHE_LINE_SIZE)BoundedConcurrentTaskDeque {
	static_assert(isPowerOfTwo(Capacity));

	BoundedConcurrentTaskDeque() : bottom{ 0 }, top{ 0 } {}

	void push(Task* task) {
		long b = bottom;
		tasks[b & Mask] = task;
		_ReadWriteBarrier(); // Write the task before updating bottom
		bottom = b + 1;
	}

	Task* pop() {
		long b = bottom - 1;
		bottom = b;
		MemoryBarrier(); // Read top after updating bottom
		long t = top;
		if (t <= b) {
			auto task = tasks[b & Mask];
			if (t != b) // Still more than one item left
				return task;
			// Last item
			if (_InterlockedCompareExchange(&top, t + 1, t) != t)
				task = nullptr; // Lost race with steal
			bottom = t + 1;
			return task;
		} else {
			bottom = t;
			return nullptr;
		}
	}

	Task* steal() {
		for (;;) {
			long t = top;
			_ReadWriteBarrier(); // Read top before bottom
			long b = bottom;
			if (t < b) {
				auto task = tasks[t & Mask];
				if (_InterlockedCompareExchange(&top, t + 1, t) != t)
					continue; // Lost race with steal or pop
				return task;
			} else {
				return nullptr;
			}
		}
	}

private:
	static const unsigned int Mask = Capacity - 1u;

	union {
		char padding1[CACHE_LINE_SIZE];
		unsigned bottom;
	};
	union {
		char padding2[CACHE_LINE_SIZE];
		unsigned top;
	};
	union {
		char padding3[CACHE_LINE_SIZE];
		Task* tasks[Capacity];
	};
};
