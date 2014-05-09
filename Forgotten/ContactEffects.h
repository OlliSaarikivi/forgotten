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
            
            contacts.update(current, Row<KnockbackEnergy>({ 0.0f }));
        }
    }
private:
    TContacts& contacts;
    TLinearImpulses& linear_impulses;
};
