#include "stdafx.h"

#include "Game.h"
#include "Utils.h"

struct ContactRecorder : b2ContactListener
{
	Game& game;

	void BeginContact(b2Contact *contact) override
	{
		auto fixtureA = contact->GetFixtureA();
		auto fixtureB = contact->GetFixtureB();
		b2WorldManifold manifold;
		contact->GetWorldManifold(&manifold);
		vec2 normal = toGLM(manifold.normal);
		game.contacts.second.put({ { contact },{ fixtureA },{ fixtureB },{ normal } });
		game.contacts.second.put({ { contact },{ fixtureB },{ fixtureA },{ -normal } });
	}
	void EndContact(b2Contact *contact) override
	{
		game.contacts.second.erase(Row<Contact>({ contact }));
	}
};

void Game::initBox2d()
{
	static ContactRecorder recorder{ *this };
	world.SetContactListener(&recorder);
}

void Game::stepBox2d(float step)
{
	for (const auto &center_force : center_forces) {
		b2Body *b = center_force.body;
		b->ApplyForceToCenter(toB2(center_force.force), true);
	}

	for (const auto &linear_impulse : linear_impulses) {
		b2Body *b = linear_impulse.body;
		b->ApplyLinearImpulse(toB2(linear_impulse.linear_impulse), b->GetWorldCenter(), true);
	}

	world.Step(step, 8, 3);
}

void Game::readBox2d()
{
	for (const auto &body : dynamic_bodies) {
		b2Body *b = body.body;
		auto position = positions.equalRange(body);
		if (position.first != position.second) {
			positions.update(position.first, Row<Position>({ toGLM(b->GetPosition()) }));
		}
		auto velocity = velocities.equalRange(body);
		if (velocity.first != velocity.second) {
			velocities.update(velocity.first, Row<Velocity>({ toGLM(b->GetLinearVelocity()) }));
		}
		auto heading = headings.equalRange(body);
		if (heading.first != heading.second) {
			headings.update(heading.first, Row<Heading>({ b->GetAngle() }));
		}
	}
}
