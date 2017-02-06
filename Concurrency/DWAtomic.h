#pragma once

template<class T>
struct alignas(16) DWAtomic {
	static_assert(sizeof(T) == 16);

	DWAtomic(T value) : value{ value } {}

	T load() {
		T result;
		auto targetPtr = reinterpret_cast<__int64*>(&value);
		auto expectedPtr = reinterpret_cast<__int64*>(&result);
		expectedPtr[0] = 0;
		expectedPtr[1] = 0;
		_InterlockedCompareExchange128(targetPtr, expectedPtr[1], expectedPtr[0], expectedPtr);
		return result;
	}

	bool compareExchange(T& expected, T exchange) {
		auto targetPtr = reinterpret_cast<__int64*>(&value);
		auto expectedPtr = reinterpret_cast<__int64*>(&expected);
		auto exchangePtr = reinterpret_cast<__int64*>(&exchange);
		return _InterlockedCompareExchange128(targetPtr, exchangePtr[1], exchangePtr[0], expectedPtr);
	}
private:
	T value;
};