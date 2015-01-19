#include "stdafx.h"
#include "TrueLanguage.h"
#include "TrueSentenceInterpreter.h"

#include <numeric>

// CYK algorithm adapted from Lange & Leiss (2009) "To CNF or not to CNF?
// An Efficient Yet Presentable Version of the CYK Algorithm". The 2NF requirement
// has been dropped.

using boost::multi_array;

Grammar::Grammar()
{
    flat_set<Symbol*> nullables;
    vector<Symbol*> todo;
    using NullabilityReq = vector<pair<Symbol*, vector<Symbol*>>>;
    unordered_map<Symbol*, NullabilityReq> occurs;
    for (const auto& A : non_terminals) {
        // Populate the requirements for this non-terminal being nullable
        for (const auto& rule : A.rules) {
            for (auto B = rule.begin(), rule_end = rule.end(); B != rule_end; ++B) {
                auto req = occurs.emplace(*B, NullabilityReq{});
                auto req_nullable = vector<Symbol*>();
                req_nullable.insert(req_nullable.end(), rule.begin(), B);
                req_nullable.insert(req_nullable.end(), std::next(B), rule.end());
            }
        }
        // Mark trivially nullable non-terminals
        if (!A.rules.empty() && A.rules.begin()->size() == 0) {
            nullables.emplace(&A);
            todo.emplace_back(&A);
        }
    }
    // Using the requirements check which actually are nullable
    while (!todo.empty()) {
        auto B = todo.back();
        todo.pop_back();
        for (const auto& req : occurs[B]) {
            auto A = req.first;
            auto and_nullable = [&](bool left, Symbol* right) { return left && nullables.find(right) != nullables.end(); };
            bool all_nullable = std::accumulate(req.second.begin(), req.second.end(), true, and_nullable);
            if (all_nullable && nullables.find(A) == nullables.end()) {
                nullables.emplace(A);
                todo.emplace_back(A);
            }
        }
    }
    // Add first step in the inverse unit relation
    for (auto& A : non_terminals) {
        for (auto& rule : A.rules) {
            for (auto& B : rule) {
                auto and_nullable_or_B = [&](bool left, Symbol* right) {
                    return left && (nullables.find(right) != nullables.end() || right == B);
                };
                bool others_nullable = std::accumulate(rule.begin(), rule.end(), true, and_nullable_or_B);
                if (others_nullable) {
                    B->unit_from.emplace(A);
                }
            }
        }
    }
    // Transitively close the inverse unit relation. The trivial implementation below may need
    // a number of iterations equal to the depth of the grammar to terminate.
    vector<Symbol&> symbols;
    for (auto& symbol : non_terminals) {
        symbols.emplace_back(symbol);
    }
    for (auto& symbol : terminals) {
        symbols.emplace_back(symbol);
    }
    bool done = false;
    while (!done) {
        done = true;
        for (auto& symbol : symbols) {
            auto size_before = symbol.unit_from.size();
            for (const auto& source : symbol.unit_from) {
                for (const auto& source_unit_source : source->unit_from) {
                    symbol.unit_from.emplace(source_unit_source);
                }
            }
            if (size_before != symbol.unit_from.size()) {
                done = false;
            }
        }
    }
}

void Grammar::parse(const vector<flat_set<WeightedSymbol>>& sentence)
{
    auto n = sentence.size();
    auto chart = multi_array<flat_set<WeightedSymbol>, 2>(boost::extents[n][n]);
    for (auto i = 0*n; i < n; ++i) {
        chart[i][i] = sentence[i];
    }
    for (auto first = 0 * n; first < n - 1; ++first) {
        for (auto last = first + 1; last < n; ++last) {
            for (const auto& non_terminal : non_terminals) {
                std::function<void()> split = [&]() {
                    split(); // TODO
                };
            }
        }
    }
}