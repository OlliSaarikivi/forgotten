#pragma once

struct NonTerminal;

struct Symbol
{
    string name;
    flat_set<NonTerminal*> unit_from;
};

namespace impl
{
    struct RuleSizeLess
    {
        inline bool operator()(const vector<Symbol*>& left, const vector<Symbol*>& right) const
        {
            return left.size() < right.size();
        }
    };
}

struct NonTerminal : public Symbol
{
    flat_multiset<vector<Symbol*>, impl::RuleSizeLess> rules;
};

struct WeightedSymbol
{
    double weight;
    Symbol* symbol;
    inline bool operator<(const WeightedSymbol& right) const
    {
        return std::less<Symbol*>()(symbol, right.symbol);
    }
};

struct Grammar
{
    Grammar();
    void parse(const vector<flat_set<WeightedSymbol>>& sentence);
private:
    vector<Symbol> terminals;
    vector<NonTerminal> non_terminals;
};