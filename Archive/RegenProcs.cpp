#include "stdafx.h"

#include "Game.h"

template<typename TStat, int MaximumX100, int RateX100>
struct Regen
{
	template<typename TStats>
	static void doStep(float step, TStats& stats)
	{
		const float Maximum = MaximumX100 / 100;
		const float Rate = RateX100 / 100;

		auto current = stats.begin();
		auto end = stats.end();
		while (current != end) {
			auto row = *current;
			TStat& stat = row;
			stat.setField(stat.get() + (Rate)* step);
			if (stat.get() > Maximum) {
				current = stats.erase(current);
			}
			else {
				stats.update(current, Key<TStat>::project(row));
				++current;
			}
		}
	}
};

void Game::regenKnockbackEnergy(float step)
{
	Regen<KnockbackEnergy, 100, 1000>::doStep(step, knockback_energies);
}
