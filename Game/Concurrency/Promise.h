#pragma once

#include "Event.h"

template<class T>
struct Promise {
	template<class U>
	void fulfill(U&& result) {
		value = std::forward<U>(result);
		fulfilled.signal();
	}
	const T& wait() {
		fulfilled.wait();
		return value;
	}
private:
	T value;
	Event fulfilled;
};
