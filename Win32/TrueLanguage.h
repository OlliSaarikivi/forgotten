#pragma once

struct NonTerminal2NF;
struct ParseTree2NF;

struct Symbol
{
    Symbol(string name) : name(name) {}
    Symbol(const Symbol&) = delete;
    Symbol& operator=(const Symbol&) = delete;

    string name;
    flat_set<NonTerminal2NF*> unit_from;
	flat_map<NonTerminal2NF*, ParseTree2NF> unit_from_derivations;
	optional<ParseTree2NF> null_derivation;
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

struct NonTerminal : Symbol
{
    NonTerminal(string name) : Symbol(name) {}

    flat_multiset<vector<Symbol*>, impl::RuleSizeLess> rules;
};

struct NonTerminal2NF : Symbol
{
    NonTerminal2NF(string name) : Symbol(name) {}

    flat_multiset<pair<Symbol*, Symbol*>, impl::Rule2SizeLess> rules;
};

struct ParseTree
{
	Symbol* symbol;
	optional<vector<unique_ptr<ParseTree>>> derivation;
};

struct ParseTree2NF
{
	Symbol* symbol;
	optional<pair<unique_ptr<ParseTree2NF>, unique_ptr<ParseTree2NF>>> derivation;
};

struct Grammar
{
    Grammar(vector<Symbol*> terminals, vector<NonTerminal*> non_terminals);
	unique_ptr<ParseTree> parse(const vector<flat_map<Symbol*, int>>& sentence);
private:
    vector<Symbol*> terminals;
    vector<NonTerminal*> original_non_terminals;
    vector<unique_ptr<NonTerminal2NF>> non_terminals;
};