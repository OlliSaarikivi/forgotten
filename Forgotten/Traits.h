#pragma once

#include "Tokens.h"

template<typename TRow, typename TContainer> struct IsMultiset : std::false_type {};
template<typename TRow> struct IsMultiset<TRow, Multiset<TRow>> : std::true_type{};
template<typename TRow> struct IsMultiset<TRow, FlatMultiset<TRow>> : std::true_type{};