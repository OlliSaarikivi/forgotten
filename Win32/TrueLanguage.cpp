#include "stdafx.h"
#include "TrueLanguage.h"
#include "TrueSentenceInterpreter.h"

#include <numeric>

// CYK algorithm adapted from Lange & Leiss (2009) "To CNF or not to CNF?
// An Efficient Yet Presentable Version of the CYK Algorithm".

using boost::multi_array;

optional<shared_ptr<ParseTree2NF>> applyProductionOnce(const shared_ptr<ParseTree2NF>& unit, const shared_ptr<ParseTree2NF>& production)
{
	if (unit->derivation) {
		optional<shared_ptr<ParseTree2NF>> expanded;
		if (unit->derivation->first && (expanded = applyProductionOnce(unit->derivation->first, production)))
			return std::make_shared<ParseTree2NF>(unit->symbol, ParseTree2NF::DerivationType{ *expanded, unit->derivation->second });
		if (unit->derivation->second && (expanded = applyProductionOnce(unit->derivation->second, production)))
			return std::make_shared<ParseTree2NF>(unit->symbol, ParseTree2NF::DerivationType{ unit->derivation->first, *expanded });
	}
	else if (unit->symbol == production->symbol) {
		return production;
	}
	return none;
}

shared_ptr<ParseTree2NF> saturateApplyProduction(const shared_ptr<ParseTree2NF>& unit, const shared_ptr<ParseTree2NF>& production)
{
	shared_ptr<ParseTree2NF> current = unit;
	optional<shared_ptr<ParseTree2NF>> maybeNext = none;
	while (true) {
		maybeNext = applyProductionOnce(current, production);
		if (maybeNext)
			current = *maybeNext;
		else
			return current;
	}
}

