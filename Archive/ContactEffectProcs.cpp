#include "stdafx.h"

#include "Game.h"

void Game::applyKnockback()
{
    static auto& knockback_contacts = from(contacts).join(knockback_impulses)
        .join(max_knockback_energy).amend(knockback_energies).select();

    for (auto contact : knockback_contacts) {
        b2Contact* b2_contact = contact.contact;
        vec2 impulse = contact.contact_normal * contact.knockback_impulse * contact.knockback_energy;
		center_impulses.put({ { contact.fixture_b->GetBody() }, { impulse } });
		contact.knockback_energy = 0.0f;
    }
}