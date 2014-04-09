#pragma once

#include "stdafx.h"

typedef long int Eid;

template<typename T>
struct EidComparator
{
    bool operator()(Eid l, const T &r) const
    {
        return l < r.eid;
    }
    bool operator()(const T &l, Eid r) const
    {
        return l.eid < r;
    }
};

template<typename TRow, typename TOther, typename... TColumns>
struct SetAllHelper;

template<typename TRow, typename TOther>
struct SetAllHelper<TRow, TOther>
{
    static void tSet(TRow *row, TOther other) {}
};

template<typename TRow, typename TOther, typename TColumn, typename... TColumns>
struct SetAllHelper<TRow, TOther, TColumn, TColumns...>
{
    static void tSet(TRow *row, TOther other)
    {
        row->set(static_cast<TColumn>(other));
        return SetAllHelper<TRow, TOther, TColumns...>::tSet(row, other);
    }
};

template<typename TKey, typename... TColumns>
struct Row : TColumns...
{
    TKey key;

    template<typename TColumn>
    void set(TColumn c)
    {
        TColumn *me = this;
        me->set(c);
    }

    template<typename... TOtherColumns>
    void setAll(Row<TKey, TOtherColumns...> other)
    {
        SetAllHelper<Row, Row<TKey, TOtherColumns...>, TOtherColumns...>::tSet(this, other);
    }
};

struct PositionColumn
{
    vec2 position;
    void set(PositionColumn c)
    {
        position = c.position;
    }
};

struct BodyColumn
{
    b2Body *body;
    void set(BodyColumn c)
    {
        body = c.body;
    }
};

struct CenterForce
{
    CenterForce(vec2 force, b2Body *body) : force(force), body(body) {}
    vec2 force;
    b2Body *body;
};

struct Contact
{
    Contact(b2Fixture *fixtureA, b2Fixture *fixtureB) : fixtureA(fixtureA), fixtureB(fixtureB) {}
    b2Fixture *fixtureA;
    b2Fixture *fixtureB;
};