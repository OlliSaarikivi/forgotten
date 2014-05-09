#pragma once

#include "Row.h"
#include "Process.h"

template<typename TContacts, typename TLinearImpulses>
struct KnockbackEffect : Process
{
    KnockbackEffect(TContacts &contacts, TLinearImpulses& linear_impulses) :
    contacts(contacts),
    linear_impulses(linear_impulses)
    {
        linear_impulses.registerProducer(this);
        contacts.registerProducer(this);
    }
    void forEachInput(function<void(const Channel&)> f) const override
    {
        f(contacts);
    }
    void tick() override
    {
        auto current = contacts.begin();
        auto end = contacts.end();
        while (current != end) {
            auto contact = *current;
            b2Contact* b2_contact = contact.contact;
            vec2 impulse = contact.contact_normal * contact.knockback_impulse * contact.knockback_energy;
            linear_impulses.put(TLinearImpulses::RowType({ contact.fixture_a->GetBody() }, { impulse }));
            contacts.update(current, Row<KnockbackEnergy>({ 0.0f }));
        }
    }
private:
    TContacts& contacts;
    TLinearImpulses& linear_impulses;
};
