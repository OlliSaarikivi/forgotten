#pragma once

struct NonTerminal2NF;

struct Symbol
{
    Symbol(string name) : name(name) {}
    Symbol(const Symbol&) = delete;
    Symbol& operator=(const Symbol&) = delete;

    string name;
    flat_set<NonTerminal2NF*> unit_from;
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
    NonTerminal(string name) : Symbol(name) {}

    flat_multiset<vector<Symbol*>, impl::RuleSizeLess> rules;
};

struct NonTerminal2NF : public Symbol
{
    NonTerminal2NF(string name) : Symbol(name) {}

    flat_multiset<pair<Symbol*, Symbol*>, impl::Rule2SizeLess> rules;
};

struct Grammar
{
    Grammar(vector<Symbol*> terminals, vector<NonTerminal*> non_terminals);
    void parse(const vector<flat_map<Symbol*, int>>& sentence);
private:
    vector<Symbol*> terminals;
    vector<NonTerminal*> original_non_terminals;
    vector<unique_ptr<NonTerminal2NF>> non_terminals;
};