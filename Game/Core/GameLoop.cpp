#include <Core/stdafx.h>
#include "GameLoop.h"
#include <Concurrency\Scheduler.h>
#include <Graphics\Renderer.h>
#include <GameLogic\Update.h>

const steady_clock::duration maxStep = chrono::milliseconds(10);
const steady_clock::duration minStep = chrono::milliseconds(2);
const int maxSubsteps = 10;

// GAME LOOP

template<int WINDOW_SIZE>
struct FilteredMovingAveragePredictor {
	static_assert(WINDOW_SIZE > 2, "the window size must be at least 3 so that the average is still defined after dropping min and max");

	FilteredMovingAveragePredictor() : window{}, next(window.begin())
	{
		for (auto& measurement : window) {
			measurement = steady_clock::duration::zero();
		}
	}

	steady_clock::duration predict() {
		steady_clock::duration sum = steady_clock::duration::zero(), min = window[0], max = window[0];
		for (const auto& measurement : window) {
			sum += measurement;
			if (measurement < min) {
				min = measurement;
			} else if (measurement > max) {
				max = measurement;
			}
		}
		return (sum - min - max) / (WINDOW_SIZE - 2);
	}

	void update(steady_clock::duration duration) {
		*next = duration;
		++next;
		next = next == window.end() ? window.begin() : next;
	}
private:
	typename std::array<steady_clock::duration, WINDOW_SIZE> window;
	typename std::array<steady_clock::duration, WINDOW_SIZE>::iterator next;
};

steady_clock::duration abs(steady_clock::duration d) {
	return d < steady_clock::duration::zero() ? -d : d;
}

void gameLoop() {
	static_assert(std::numeric_limits<steady_clock::rep>::digits > 60, "clock representation is not large enough");

	FilteredMovingAveragePredictor<7> updatePredictor;
	FilteredMovingAveragePredictor<7> renderPredictor;

	steady_clock::time_point now = steady_clock::now();
	steady_clock::time_point gameTime = now; // The time represented by the current state of the game

	while (true) {
		auto beginRender = now;
		render();
		now = steady_clock::now();
		renderPredictor.update(now - beginRender);
		auto renderedGameTime = gameTime;

		int substeps = 0;
		while (substeps < maxSubsteps && now - gameTime > minStep) {

			auto updatePrediction = updatePredictor.predict();
			auto deltaToNow = (now - gameTime);

			steady_clock::duration idealStep;
			if (updatePrediction < maxStep) {
				auto substeps_needed = deltaToNow / (maxStep - updatePrediction) + 1;
				idealStep = substeps_needed > maxSubsteps ? maxStep :
					(deltaToNow + updatePrediction * substeps_needed) / substeps_needed;
			} else {
				idealStep = maxStep;
			}
			auto clampedStep = std::max(minStep, std::min(maxStep, idealStep));

			float dt = chrono::duration_cast<chrono::nanoseconds>(clampedStep).count() / 1e9f;
			auto dtNanos = chrono::nanoseconds(std::llroundf(dt * 1e9f));

			auto beginUpdate = now;
			update(dt);
			now = steady_clock::now();
			updatePredictor.update(now - beginUpdate);

			gameTime += dtNanos;
			++substeps;
		}

		// Slow down if we are falling behind (avoid spiral of death).
		if (gameTime < now && now - gameTime > maxStep) {
			gameTime = now - maxStep;
		}

		auto afterNextRender = now + renderPredictor.predict();
		auto lastRenderError = abs(afterNextRender - renderedGameTime);
		auto newRenderError = abs(afterNextRender - gameTime);

		// If rendering now would increase the timing error we sleep here until the errors break even.
		if (newRenderError > lastRenderError) {
			sleepFor((newRenderError - lastRenderError) / 2);
			now = steady_clock::now();
		}
	}
}

void exitGame() {
	exit(0);
}