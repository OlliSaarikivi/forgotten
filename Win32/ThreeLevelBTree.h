#pragma once

#include "Columnar.h"

template<class TIndex, class... TValues> class ThreeLevelBTree {
	using Index = TIndex;
	using Key = Index::Key;
	
	const auto getKey = Index::GetKey{};
	static const Key LeastKey = Index::LeastKey;

	static const size_t KeysInCacheLine = ((CACHE_LINE_SIZE * 3) / 2) / sizeof(Key);
	static const size_t MaxKeys = KeysInCacheLine < 4 ? 4 : KeysInCacheLine;
	static const size_t MinKeys = MaxKeys / 2;

	static const size_t MaxRows = 24;
	static const size_t MinRows = 12;

	struct LeafNode {
		LeafNode* next;
		uint8_t size;
		ColumnarArray<MaxRows, TValues...> rows;
		LeafNode* previous;

		LeafNode() : size(0), next(nullptr), previous(nullptr) {}

		bool isFull() {
			return size == MaxRows;
		}
	};
	struct InnerNode {
		uint8_t size;
		array<Key, MaxKeys> keys;
		array<LeafNode*, MaxKeys + 1> children;

		InnerNode() : size(0) {}

		bool isFull() {
			return size == MaxKeys;
		}
	};

	struct Path {
		size_t rootSlot;
		InnerNode* inner;
		Key innerUpperBound;
		uint_fast8_t innerSlot;
		LeafNode* leaf;
		Key leafUpperBound;
	};

	boost::pool<> leafPool;
	boost::pool<> innerPool;
	vector<Key> rootKeys;
	vector<InnerNode*> rootChildren;
	InnerNode firstInner;
	LeafNode firstLeaf;
	LeafNode* lastLeaf;

	InnerNode* constructInner() {
		auto inner = (InnerNode*)innerPool.malloc();
		new (inner) InnerNode();
		return inner;
	}
	void destroyInner(InnerNode* node) {
		innerPool.free(node);
	}

	LeafNode* constructLeaf() {
		auto leaf = (LeafNode*)leafPool.malloc();
		new (leaf) LeafNode();
		return leaf;
	}
	void destroyLeaf(LeafNode* node) {
		leafPool.free(node);
	}

	void findInner(Path& path, Key key) {
		// Linear search
		size_t i = 0;
		for (;;) {
			if (i == rootKeys.size()) {
				path.innerUpperBound = LeastKey;
				break;
			}
			Key rootKey = rootKeys[i];
			if (key < rootKey) {
				path.innerUpperBound = rootKey;
				break;
			}
		}
		path.rootSlot = i;
		path.inner = rootChildren[i];
	}

	void findLeaf(Path& path, Key key) {
		assert(path.inner);
		uint_fast8_t i = 0;
		for (; i < path.inner->size; ++i) {
			if (key < path.inner->keys[i]) {
				path.leafUpperBound = path.inner->keys[i];
				goto FOUND;
			}
		}
		path.leafUpperBound = path.innerUpperBound;
	FOUND:
		path.innerSlot = i;
		path.leaf = path.inner->children[i];
	}

	void rootInsert(Key key, InnerNode* inner, size_t pos) {
		assert(pos != 0);
		rootKeys.insert(::begin(rootKeys) + (pos - 1), key);
		rootChildren.insert(::begin(rootChildren) + pos, inner);
	}

	void innerInsert(Path& path, Key key, LeafNode* leaf, uint_fast_t pos) {
		InnerNode* inner = path.inner;

		if (inner->isFull()) {
			// Split
			InnerNode* newInner = constructInner();
			uint_fast8_t numRight = (MaxKeys + 1) / 2;
			uint_fast8_t numLeft = (MaxKeys + 1) - numRight;
			for (uint_fast8_t i = 0; i < numRight; ++i) {
				newInner->children[i] = inner->children[numLeft + i];
			}
			for (uint_fast8_t i = 0; i < numRight - 1; ++i) {
				newInner->keys[i] = inner->keys[numLeft + i];
			}
			inner->size = numLeft - 1;
			newInner->size = numRight - 1;

			auto insertAt = path.rootSlot + 1;

			Key middleKey = inner->keys[numLeft - 1];
			InnerNode* target = inner;
			if (key < middleKey)
				path.innerUpperBound = middleKey;
			else {
				path.inner = newSibling;
				++(path.rootSlot);
				path.innerSlot -= numLeft;
				pos -= numLeft;
				target = newSibling;
			}

			rootInsert(middleKey, newSibling, insertAt);

			inner = target;
		}

		assert(pos != 0);
		for (uint_fast8_t i = inner->size; i >= pos; --i) {
			inner->keys[i] = inner->keys[i - 1];
			inner->children[i + 1] = inner->children[i];
		}
		inner->keys[pos - 1] = key;
		inner->children[pos] = node;
		++(inner->size);
	}

	void splitLeaf(Path& path, Key key) {
		LeafNode* leaf = path.leaf;

		LeafNode* newLeaf = constructLeaf();
		newLeaf->previous = leaf;
		if (leaf->next) {
			newLeaf->next = leaf->next;
			leaf->next->previous = newLeaf;
		}
		else
			lastLeaf = newLeaf;
		leaf->next = newLeaf;

		auto toNext = (MaxRows + 1) / 2;
		for (uint_fast8_t i = 0; i < toNext; ++i) {
			newLeaf->rows[i] |= leaf->rows[i + (MaxRows - toNext)];
		}
		leaf->size = MaxRows - toNext;
		newLeaf->size = toNext;

		auto insertAt = path.innerSlot + 1;

		Key splitKey = getKey(newLeaf->rows[0]);
		if (key < splitKey)
			path.leafUpperBound = splitKey;
		else {
			path.leaf = newLeaf;
			++(path.innerSlot);
		}

		innerInsert(path, splitKey, newLeaf, insertAt);
	}

	struct NoInsertBound {
		bool operator()(KeyType key) { return true; }
	};
	struct InsertBound {
		KeyType upperBound;
		bool operator()(KeyType key) { return KeyLess()(key, upperBound); }
	};
	template<class TIter, class TBound = NoInsertBound> TIter leafInsertUniqueSorted(LeafNode* leaf, TIter rangeBegin, TIter rangeEnd, TBound inBound = NoInsertBound{}) {
		uint_fast8_t space = MaxRows - leaf->size;

		auto actualEnd = rangeBegin;
		auto spaceLeft;

		auto rangeSize = rangeEnd - rangeBegin;
		if (rangeSize > space) {
			actualEnd += space;
			spaceLeft = 0;
		}
		else {
			actualEnd = rangeEnd;
			spaceLeft = space - rangeSize;
		}
		while (actualEnd != rangeBegin && !inBound(getKey(*(actualEnd - 1)))) {
			--actualEnd;
			++spaceLeft;
		}

		auto rowsBegin = ::begin(leaf->rows);
		auto rowsEnd = rowsBegin + leaf->size;
		auto out = rowsBegin + (MaxRows - 1 - space);

		// Remember the end
		auto result = actualEnd;

		if (rowsEnd == rowsBegin) {
			while (actualEnd != rangeBegin)
				*out-- |= *(--actualEnd);
			goto INSERT_DONE;
		}
		Key rowsKey = getKey(*(--rowsEnd));

		if (actualEnd != rangeBegin) {
			Key rangeKey = getKey(*(--actualEnd));
			for (;;) {
				if (rowsKey < rangeKey) {
					*out-- |= *actualEnd;
					if (actualEnd == rangeBegin) goto INSERT_DONE;
					rangeKey = getKey(*(--actualEnd));
				}
				else {
					assert(rangeKey < rowsKey);
					*out-- |= *rowsEnd;
					if (rowsEnd == rowsBegin) {
						while (actualEnd != rangeBegin)
							*out-- |= *(--actualEnd);
						goto INSERT_DONE;
					}
					rowsKey = getKey(*(--rowsEnd));
				}
			}
		}
	INSERT_DONE:

		leaf->size = MaxRows - space;

		return result;
	}



	LeafNode* prevSiblingLeaf(InnerNode* inner, uint_fast8_t slot) {
		return slot != 0 ? inner->children[slot - 1] : nullptr;
	}
	LeafNode* nextSiblingLeaf(InnerNode* inner, uint_fast8_t slot) {
		return slot != inner->size ? inner->children[slot + 1] : nullptr;
	}
	void balanceLeaf(Path& path) {
		LeafNode* leaf = path.leaf;
		uint_fast8_t needed = leaf->size < MinRows ? MinRows - leaf->size : 0;
		if (needed > 0 && firstLeaf != lastLeaf) {
			InnerNode* parent = path.inner;
			uint_fast8_t slot = path.innerSlot;
			LeafNode* prev = prevSiblingLeaf(parent, slot);
			LeafNode* next = nextSiblingLeaf(parent, slot);
			uint_fast8_t prevAvailable = prev && prev->size > MinRows ? prev->size - MinRows : 0;
			uint_fast8_t nextAvailable = next && next->size > MinRows ? next->size - MinRows : 0;
			//TODO
			if (prevAvailable + nextAvailable >= needed) {
				auto fromPrev = std::min(needed, prevAvailable);
				if (fromPrev > 0) {
					for (uint_fast8_t i = 1; i <= leaf->size; ++i) {
						leaf->rows[leaf->size - i + fromPrev] |= leaf->rows[leaf->size - i];
					}
					for (int_fast8_t i = 0; i < fromPrev; ++i) {
						leaf->rows[i] |= prev->rows[prev->size - fromPrev + i];
					}
					prev->size -= fromPrev;
					leaf->size += fromPrev;
					needed -= fromPrev;

					parent->keys[slot - 1] = getKey(leaf->rows[0]);
				}
				auto fromNext = std::min(needed, nextAvailable);
				if (fromNext > 0) {
					for (int_fast8_t i = 0; i < fromNext; ++i) {
						leaf->rows[leaf->size + i] |= next->rows[i];
					}
					next->size -= fromNext;
					for (int_fast8_t i = 0; i < next->size; ++i) {
						next->rows[i] |= next->rows[i + fromNext];
					}
					leaf->size += fromNext;

					KeyType newUpperBound = getKey(next->rows[0]);
					path.leafUpperBound = newUpperBound;
					parent->keys[slot] = newUpperBound;
				}
			}
			else {
				// Merge
				if (next) {
					assert(leaf != next);
					for (uint_fast8_t i = 0; i < next->size; ++i) {
						leaf->rows[leaf->size + i] |= next->rows[i];
					}
					leaf->size += next->size;
					leaf->next = next->next;
					if (leaf->next) {
						leaf->next->previous = leaf;
						path[0].upperBound = GetKey()(leaf->next->rows[0]);
					}
					else
						path[0].upperBound = LeastKey;
					if (lastLeaf == next)
						lastLeaf = leaf;

					innerErase(path, 1, path[0].slot + 1);

					destroyLeaf(next);
					removeRedundantRoot();
				}
				else if (prev) {
					for (uint_fast8_t i = 0; i < leaf->size; ++i) {
						prev->rows[prev->size + i] |= leaf->rows[i];
					}
					prev->size += leaf->size;
					prev->next = leaf->next;
					if (prev->next)
						prev->next->previous = prev;
					path[0].node = prev;
					--(path[0].slot);
					if (lastLeaf == leaf)
						lastLeaf = prev;

					innerErase(path, 1, path[0].slot + 1);

					destroyLeaf(leaf);
					removeRedundantRoot();
				}
				else
					assert(false);
			}
		}
	}

	template<class TIter> TIter leafEraseSorted(LeafNode* leaf, TIter rangeBegin, TIter rangeEnd) {
		TIter rangeIter = rangeBegin;
		uint_fast8_t out = 0;
		uint_fast8_t i = 0;
		if (rangeIter == rangeEnd) goto ALL_DONE;
		Key rangeKey = getKey(*rangeIter);
		for (; i < leaf->size; ++i) {
			Key key = getKey(leaf->rows[i]);
			for (;;) {
				if (rangeKey >= key) {
					if (key < rangeKey) {
						if (out != i) leaf->rows[out] |= leaf->rows[i];
						++out;
					}
					break;
				}
				++rangeIter;
				if (rangeIter == rangeEnd) goto MATCH_DONE;
				rangeKey = getKey(*rangeIter);
			}
		}
		goto ALL_DONE;
	MATCH_DONE:
		if (out != i)
			for (; i < leaf->size; ++i) {
				leaf->rows[out] |= leaf->rows[i];
				++out;
			}
	ALL_DONE:
		leaf->size -= i - out;
		return rangeIter;
	}

public:
	ThreeLevelBTree() {}

	template<class TRow> void insert(const TRow& row) {
		Key key = getKey(row);
		Path path;
		findInner(path, key);
		findLeaf(path, key);

		if (path.leaf->isFull())
			splitLeaf(path, key);

		leafInsertUniqueSorted(path.leaf, &row, &row + 1);
	}

	template<class TRow> void erase(const TRow& row) {
		Key key = getKey(row);
		Path path;
		findInner(path, key);
		findLeaf(path, key);

		leafEraseSorted(path.leaf, &row, &row + 1);
		balanceLeaf(path);
	}
};
