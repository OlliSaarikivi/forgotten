#pragma once

template<class T> class Eventual {
	promise<T> promise_;
	future<T> future_;

public:
	Eventual() : future_(promise_.get_future()) {}

	template<class R> void set(R&& value) {
		promise_.set_value(std::forward<R>(value));
	}

	T await() {
		future_.wait();
		return future_.get();
	}
};