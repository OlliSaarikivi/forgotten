#include "stdafx.h"
#include "TrueLanguage.h"
#include "TrueSentenceInterpreter.h"

#include <numeric>

// CYK algorithm adapted from Lange & Leiss (2009) "To CNF or not to CNF?
// An Efficient Yet Presentable Version of the CYK Algorithm".

using boost::multi_array;

Grammar::Grammar(vector<Symbol*> terminals, vector<NonTerminal*> original_non_terminals)
    : terminals(terminals), original_non_terminals(original_non_terminals)
{
    flat_set<Symbol*> terminals_set;
    unordered_map<Symbol*, Symbol*> replacements;
    for (const auto& terminal : terminals) {
        terminals_set.emplace(terminal);
        replacements.emplace(terminal, terminal);
        terminal->unit_from.clear();
		terminal->unit_from_rules.clear();
    }
    // Create a 2NF non terminal for each original non terminal
    unordered_map<NonTerminal*, NonTerminal2NF*> nt_replacements;
    for (const auto& original : original_non_terminals) {
        auto root_replacement = std::make_unique<NonTerminal2NF>(original->name);
        replacements.emplace(original, root_replacement.get());
        nt_replacements.emplace(original, root_replacement.get());
        non_terminals.emplace_back(std::move(root_replacement));
    }
    // Create new 2NF non terminals to implement rules longer than 2
    unordered_map<pair<Symbol*, Symbol*>, Symbol*> suffix_abbreviations;
    for (const auto& original : original_non_terminals) {
        auto nt_replacement = nt_replacements[original];
        for (auto& rule : original->rules) {
            auto size = rule.size();
            if (size < 2) {
                nt_replacement->rules.emplace(
                    size >= 1 ? replacements[rule[0]] : nullptr,
                    size >= 2 ? replacements[rule[1]] : nullptr);
            } else {
                auto tail = replacements[rule[size - 1]];
                for (auto i = size - 2; i > 1; --i) {
                    auto head = replacements[rule[i]];
                    auto suffix_rule = std::make_pair(head, tail);
                    auto search = suffix_abbreviations.find(suffix_rule);
                    if (search != suffix_abbreviations.end()) {
                        tail = search->second;
                    } else {
                        auto tail_symbol = std::make_unique<NonTerminal2NF>("<" + head->name + "," + tail->name + ">");
                        tail_symbol->rules.emplace(suffix_rule);
                        suffix_abbreviations.emplace(suffix_rule, tail_symbol);
                        tail = tail_symbol.get();
                        non_terminals.emplace_back(std::move(tail_symbol));
                    }
                }
            }
        }
    }

    flat_set<Symbol*> nullables;
    vector<Symbol*> todo;
    using NullabilityReq = vector<pair<Symbol*, Symbol*>>;
    unordered_map<Symbol*, NullabilityReq> occurs;
    for (auto& A : non_terminals) {
        // Populate the requirements for this non-terminal being nullable
        for (const auto& rule : A->rules) {
            if (rule.first) {
                if (rule.second) {
                    auto req = occurs.emplace(rule.first, NullabilityReq{});
                    req.first->second.emplace_back(A.get(), rule.second);
                    auto req = occurs.emplace(rule.second, NullabilityReq{});
                    req.first->second.emplace_back(A.get(), rule.first);
                } else {
                    auto req = occurs.emplace(rule.first, NullabilityReq{});
                    req.first->second.emplace_back(A.get(), nullptr);
                }
            }
        }
        // Mark trivially nullable non-terminals
        if (!A->rules.empty() && !(A->rules.begin()->first)) {
            nullables.emplace(&A);
            todo.emplace_back(&A);
			A->null_derivation = {A, {nullptr, nullptr}};
        }
    }
    // Using the requirements check which actually are nullable
    while (!todo.empty()) {
        auto B = todo.back();
        todo.pop_back();
        for (const auto& req : occurs[B]) {
            auto A = req.first;
            if (req.second && nullables.find(req.second) == nullables.end()) {
                continue;
            }
            if (nullables.find(A) == nullables.end()) {
                nullables.emplace(A);
                todo.emplace_back(A);
            }
        }
    }
    // Add first step in the inverse unit relation
    for (auto& A : non_terminals) {
        for (auto& rule : A->rules) {
            if (rule.first) {
                if (rule.second) {
                    if (nullables.find(rule.second) != nullables.end()) {
                        rule.first->unit_from.emplace(A);
						rule.first->unit_from_rules.emplace(A, rule.first);
                    }
                    if (nullables.find(rule.first) != nullables.end()) {
                        rule.second->unit_from.emplace(A);
						rule.second->unit_from_rules.emplace(A, rule.second);
                    }
                } else {
                    rule.first->unit_from.emplace(A);
					rule.first->unit_from_rules.emplace(A, rule.first);
                }
            }
        }
    }
    // Transitively close the inverse unit relation. The trivial implementation below may need
    // a number of iterations equal to the depth of the grammar to terminate.
    vector<Symbol*> symbols;
    for (auto& symbol : terminals) {
        symbols.emplace_back(&symbol);
    }
    for (auto& symbol : non_terminals) {
        symbols.emplace_back(&symbol);
    }
    bool done = false;
    while (!done) {
        done = true;
        for (auto symbol : symbols) {
            for (const auto source : symbol->unit_from) {
                for (const auto source_unit_source : source->unit_from) {
                    auto result = symbol->unit_from.emplace(source_unit_source);
					if (result.second) {
						auto combined = source->unit_from_rules.at(source_unit_source);
						const auto& from_source = symbol->unit_from_rules.at(source);
						combined.insert(end(combined), begin(from_source), end(from_source));
						symbol->unit_from_rules.emplace(source_unit_source, combined);
						done = false;
					}
                }
            }
        }
    }
}

