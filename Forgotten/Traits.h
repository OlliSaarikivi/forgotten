#pragma once

#include "Tokens.h"

template<typename TRow, typename TKey, typename TContainer> struct IsMultiset : std::false_type {};
template<typename TRow, typename TKey> struct IsMultiset<TRow, TKey, Multiset<TRow, TKey>> : std::true_type{};
template<typename TRow, typename TKey> struct IsMultiset<TRow, TKey, FlatMultiset<TRow, TKey>> : std::true_type{};