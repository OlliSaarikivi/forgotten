#pragma once

#include "Columnar.h"
#include "Row.h"

COL(uint16_t, PerKeyRecordCount) // Limits the number of records that have the same key

template<class TGetKey, class TLess, class... TValues> class BTree {
	using KeyType = typename TGetKey::KeyType;
	using KeyLess = typename TGetKey::Less;
	static const KeyType LeastKey = TGetKey::LeastKey;

	using Records = Columnar<TValues...>;
	using Counts = Columnar<KeyType, PerKeyRecordCount>;

	static const size_t KeysInCacheLine = CACHE_LINE_SIZE / sizeof(KeyType);
	static const size_t MaxKeys = KeysInCacheLine < 2 ? 2 : KeysInCacheLine;
	static const size_t MaxInnerLevels = 32; // Gives at least 2^32

	static const size_t MinRecords = 32;
	static const size_t MaxRecords = 64;

	struct InnerNode;
	struct LeafNode;
	struct NodePtr {
		uintptr_t value;

		NodePtr() : value(reinterpret_cast<uintptr_t>(nullptr)) {}
		NodePtr(const InnerNode* node) : value(reinterpret_cast<uintptr_t>(node)) {}
		NodePtr(const LeafNode* node) : value(reinterpret_cast<uintptr_t>(node) | 1u) {}

		operator bool() { return value != reinterpret_cast<uintptr_t>(nullptr); }

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

		InnerNode() : parent(nullptr) {
			keys.fill(LeastKey);
			children.fill(NodePtr{});
		}
	};
	struct LeafNode {
		LeafNode* next;
		LeafNode* previous;
		LeafNode* nextDirty;
		InnerNode* parent;
		Counts counts;
		Records records;

		LeafNode() : next(nullptr), previous(nullptr), parent(nullptr), nextDirty(nullptr) {}
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

	size_t leaves = 1; // TODO: remove

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

	void incrementCount(Counts& counts, size_t& pos, KeyType key) {
		auto size = counts.size();
		while (pos < size && KeyLess()(KeyType(counts[pos]), key))
			++pos;
		if (pos < size) {
			if (!KeyLess()(key, KeyType(counts[pos]))) {
				assert(PerKeyRecordCount(counts[pos]) < std::numeric_limits<PerKeyRecordCount::type>::max());
				counts[pos].col<PerKeyRecordCount>() += 1;
			}
			else
				counts.insert(pos, makeRow(key, PerKeyRecordCount(1)));
		}
		else
			counts.pushBack(makeRow(key, PerKeyRecordCount(1)));
	}

	template<class TIter> void moveMerge(LeafNode* leaf, TIter newIter, TIter newEnd) {
		Records& target = leaf->records;
		scratch.reserve(target.size() + (newEnd - newIter));

		size_t countsPos = 0;
		auto oldIter = ::begin(target);
		auto oldEnd = ::end(target);
		for (;;) {
			if (oldIter == oldEnd) {
				while (newIter != newEnd) {
					incrementCount(leaf->counts, countsPos, TGetKey()(*newIter));
					scratch.unsafePushBack(std::move(*newIter));
					++newIter;
				}
				break;
			}
			if (newIter == newEnd) {
				while (oldIter != oldEnd)
					scratch.unsafePushBack(std::move(*oldIter++));
				break;
			}

			if (TLess()(*oldIter, *newIter))
				scratch.unsafePushBack(std::move(*oldIter++));
			else if (TLess()(*newIter, *oldIter)) {
				incrementCount(leaf->counts, countsPos, TGetKey()(*newIter));
				scratch.unsafePushBack(std::move(*newIter));
				++newIter;
			}
			else { // New overwrites old. Counts are unchanged.
				scratch.unsafePushBack(std::move(*newIter++));
				++oldIter;
			}
		}

		swap(scratch, target);
		scratch.clear();
	}
	template<class TIter> TIter moveMergeUntil(LeafNode* leaf, TIter newIter, TIter newEnd, KeyType upperBound) {
		Records& target = leaf->records;
		size_t sizeAtLeast = target.size();
		scratch.reserve(sizeAtLeast);

		size_t countsPos = 0;
		auto oldIter = ::begin(target);
		auto oldEnd = ::end(target);
		for (;;) {
			if (oldIter == oldEnd) {
				while (newIter != newEnd && KeyLess()(TGetKey()(*newIter), upperBound)) {
					incrementCount(leaf->counts, countsPos, TGetKey()(*newIter));
					scratch.pushBack(std::move(*newIter));
					++newIter;
				}
				break;
			}
			if (newIter == newEnd || !KeyLess()(TGetKey()(*newIter), upperBound)) {
				while (oldIter != oldEnd) {
					scratch.unsafePushBack(std::move(*oldIter));
					++oldIter;
				}
				break;
			}

			if (TLess()(*oldIter, *newIter)) {
				scratch.unsafePushBack(std::move(*oldIter));
				++oldIter;
			}
			else if (TLess()(*newIter, *oldIter)) {
				incrementCount(leaf->counts, countsPos, TGetKey()(*newIter));
				++sizeAtLeast;
				scratch.reserve(sizeAtLeast);
				scratch.unsafePushBack(std::move(*newIter));
				++newIter;
			}
			else { // New overwrites old. Counts are unchanged.
				scratch.unsafePushBack(std::move(*newIter));
				++newIter;
				++oldIter;
			}
		}

		swap(scratch, target);
		scratch.clear();
		return newIter;
	}

	void innerInsert(InnerNode* inner, KeyType key, NodePtr node) {
		if (!inner->children[0]) {
			inner->children[0] = inner->children[1];
			inner->children[1] = node;
			inner->keys[0] = key;
			for (size_t i = 0; i < MaxKeys - 1; ++i) {
				if (!KeyLess()(inner->keys[i], inner->keys[i + 1])) {
					using std::swap;
					swap(inner->keys[i], inner->keys[i + 1]);
					swap(inner->children[i + 1], inner->children[i + 2]);
				}
			}
		}
		else {
			// Split
			InnerNode* newSibling = innerPool.construct();
			size_t numRight = (MaxKeys + 1) / 2;
			size_t numLeft = (MaxKeys + 1) - numRight;
			for (size_t i = 0; i < MaxKeys + 1; ++i) {
				if (i < numRight)
					newSibling->children[MaxKeys - i] = inner->children[MaxKeys - i];
				else
					newSibling->children[MaxKeys - i] = nullptr;

				if (i < numLeft)
					inner->children[MaxKeys - i] = inner->children[MaxKeys - numRight - i];
				else
					inner->children[MaxKeys - i] = nullptr;
			}
			KeyType middleKey = inner->keys[MaxKeys - numRight];
			for (size_t i = 0; i < MaxKeys; ++i) {
				if (i < numRight - 1)
					newSibling->keys[MaxKeys - 1 - i] = inner->keys[MaxKeys - 1 - i];
				else
					newSibling->keys[MaxKeys - 1 - i] = LeastKey;

				if (i < numLeft - 1)
					inner->keys[MaxKeys - 1 - i] = inner->keys[MaxKeys - 1 - (numRight - 1) - 1 - i];
				else
					inner->keys[MaxKeys - 1 - i] = LeastKey;
			}
			InnerNode* targetNode = KeyLess()(key, middleKey) ? inner : newSibling;
			innerInsert(targetNode, key, node); // Will not split again
			if (inner->parent)
				innerInsert(inner->parent, middleKey, newSibling);
			else {
				InnerNode* parent = innerPool.construct();
				parent->keys.back() = middleKey;
				*(parent->children.end() - 1) = newSibling;
				*(parent->children.end() - 2) = inner;

				inner->parent = parent;
				root = parent;
				++innerLevels;
			}
			newSibling->parent = inner->parent;
		}
	}

	void splitLeaf(LeafNode* leaf) {
		auto numRecords = leaf->records.size();
		auto idealLeafSize = numRecords / 2;
		auto numLeft = decltype(numRecords)(0);
		auto countsLeftEnd = ::begin(leaf->counts);
		auto countsEnd = ::end(leaf->counts);
		for (;;) {
			if (countsLeftEnd == countsEnd)
				break;
			auto newNumLeft = numLeft + PerKeyRecordCount(*countsLeftEnd);
			if (newNumLeft >= idealLeafSize) {
				auto oldDiff = idealLeafSize - numLeft;
				auto newDiff = newNumLeft - idealLeafSize;
				if (newDiff < oldDiff) {
					numLeft = newNumLeft;
					++countsLeftEnd;
				}
				break;
			}
			numLeft = newNumLeft;
			++countsLeftEnd;
		}
		if (numLeft < MinRecords || (numRecords - numLeft) < MinRecords)
			return;

		LeafNode* newNext = leafPool.construct();
		leaves += 1; // TODO: remove
		newNext->previous = leaf;
		if (lastLeaf == leaf)
			lastLeaf = newNext;
		if (leaf->next) {
			newNext->next = leaf->next;
			leaf->next->previous = newNext;
		}
		leaf->next = newNext;

		auto toNextBegin = ::begin(leaf->records) + numLeft;
		auto toNextEnd = ::end(leaf->records);
		newNext->records.moveInsert(::begin(newNext->records), toNextBegin, toNextEnd);
		leaf->records.erase(toNextBegin, toNextEnd);

		newNext->counts.moveInsert(::begin(newNext->counts), countsLeftEnd, countsEnd);
		leaf->counts.erase(countsLeftEnd, countsEnd);

		if (leaf->parent)
			innerInsert(leaf->parent, KeyType(newNext->counts[0]), NodePtr(newNext));
		else {
			InnerNode* parent = innerPool.construct();
			parent->keys.back() = KeyType(newNext->counts[0]);
			*(parent->children.end() - 1) = newNext;
			*(parent->children.end() - 2) = leaf;

			leaf->parent = parent;
			root = parent;
			++innerLevels;
		}
		newNext->parent = leaf->parent;
	}

	void updateKey(InnerNode* inner, KeyType key, KeyType newKey) {
		auto slot = findSlot<0, MaxKeys>(inner->keys, key);
		if (slot == 0 || !inner->children[slot - 1]) {
			if (inner->parent)
				updateKey(inner->parent, key, newKey);
		}
		else
			inner->keys[slot - 1] = newKey;
	}

	void updateParentKeys(LeafNode* leaf, KeyType oldLowKey) {
		assert(leaf->parent);
		KeyType key = KeyType(leaf->counts[0]);
		updateKey(leaf->parent, oldLowKey, key);
	}

	void insertBalance() {
		while (dirty) {
			LeafNode* current = dirty;
			dirty = current->nextDirty;
			current->nextDirty = nullptr;

			if (current->records.size() > MaxRecords) {
				size_t overage = current->records.size() - MaxRecords;
				size_t prevSize = current->previous ? current->previous->records.size() : MaxRecords;
				size_t nextSize = current->next ? current->next->records.size() : MaxRecords;
				size_t prevSpace = prevSize < MaxRecords ? MaxRecords - prevSize : 0;
				size_t nextSpace = nextSize < MaxRecords ? MaxRecords - nextSize : 0;

				size_t toPrev = 0;
				size_t toNext = 0;
				auto toPrevCountsEnd = ::begin(current->counts);
				auto toNextCountsBegin = ::end(current->counts);

				do {
					if (prevSize + toPrev < nextSize + toNext) {
						if (toPrev + PerKeyRecordCount(toPrevCountsEnd[0]) <= prevSpace) {
							toPrev += PerKeyRecordCount(toPrevCountsEnd[0]);
							++toPrevCountsEnd;
						}
						else {
							while (toPrev + toNext < overage) {
								if (toNext + PerKeyRecordCount(toNextCountsBegin[-1]) <= nextSpace) {
									toNext += PerKeyRecordCount(toNextCountsBegin[-1]);
									--toNextCountsBegin;
								}
								else
									goto SPLIT;
							}
						}
					}
					else {
						if (toNext + PerKeyRecordCount(toNextCountsBegin[-1]) <= nextSpace) {
							toNext += PerKeyRecordCount(toNextCountsBegin[-1]);
							--toNextCountsBegin;
						}
						else {
							while (toPrev + toNext < overage) {
								if (toPrev + PerKeyRecordCount(toPrevCountsEnd[0]) <= prevSpace) {
									toPrev += PerKeyRecordCount(toPrevCountsEnd[0]);
									++toPrevCountsEnd;
								}
								else
									goto SPLIT;
							}
						}
					}
				} while (toPrev + toNext < overage);

				if (toNext > 0) {
					Records& nextRecords = current->next->records;
					auto toNextEnd = ::end(current->records);
					auto toNextBegin = toNextEnd - toNext;
					nextRecords.moveInsert(::begin(nextRecords), toNextBegin, toNextEnd);
					current->records.erase(toNextBegin, toNextEnd);

					Counts& nextCounts = current->next->counts;
					KeyType nextLowKey = KeyType(nextCounts[0]);

					auto toNextCountsEnd = ::end(current->counts);
					nextCounts.moveInsert(::begin(nextCounts), toNextCountsBegin, toNextCountsEnd);
					current->counts.erase(toNextCountsBegin, toNextCountsEnd);

					updateParentKeys(current->next, nextLowKey);
				}
				if (toPrev > 0) {
					Records& prevRecords = current->previous->records;
					auto toPrevBegin = ::begin(current->records);
					auto toPrevEnd = toPrevBegin + toPrev;
					prevRecords.moveInsert(::end(prevRecords), toPrevBegin, toPrevEnd);
					current->records.erase(toPrevBegin, toPrevEnd);

					KeyType lowKey = KeyType(current->counts[0]);

					Counts& prevCounts = current->previous->counts;
					auto toPrevCountsBegin = ::begin(current->counts);
					prevCounts.moveInsert(::end(prevCounts), toPrevCountsBegin, toPrevCountsEnd);
					current->counts.erase(toPrevCountsBegin, toPrevCountsEnd);

					updateParentKeys(current, lowKey);
				}

				continue;
			SPLIT:
				splitLeaf(current);
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
				moveMerge(leaf, iter, end);
				break;
			}
			iter = moveMergeUntil(leaf, iter, end, path[innerLevels].upperBound);
		}
		insertBalance();
	}
};

template<class... Ts> inline auto begin(BTree<Ts...>& t) { return t.begin(); }
template<class... Ts> inline auto end(BTree<Ts...>& t) { return t.end(); }