struct SymbolEntry
{
	int weight;
	pair<Symbol*, Symbol*> rule;
	int second_index;
};

void addMinimum(flat_map<Symbol*, SymbolEntry>& map, Symbol* symbol, SymbolEntry candidate)
{
    auto search = map.find(symbol);
    if (search == map.end() || candidate.weight < search->second.weight) {
        auto result = map.emplace(symbol, candidate);
		if (!result.second)
			result.first->second = candidate;
    }
}

void addUnitClosure(flat_map<Symbol*, SymbolEntry>& map, Symbol* symbol, SymbolEntry candidate)
{
	addMinimum(map, symbol, candidate);
    for (const auto& entry : symbol->unit_from) {
		addMinimum(map, entry, );
    }
}

void Grammar::parse(const vector<flat_map<Symbol*, int>>& sentence)
{
    auto n = sentence.size();
    auto chart = multi_array<flat_map<Symbol*, SymbolEntry>, 2>(boost::extents[n][n]);
    for (auto i = decltype(n)(0); i < n; ++i) {
        auto map = chart[i][i];
        for (const auto& entry : sentence[i]) {
			addUnitClosure(map, entry.first, {entry.second, {nullptr, nullptr}, -1});
        }
    }
    for (auto j = decltype(n)(1); j < n - 1; ++j) {
        for (auto i = j - 1; i >= 0; --i) {
            for (auto h = i; h < j; ++h) {
                for (const auto& non_terminal : non_terminals) {
                    for (const auto& rule : non_terminal->rules) {
                        if (rule.first && rule.second) {
                            auto first_entry = chart[i][h].find(rule.first);
                            auto second_entry = chart[h + 1][j].find(rule.second);
                            if (first_entry != chart[i][h].end() && second_entry != chart[h + 1][j].end()) {
								int combined_weight = first_entry->second.weight + second_entry->second.weight;
								addUnitClosure(chart[i][j], non_terminal.get(), {combined_weight, rule, h + 1});
                            }
                        }
                    }
                }
            }
        }
    }

	if (chart[0][n - 1].empty()) {
		// TODO: no result
		return;
	}


}