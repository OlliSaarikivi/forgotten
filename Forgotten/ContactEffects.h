#pragma once

#include "GameProcess.h"

struct KnockbackEffect : GameProcess
{
    KnockbackEffect(Game& game, ProcessHost<Game>& host) : GameProcess(game, host) {}

    void tick() override
    {
        static auto& knockback_contacts = host.from(contacts).join(knockback_impulses)
            .join(max_knockback_energy).amend(knockback_energies).select();

        auto current = knockback_contacts.begin();
        auto end = knockback_contacts.end();
        while (current != end) {
            auto contact = *current;
            b2Contact* b2_contact = contact.contact;
            vec2 impulse = contact.contact_normal * contact.knockback_impulse * contact.knockback_energy;
            linear_impulses.put({ { contact.fixture_b->GetBody() }, { impulse } });
            knockback_contacts.update(current, Row<KnockbackEnergy>({ 0.0f }));
            ++current;
        }
    }
private:
    BUFFER_SOURCE(contacts, contacts);
    SOURCE(knockback_impulses, knockback_impulses);
    SOURCE(max_knockback_energy, max_knockback_energy);
    MUTABLE(knockback_energies, knockback_energies);
    SINK(linear_impulses, center_impulses);
};
