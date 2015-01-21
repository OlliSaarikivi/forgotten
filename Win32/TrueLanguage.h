#pragma once

struct NonTerminal;

struct Symbol
{
    Symbol(string name) : name(name) {}
    Symbol(const Symbol&) = delete;
    Symbol& operator=(const Symbol&) = delete;

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
    struct Rule2SizeLess
    {
        inline bool operator()(const pair<Symbol*, Symbol*>& left, const pair<Symbol*, Symbol*>& right) const
        {
            return (!left.first && right.first) || (!left.second && right.first && right.second);
        }
    };
}

struct NonTerminal : public Symbol
{
    flat_multiset<vector<Symbol*>, impl::RuleSizeLess> rules;
};

struct NonTerminal2 : public Symbol
{
    flat_multiset<pair<Symbol*, Symbol*>, impl::Rule2SizeLess> rules;
};

struct WeightedSymbol
{
    int weight;
    Symbol* symbol;
    inline bool operator<(const WeightedSymbol& right) const
    {
        return weight < right.weight;
    }
};

struct Grammar
{
    Grammar(unique_ptr<vector<Symbol>> terminals, unique_ptr<vector<NonTerminal>> non_terminals);
    void parse(const vector<flat_multiset<WeightedSymbol>>& sentence);
private:
    unique_ptr<vector<Symbol>> terminals;
    unique_ptr<vector<NonTerminal>> original_non_terminals;
    vector<NonTerminal2> non_terminals;
};