Grammar::Grammar(vector<Symbol*> terminals, vector<NonTerminal*> original_non_terminals)
	: terminals(terminals), original_non_terminals(original_non_terminals)
{
	flat_set<Symbol*> terminals_set;
	unordered_map<Symbol*, Symbol*> replacements;
	for (const auto& terminal : terminals) {
		terminals_set.emplace(terminal);
		replacements.emplace(terminal, terminal);
		terminal->unit_from.clear();
		terminal->unit_from_derivations.clear();
		terminal->null_derivation = nullptr;
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
			}
			else {
				auto tail = replacements[rule[size - 1]];
				for (auto i = size - 2; i > 1; --i) {
					auto head = replacements[rule[i]];
					auto suffix_rule = std::make_pair(head, tail);
					auto search = suffix_abbreviations.find(suffix_rule);
					if (search != suffix_abbreviations.end()) {
						tail = search->second;
					}
					else {
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
	using NullabilityReq = vector<tuple<Symbol*, Symbol*, shared_ptr<ParseTree2NF>>>;
	unordered_map<Symbol*, NullabilityReq> occurs;
	for (auto& A : non_terminals) {
		// Populate the requirements for this non-terminal being nullable
		for (const auto& rule : A->rules) {
			if (rule.first) {
				auto first_parse = std::make_shared<ParseTree2NF>(rule.first, none);
				if (rule.second) {
					auto second_parse = std::make_shared<ParseTree2NF>(rule.second, none);
					auto req1 = occurs.emplace(rule.first, NullabilityReq{});
					req1.first->second.emplace_back(A, rule.second,
						std::make_shared<ParseTree2NF>(A, ParseTree2NF::DerivationType{ first_parse, second_parse }));
					auto req2 = occurs.emplace(rule.second, NullabilityReq{});
					req2.first->second.emplace_back(A, rule.first,
						std::make_shared<ParseTree2NF>(A, ParseTree2NF::DerivationType{ first_parse, second_parse }));
				}
				else {
					auto req = occurs.emplace(rule.first, NullabilityReq{});
					req.first->second.emplace_back(A, nullptr,
						std::make_shared<ParseTree2NF>(A, ParseTree2NF::DerivationType{ first_parse, nullptr }));
				}
			}
		}
		// Mark trivially nullable non-terminals
		if (!A->rules.empty() && !(A->rules.begin()->first)) {
			nullables.emplace(A);
			todo.emplace_back(A);
			A->null_derivation = std::make_shared<ParseTree2NF>(A,
				ParseTree2NF::DerivationType{ nullptr, nullptr });
		}
	}
	// Using the requirements check which actually are nullable
	while (!todo.empty()) {
		auto B = todo.back();
		todo.pop_back();
		for (const auto& req : occurs[B]) {
			auto A = get<0>(req);
			if (get<1>(req) && nullables.find(get<1>(req)) == nullables.end()) {
				continue;
			}
			if (nullables.find(A) == nullables.end()) {
				nullables.emplace(A);
				todo.emplace_back(A);
				assert(B->null_derivation);
				auto one_applied = applyProductionOnce(get<2>(req), B->null_derivation);
				assert(one_applied);
				if (get<1>(req)) {
					assert(get<1>(req)->null_derivation);
					auto both_applied = applyProductionOnce(*one_applied, get<1>(req)->null_derivation);
					assert(both_applied);
					A->null_derivation = *both_applied;
				}
				else
					A->null_derivation = *one_applied;
			}
		}
	}
	// Add first step in the inverse unit relation
	for (auto& A : non_terminals) {
		for (auto& rule : A->rules) {
			if (rule.first) {
				auto first_parse = std::make_shared<ParseTree2NF>(rule.first, none);
				if (rule.second) {
					auto second_parse = std::make_shared<ParseTree2NF>(rule.second, none);
					if (nullables.find(rule.second) != nullables.end()) {
						rule.first->unit_from.emplace(A);
						assert(rule.second->null_derivation);
						rule.first->unit_from_derivations.emplace(A,
							std::make_shared<ParseTree2NF>(A, ParseTree2NF::DerivationType{ first_parse, rule.second->null_derivation }));
					}
					if (nullables.find(rule.first) != nullables.end()) {
						rule.second->unit_from.emplace(A);

						assert(rule.first->null_derivation);
						rule.first->unit_from_derivations.emplace(A,
							std::make_shared<ParseTree2NF>(A, ParseTree2NF::DerivationType{ rule.first->null_derivation, second_parse }));
					}
				}
				else {
					rule.first->unit_from.emplace(A);
					rule.first->unit_from_derivations.emplace(A,
						std::make_shared<ParseTree2NF>(A, ParseTree2NF::DerivationType{ first_parse, nullptr }));
				}
			}
		}
	}
	// Transitively close the inverse unit relation. The trivial implementation below may need
	// a number of iterations equal to the depth of the grammar to terminate.
	vector<Symbol*> symbols;
	for (auto& symbol : terminals) {
		symbols.emplace_back(symbol);
	}
	for (auto& symbol : non_terminals) {
		symbols.emplace_back(symbol);
	}
	bool done = false;
	while (!done) {
		done = true;
		for (auto symbol : symbols) {
			for (const auto source : symbol->unit_from) {
				for (const auto source_unit_source : source->unit_from) {
					auto result = symbol->unit_from.emplace(source_unit_source);
					if (result.second) {
						const auto& to_source = source->unit_from_derivations.at(source_unit_source);
						const auto& to_symbol = symbol->unit_from_derivations.at(source);
						auto combined = applyProductionOnce(to_source, to_symbol);
						assert(combined);
						symbol->unit_from_derivations.emplace(source_unit_source, *combined);
						done = false;
					}
				}
			}
		}
	}
}

struct DerivationEntry
{
	int cost;
	shared_ptr<ParseTree2NF> derivation;
};

void addMinimum(flat_map<Symbol*, DerivationEntry>& map, Symbol* symbol, int cost,
	function<shared_ptr<ParseTree2NF>()> make_derivation)
{
	auto search = map.find(symbol);
	if (search == map.end() || cost < search->second.cost) {
		DerivationEntry entry{ cost, make_derivation() };
		auto result = map.emplace(symbol, entry);
		if (!result.second)
			result.first->second = entry;
	}
}

void addUnitClosure(flat_map<Symbol*, DerivationEntry>& map, Symbol* symbol, int cost,
	function<shared_ptr<ParseTree2NF>()> make_derivation)
{
	addMinimum(map, symbol, cost, make_derivation);
	auto best = map.at(symbol).derivation;
	for (const auto& source : symbol->unit_from) {
		addMinimum(map, source, cost, [&]() {
			auto combined = applyProductionOnce(symbol->unit_from_derivations.at(source), best);
			assert(combined);
			return *combined;
		});
	}
}

enum class RewriteCmd {
	EXPAND, POP
};

unique_ptr<ParseTree> toOriginals(const shared_ptr<ParseTree2NF>& root_2nf)
{
	stack<RewriteCmd> cmds;
	stack<shared_ptr<ParseTree2NF>> todo;
	stack<unique_ptr<ParseTree>> rewritten;
	cmds.emplace(RewriteCmd::EXPAND);
	todo.emplace(root_2nf);
	while (!cmds.empty()) {
		auto cmd = cmds.top();
		cmds.pop();
		if (cmd == RewriteCmd::POP) {
			auto popped = std::move(rewritten.top());
			rewritten.pop();
			if (!rewritten.empty())
				rewritten.top()->derivation->push_back(std::move(popped));
			else
				return popped;
		}
		else {
			auto current = todo.top();
			todo.pop();
			if (current->derivation) {
				if (current->symbol->original) {
					rewritten.push(std::make_unique<ParseTree>(current->symbol->original, vector<unique_ptr<ParseTree>>{}));
					cmds.emplace(RewriteCmd::POP);
				}
				if (current->derivation->first) {
					if (current->derivation->second) {
						cmds.emplace(RewriteCmd::EXPAND);
						todo.emplace(current->derivation->second);
					}
					cmds.emplace(RewriteCmd::EXPAND);
					todo.emplace(current->derivation->first);
				}
			}
			else {
				assert(!current->symbol->original);
				rewritten.top()->derivation->push_back(
					std::make_unique<ParseTree>(current->symbol, none));
			}
		}
	}
	assert(false); // Ran out of commands
}

pair<unique_ptr<ParseTree>, int> Grammar::parse(const vector<flat_map<Symbol*, int>>& sentence, const NonTerminal* into)
{
	auto n = sentence.size();
	auto chart = multi_array<flat_map<Symbol*, DerivationEntry>, 2>(boost::extents[n][n]);
	for (auto i = decltype(n)(0); i < n; ++i) {
		auto map = chart[i][i];
		for (const auto& entry : sentence[i]) {
			addUnitClosure(map, entry.first, entry.second, [&]() {
				return std::make_shared<ParseTree2NF>(entry.first, none);
			});
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
								int combined_cost = first_entry->second.cost + second_entry->second.cost;
								addUnitClosure(chart[i][j], non_terminal.get(), combined_cost, [&]() {
									return std::make_shared<ParseTree2NF>(non_terminal.get(),
										ParseTree2NF::DerivationType{ first_entry->second.derivation, second_entry->second.derivation });
								});
							}
						}
					}
				}
			}
		}
	}

	auto into_2nf = non_terminal_mapping.at(into);
	auto result = chart[0][n - 1].find(into_2nf);
	if (result != chart[0][n - 1].end()) {
		return{ toOriginals(result->second.derivation) , result->second.cost };
	}
	else return{ nullptr, 0 };
}