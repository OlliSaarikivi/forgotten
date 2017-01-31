#pragma once

class Timer
{
public:
	Timer() : beg_(clock_::now()) {}
	void reset() { beg_ = clock_::now(); }
	double elapsed() const {
		return chrono::duration_cast<second_>
			(clock_::now() - beg_).count();
	}

private:
	typedef chrono::high_resolution_clock clock_;
	typedef chrono::duration<double, std::ratio<1> > second_;
	chrono::time_point<clock_> beg_;
};

template<class TFunc> void Time(string name, const TFunc& f) {
	Timer tmr; tmr.reset(); f(); double t = tmr.elapsed(); std::cout << t << "s\t" << name << "\n";
}
