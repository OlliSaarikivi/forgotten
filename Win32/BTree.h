#pragma once

#include "Columnar.h"

template<class TGetKey, class TLess, class... TValues> class BTree {
	using KeyType = typename TGetKey::KeyType;
	using KeyLess = typename TGetKey::Less;
	static const KeyType LeastKey = TGetKey::LeastKey;

	using Records = Columnar<TValues...>;

	static const size_t KeysInCacheLine = CACHE_LINE_SIZE / sizeof(KeyType);
	static const size_t MaxKeys = KeysInCacheLine < 2 ? 2 : KeysInCacheLine;
	static const size_t MaxInnerLevels = 32; // Gives at least 2^32
	static const size_t MaxRecords = 32;

	struct InnerNode;
	struct LeafNode;
	struct NodePtr {
		uintptr_t value;

		NodePtr() : value(reinterpret_cast<uintptr_t>(nullptr)) {}
		NodePtr(const InnerNode* node) : value(reinterpret_cast<uintptr_t>(node)) {}
		NodePtr(const LeafNode* node) : value(reinterpret_cast<uintptr_t>(node) | 1u) {}

		NodePtr& operator=(const InnerNode* node) {
			value = reinterpret_cast<uintptr_t>(node);
			return *this;
		}
		NodePtr& operator=(const LeafNode* node) {
			value = reinterpret_cast<uintptr_t>(node) | 1u;
			return *this;
		}
		NodePtr& operator=(nullptr_t nullPtr) {
			value = reinterpret_cast<uintptr_t>(nullptr);
			return *this;
		}

		bool isInner() {
			return (value & 1u) == 0;
		}
		bool isLeaf() {
			return (value & 1u) != 0;
		}

		InnerNode* inner() {
			assert(isInner());
			return reinterpret_cast<InnerNode*>(value);
		}
		LeafNode* leaf() {
			assert(isLeaf());
			return reinterpret_cast<LeafNode*>(value & ~((uintptr_t)1u));
		}
	};

	struct InnerNode {
		array<KeyType, MaxKeys> keys;
		array<NodePtr, MaxKeys + 1> children;
		InnerNode* parent;
	};
	struct LeafNode {
		LeafNode* next;
		LeafNode* previous;
		LeafNode* nextDirty;
		InnerNode* parent;
		Records records;

		LeafNode() : next(nullptr), previous(nullptr), parent(nullptr), nextDirty(nullptr) {}
		LeafNode(LeafNode* next, LeafNode* previous, InnerNode* parent) : next(next), previous(previous), parent(parent), nextDirty(nullptr) {}
	};

	static_assert(alignof(InnerNode) >= 2, "Can not use low byte for type info because InnerNode has alignment of 1 byte.");
	static_assert(alignof(LeafNode) >= 2, "Can not use low byte for type info because LeafNode has alignment of 1 byte.");

	template<class... TSomeValues> class Iterator {
		using IteratorType = Records::Iterator<TSomeValues...>;

		LeafNode* current;
		IteratorType pos;

	public:
		using reference = typename IteratorType::reference;
		using pointer = typename IteratorType::pointer;
		using iterator_category = std::forward_iterator_tag;

		Iterator(LeafNode* current) : current(current) {
			pos = current->records.begin<TSomeValues...>();
		}
		Iterator(LeafNode* current, IteratorType pos) : current(current), pos(pos) {}

		Iterator& operator++() {
			++pos;
			if (current->next != nullptr && pos == current->records.end<TSomeValues...>()) {
				current = current->next;
				pos = current->records.begin<TSomeValues...>();
			}
			return *this;
		}
		Iterator operator++(int) {
			Iterator old = *this;
			this->operator++();
			return old;
		}

		reference operator*() const {
			return *pos;
		}
		pointer operator->() const {
			return pos.operator->();
		}

		friend bool operator==(const Iterator& left, const Iterator& right) {
			return left.current == right.current && left.pos == right.pos;
		}
		friend bool operator!=(const Iterator& left, const Iterator& right) {
			return !(left == right);
		}

		friend void swap(Iterator& left, Iterator& right) {
			using std::swap;
			swap(left.current, right.current);
			swap(left.pos, right.pos);
		}
	};

	boost::object_pool<InnerNode> innerPool;
	boost::object_pool<LeafNode> leafPool;
	NodePtr root;
	LeafNode* firstLeaf;
	LeafNode* lastLeaf;
	LeafNode* dirty;
	size_t innerLevels;
	Records scratch;

	template<uint8_t BEGIN, uint8_t END, int N> uint8_t findSlot(const array<KeyType, N>& keys, KeyType key) {
		for (uint_fast8_t i = BEGIN; i < END; ++i) {
			if (KeyLess()(key, keys[i]))
				return i;
		}
		return END;
	}

	struct PathEntry {
		KeyType upperBound;
		NodePtr node;
	};
	void findLeaf(KeyType key, PathEntry* path) {
		while (path->node.isInner()) {
			InnerNode* inner = path->node.inner();
			auto slot = findSlot<0, MaxKeys>(inner->keys, key);
			KeyType newBound = slot == MaxKeys ? path->upperBound : inner->keys[slot];
			++path;
			path->upperBound = newBound;
			path->node = inner->children[slot];
		}
	}

	template<class TIter> void moveMerge(Records& target, TIter newIter, TIter newEnd) {
		scratch.reserve(target.size() + (newEnd - newIter));

		auto oldIter = ::begin(target);
		auto oldEnd = ::end(target);
		while (true) {
			if (oldIter == oldEnd) {
				while (newIter != newEnd)
					scratch.unsafePushBack(std::move(*newIter++));
				break;
			}
			if (newIter == newEnd) {
				while (oldIter != oldEnd)
					scratch.unsafePushBack(std::move(*oldIter++));
				break;
			}

			if (TLess()(*oldIter, *newIter))
				scratch.unsafePushBack(std::move(*oldIter++));
			else if (TLess()(*newIter, *oldIter))
				scratch.unsafePushBack(std::move(*newIter++));
			else { // New overwrites old
				scratch.unsafePushBack(std::move(*newIter++));
				++oldIter;
			}
		}

		swap(scratch, target);
		scratch.clear();
	}
	template<class TIter> TIter moveMergeUntil(Records& target, TIter newIter, TIter newEnd, KeyType upperBound) {
		size_t sizeAtLeast = target.size();
		scratch.reserve(sizeAtLeast);

		auto oldIter = ::begin(target);
		auto oldEnd = ::end(target);
		while (true) {
			if (oldIter == oldEnd) {
				while (newIter != newEnd && KeyLess()(TGetKey()(*newIter), upperBound))
					scratch.pushBack(std::move(*newIter++));
				break;
			}
			if (newIter == newEnd || !KeyLess()(TGetKey()(*newIter), upperBound)) {
				while (oldIter != oldEnd)
					scratch.unsafePushBack(std::move(*oldIter++));
				break;
			}

			if (TLess()(*oldIter, *newIter)) {
				scratch.unsafePushBack(std::move(*oldIter++));
			}
			else if (TLess()(*newIter, *oldIter)) {
				++sizeAtLeast;
				scratch.reserve(sizeAtLeast);
				scratch.unsafePushBack(std::move(*newIter++));
			}
			else { // New overwrites old
				scratch.unsafePushBack(std::move(*newIter++));
				++oldIter;
			}
		}

		swap(scratch, target);
		scratch.clear();
		return newIter;
	}

	void insertBalance() {
		while (dirty) {
			LeafNode* current = dirty;
			dirty = current->nextDirty;
			current->nextDirty = nullptr;

			if (current->records.size() > MaxRecords) {
				size_t overage = current->records.size() - MaxRecords;

				if (current->next && current->previous) {
					size_t nextSpace = MaxRecords - current->next->records.size();
					size_t previousSpace = MaxRecords - current->previous->records.size();
					if (overage < nextSpace + previousSpace) {
						// Find amount to move to next
						auto toNextBegin = ::end(current->records) - nextSpace;
						auto toNextEnd = ::end(current->records);
						optional<decltype(toNextBegin)> bestBegin;
						while (size_t(toNextEnd - toNextBegin) >= overage) {
							if (KeyLess()(TGetKey()(*(toNextBegin - 1)), TGetKey()(*toNextBegin)))
								bestBegin = toNextBegin;
							++toNextBegin;
						}
						// And to previous
						auto toPreviousBegin = ::begin(current->records);
						auto toPreviousEnd = ::begin(current->records) + previousSpace;
						optional<decltype(toPreviousEnd)> bestEnd;
						while (size_t(toPreviousEnd - toPreviousBegin) >= overage) {
							if (KeyLess()(TGetKey()(*(toPreviousEnd - 1)), TGetKey()(*toPreviousEnd)))
								bestEnd = toPreviousEnd;
							--toPreviousEnd;
						}
						
						// Try to move less if possible
						optional<decltype(toNextBegin)> finalBegin = bestBegin;
						optional<decltype(toPreviousEnd)> finalEnd = bestEnd;
						do {
							if (bestBegin) {
								size_t numToPrevious = finalEnd ? (*finalEnd - toPreviousBegin) : 0;
								toNextBegin = *bestBegin;
								bestBegin = none;
								while (toNextBegin != toNextEnd && ((toNextEnd - toNextBegin) + numToPrevious > overage)) {
									if (KeyLess()(TGetKey()(*(toNextBegin - 1)), TGetKey()(*toNextBegin))) {
										bestBegin = toNextBegin;
										finalBegin = bestBegin;
										break;
									}
									++toNextBegin;
								}
							}
							if (bestEnd) {
								size_t numToNext = finalBegin ? (toNextEnd - *finalBegin) : 0;
								toPreviousEnd = *bestEnd;
								bestEnd = none;
								while (toPreviousBegin != toPreviousEnd && ((toPreviousEnd - toPreviousBegin) + numToNext > overage)) {
									if (KeyLess()(TGetKey()(*(toPreviousEnd - 1)), TGetKey()(*toPreviousEnd))) {
										bestEnd = toPreviousEnd;
										finalEnd = bestEnd;
										break;
									}
									--toPreviousEnd;
								}
							}
						} while (bestBegin || bestEnd);

						if (finalBegin) {
							current->next->records.moveInsert(::begin(current->next->records), *finalBegin, toNextEnd);
							current->records.erase(*finalBegin, toNextEnd);
						}
						if (finalEnd) {
							current->previous->records.moveInsert(::begin(current->previous->records), toPreviousBegin, *finalEnd);
							current->records.erase(toPreviousBegin, *finalEnd);
						}
					}
				}
				else if (current->next) {
					size_t nextSpace = MaxRecords - current->next->records.size();
					if (overage < nextSpace) {
						auto toNextBegin = ::end(current->records) - nextSpace;
						auto toNextEnd = ::end(current->records);
						optional<decltype(toNextBegin)> bestBegin;
						while (size_t(toNextEnd - toNextBegin) >= overage) {
							if (KeyLess()(TGetKey()(*(toNextBegin - 1)), TGetKey()(*toNextBegin)))
								bestBegin = toNextBegin;
							++toNextBegin;
						}
						if (bestBegin) {
							current->next->records.moveInsert(::begin(current->next->records), *bestBegin, toNextEnd);
							current->records.erase(*bestBegin, toNextEnd);
							continue; // Next dirty leaf
						}
					}
				}
				else if (current->previous) {
					size_t previousSpace = MaxRecords - current->previous->records.size();
					if (overage < previousSpace) {
						auto toPreviousBegin = ::begin(current->records);
						auto toPreviousEnd = ::begin(current->records) + previousSpace;
						optional<decltype(toPreviousEnd)> bestEnd;
						while (size_t(toPreviousEnd - toPreviousBegin) >= overage) {
							if (KeyLess()(TGetKey()(*(toPreviousEnd - 1)), TGetKey()(*toPreviousEnd)))
								bestEnd = toPreviousEnd;
							--toPreviousEnd;
						}
						if (bestEnd) {
							current->previous->records.moveInsert(::begin(current->previous->records), toPreviousBegin, *bestEnd);
							current->records.erase(toPreviousBegin, *bestEnd);
							continue; // Next dirty leaf
						}
					}
				}

				// TODO: update keys in inner nodes in code above
				// Split
			}
		}
	}

public:
	BTree() : root(leafPool.construct()), firstLeaf(root.leaf()), lastLeaf(firstLeaf), innerLevels(0) {}

	template<class T, class... Ts> auto begin() {
		return Iterator<T, Ts...>(firstLeaf);
	}
	auto begin() {
		return begin<TValues...>();
	}

	template<class T, class... Ts> auto end() {
		return Iterator<T, Ts...>(lastLeaf, lastLeaf->records.end<T, Ts...>());
	}
	auto end() {
		return end<TValues...>();
	}

	template<class TIter> void moveInsertSorted(TIter iter, TIter end) {
		PathEntry path[MaxInnerLevels];
		path[0].upperBound = LeastKey;
		path[0].node = root;
		size_t currentDepth = 0;
		while (iter != end) {
			auto key = TGetKey()(*iter);
			while (path[currentDepth].upperBound != LeastKey && !(KeyLess()(key, path[currentDepth].upperBound)))
				--currentDepth;
			findLeaf(key, path + currentDepth);
			LeafNode* leaf = path[innerLevels].node.leaf();

			leaf->nextDirty = dirty;
			dirty = leaf;
			if (path[innerLevels].upperBound == LeastKey) {
				moveMerge(leaf->records, iter, end);
				break;
			}
			iter = moveMergeUntil(leaf->records, iter, end, path[innerLevels - 1].upperBound);
		}
		insertBalance();
	}
};

template<class... Ts> inline auto begin(BTree<Ts...>& t) { return t.begin(); }
template<class... Ts> inline auto end(BTree<Ts...>& t) { return t.end(); }