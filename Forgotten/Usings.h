#pragma once

using std::string;
using std::vector;
using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;
using std::array;
using std::function;
using std::set;
using std::multiset;
using std::pair;
using std::tuple;

using boost::container::flat_set;
using boost::container::flat_multiset;

using glm::vec2;

template<typename T>
using Vector = vector<T>;

template<typename T>
using Set = set<T>;

template<typename T>
using FlatSet = flat_set<T>;

template<typename T>
using Multiset = multiset<T>;

template<typename T>
using FlatMultiset = flat_multiset<T>;