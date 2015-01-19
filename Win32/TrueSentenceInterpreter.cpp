#include "stdafx.h"
#include "TrueSentenceInterpreter.h"

struct WordClass
{
    string name;

    inline bool operator==(const WordClass& other) const
    {
        assert(this != &other || name == other.name);
        return this == &other;
    }
};

struct NonTerminal;

using Symbol = variant<NonTerminal&, WordClass&>;

struct NonTerminal
{
    string name;
    flat_multimap<int, vector<Symbol>> rules;
};

