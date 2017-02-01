#pragma once

#include "Event.h"

template<class T>
struct Promise {
	template<class U>
	void fulfill(U&& val) {
		val = std::forward<U>(val);
		fulfilled.signal();
	}
	const T& wait() {
		fulfilled.wait();
		return val;
	}
private:
	T val;
	Event fulfilled;
};
