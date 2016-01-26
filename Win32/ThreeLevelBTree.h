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
			}
		}
		path.leafUpperBound = path.innerUpperBound;
	SET_PATH:
		path.innerSlot = i;
		path.leaf = path.inner->children[i];
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

		TIter actualEnd = rangeBegin;
		for (;;) {
			if (space == 0 | (actualEnd == rangeEnd || !inBound(getKey(*actualEnd))))
				break;
			++actualEnd;
			--space;
		}

		auto& rows = leaf->rows;
		auto rowsBegin = ::begin(rows);
		auto rowsEnd = rowsBegin + leaf->size;
		Key rowsKey = getKey(*rowsBack);
		auto out = rowsBegin + (MaxRows - 1 - space);

		//if (rowsBack < rowsBegin) {
		//	while (rangeBack >= rangeBegin)
		//		*out-- |= *rangeBack--;
		//	goto INSERT_DONE;
		//}

		if (actualEnd != rangeBegin) {
			auto rangeBack = actualEnd - 1;
			Key rangeKey = getKey(*rangeBack);
			for (;;) {
				if (rowsKey < rangeKey) {
					*out-- |= *rangeBack;
					actualEnd = rangeBack;
					if (actualEnd == rangeBegin) goto INSERT_DONE;
					rangeBack = actualEnd - 1;
					rangeKey = getKey(*rangeBack);
				}
				else if (rangeKey < rowsKey) {
					*out-- |= *rowsBack--;
					if (rowsBack < rowsBegin) {
						while (rangeBack >= rangeBegin)
							*out-- |= *rangeBack--;
						goto INSERT_DONE;
					}
					rowsKey = GetKey()(*rowsBack);
				}
				else {
					do {
						*out |= *rangeBack--;
						if (rangeBack < rangeBegin) goto INSERT_DONE;
						rangeKey = GetKey()(*rangeBack);
					} while (!KeyLess()(rangeKey, rowsKey));
					--out;
					--rowsBack;
					rowsKey = GetKey()(*rowsBack);
				}
			}
		}
	INSERT_DONE:

		leaf->size = MaxRows - space;

		return actualEnd;
	}

public:
	ThreeLevelBTree() {}

	template<class TRow> void insert(const TRow& row) {
		Key key = getKey(row);
		Path path;
		findInner(path, key);
		findLeaf(path, key);

		if (path.leaf->isFull())
			split(path, key);

		leafInsertSorted(path.leaf, &row, &row + 1);
	}
};
