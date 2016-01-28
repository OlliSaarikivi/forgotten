#pragma once

#include "Columnar.h"

template<class TIndex, class... TValues> class ThreeLevelBTree {
	using Index = TIndex;
	using Key = typename Index::Key;
	using GetKey = typename Index::GetKey;

	GetKey getKey = GetKey{};
	static const Key LeastKey = Index::LeastKey;

	static const size_t KeysInCacheLine = (CACHE_LINE_SIZE * 8) / sizeof(Key);
	static const size_t MaxKeys = KeysInCacheLine < 4 ? 4 : KeysInCacheLine;
	static const size_t MinKeys = MaxKeys / 2;

	static const size_t MaxRows = MaxKeys / 2;
	static const size_t MinRows = MaxRows / 2;

	struct LeafNode {
		LeafNode* next;
		size_t size;
		ColumnarArray<MaxRows, TValues...> rows;
		LeafNode* previous;

		LeafNode() : size(0), next(nullptr), previous(nullptr) {}

		bool isFull() {
			return size == MaxRows;
		}
	};
	struct InnerNode {
		size_t size;
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
		size_t innerSlot;
		LeafNode* leaf;
		Key leafUpperBound;
	};

	class Iterator
		: boost::equality_comparable<Iterator
		, boost::equality_comparable<Iterator, End >> {

		LeafNode* current;
		size_t pos;

	public:
		using RowType = Row<TValues&...>;

		Iterator() : current(nullptr), pos(0) {}
		Iterator(LeafNode* beginLeaf, size_t beginPos) : current(beginLeaf), pos(beginPos) {
			if (current && pos == current->size) {
				current = current->next;
				pos = 0;
			}
		}

		Iterator& operator++() {
			++pos;
			if (pos == current->size) {
				current = current->next;
				pos = 0;
			}
			return *this;
		}
		Iterator operator++(int) {
			Iterator old = *this;
			this->operator++();
			return old;
		}

		RowType operator*() const {
			return current->rows[pos];
		}
		FauxPointer<RowType> operator->() const {
			return FauxPointer<RowType>{ this->operator*() };
		}

		friend bool operator==(const Iterator& iter, const End& sentinel) {
			return iter.current == nullptr;
		}
		friend bool operator==(const Iterator& left, const Iterator& right) {
			return left.current == right.current && left.pos == right.pos;
		}

		friend void swap(Iterator& left, Iterator& right) {
			using std::swap;
			swap(left.current, right.current);
			swap(left.pos, right.pos);
		}
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
		//size_t i = 0;
		//for (;;) {
		//	if (i == rootKeys.size()) break;
		//	if (key < rootKeys[i]) break;
		//	++i;
		//}

		size_t lower = 0;
		size_t upper = rootKeys.size();
		while (upper - lower > 8) {
			auto middle = (upper + lower) / 2;
			bool isLess = key < rootKeys[middle];
			lower = isLess ? lower : middle + 1;
			upper = isLess ? middle : upper;
		}
		for (; lower < upper; ++lower)
			if (key < rootKeys[lower]) break;
		size_t i = lower;

		if (i == rootKeys.size())
			path.innerUpperBound = LeastKey;
		else
			path.innerUpperBound = rootKeys[i];
		path.rootSlot = i;
		path.inner = rootChildren[i];
	}

	void findLeaf(Path& path, Key key) {
		assert(path.inner);
		size_t i = 0;
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

	void innerInsert(Path& path, Key key, LeafNode* leaf, size_t pos) {
		InnerNode* inner = path.inner;

		if (inner->isFull()) {
			// Split
			InnerNode* newInner = constructInner();
			size_t numRight = (MaxKeys + 1) / 2;
			size_t numLeft = (MaxKeys + 1) - numRight;
			for (size_t i = 0; i < numRight; ++i) {
				newInner->children[i] = inner->children[numLeft + i];
			}
			for (size_t i = 0; i < numRight - 1; ++i) {
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
				path.inner = newInner;
				++(path.rootSlot);
				path.innerSlot -= numLeft;
				pos -= numLeft;
				target = newInner;
			}

			rootInsert(middleKey, newInner, insertAt);

			inner = target;
		}

		assert(pos != 0);
		for (size_t i = inner->size; i >= pos; --i) {
			inner->keys[i] = inner->keys[i - 1];
			inner->children[i + 1] = inner->children[i];
		}
		inner->keys[pos - 1] = key;
		inner->children[pos] = leaf;
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
		for (size_t i = 0; i < toNext; ++i) {
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
		bool operator()(Key key) { return true; }
	};
	struct InsertBound {
		Key upperBound;
		bool operator()(Key key) { return key < upperBound; }
	};
	template<class TIter, class TBound = NoInsertBound> TIter leafInsertUniqueSorted(LeafNode* leaf, TIter rangeBegin, TIter rangeEnd, TBound inBound = NoInsertBound{}) {
		size_t space = MaxRows - leaf->size;
		auto actualEnd = rangeBegin;
		size_t spaceLeft;
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
		auto out = rowsBegin + (MaxRows - 1 - spaceLeft);

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

		leaf->size = MaxRows - spaceLeft;

		return result;
	}

	void rootErase(size_t pos) {
		assert(pos != 0);
		rootKeys.erase(::begin(rootKeys) + (pos - 1));
		rootChildren.erase(::begin(rootChildren) + pos);
	}

	void innerErase(Path& path, size_t pos) {
		InnerNode* inner = path.inner;

		for (size_t i = pos; i < inner->size; ++i) {
			inner->keys[i - 1] = inner->keys[i];
			inner->children[i] = inner->children[i + 1];
		}
		--(inner->size);

		if (inner->size >= MinKeys || rootChildren.size() <= 1)
			return;

		auto slot = path.rootSlot;

		InnerNode* prev = nullptr;
		if (slot != 0 && (prev = rootChildren[slot - 1])->size > MinKeys) {
			for (size_t i = 0; i < inner->size; ++i) {
				inner->keys[inner->size - i] = inner->keys[inner->size - i - 1];
				inner->children[inner->size + 1 - i] = inner->children[inner->size - i];
			}
			inner->children[1] = inner->children[0];

			inner->children[0] = prev->children[prev->size];
			inner->keys[0] = rootKeys[slot - 1];
			rootKeys[slot - 1] = prev->keys[prev->size - 1];

			++(inner->size);
			--(prev->size);

			++(path.innerSlot);

			return;
		}

		InnerNode* next = nullptr;
		if (slot + 1 < rootChildren.size() && (next = rootChildren[slot + 1])->size > MinKeys) {
			++(inner->size);
			--(next->size);

			inner->children[inner->size] = next->children[0];
			inner->keys[inner->size - 1] = rootKeys[slot];
			rootKeys[slot] = next->keys[0];

			path.innerUpperBound = rootKeys[slot];

			next->children[0] = next->children[1];
			for (size_t i = 1; i <= next->size; ++i) {
				next->keys[i - 1] = next->keys[i];
				next->children[i] = next->children[i + 1];
			}
			return;
		}
			
		if (next) {
			inner->keys[inner->size] = rootKeys[slot];
			for (size_t i = 0; i < next->size; ++i) {
				inner->keys[inner->size + 1 + i] = next->keys[i];
				inner->children[inner->size + 1 + i] = next->children[i];
			}
			inner->children[inner->size + next->size + 1] = next->children[next->size];

			inner->size += next->size + 1;

			path.innerUpperBound = rootKeys[slot + 1];

			rootErase(slot + 1);

			destroyInner(next);
		}
		else {
			assert(prev);

			prev->keys[prev->size] = rootKeys[slot - 1];
			for (size_t i = 0; i < inner->size; ++i) {
				prev->keys[prev->size + 1 + i] = inner->keys[i];
				prev->children[prev->size + 1 + i] = inner->children[i];
			}
			prev->children[prev->size + inner->size + 1] = inner->children[inner->size];

			path.inner = prev;
			--(path.rootSlot);
			path.innerSlot += prev->size + 1;

			prev->size += inner->size + 1;

			rootErase(slot);

			destroyInner(inner);
		}
	}

	LeafNode* prevSiblingLeaf(InnerNode* inner, size_t slot) {
		return slot != 0 ? inner->children[slot - 1] : nullptr;
	}
	LeafNode* nextSiblingLeaf(InnerNode* inner, size_t slot) {
		return slot != inner->size ? inner->children[slot + 1] : nullptr;
	}
	void balanceLeaf(Path& path) {
		LeafNode* leaf = path.leaf;
		if (leaf->size >= MinRows || &firstLeaf == lastLeaf)
			return;

		InnerNode* parent = path.inner;
		size_t slot = path.innerSlot;
		LeafNode* prev = prevSiblingLeaf(parent, slot);
		LeafNode* next = nextSiblingLeaf(parent, slot);

		size_t fromPrev = 0;
		size_t fromNext = 0;
		if (prev) {
			if (next) {
				auto targetSize = (prev->size + leaf->size) / 2;
				if (targetSize >= MinRows) {
					fromPrev = targetSize - leaf->size;
				}
				else {
					targetSize = (next->size + leaf->size) / 2;
					if (targetSize >= MinRows) {
						fromNext = targetSize - leaf->size;
					}
					else if ((MinRows - prev->size) + (MinRows - next->size) + leaf->size > MinRows) {
						fromPrev = (MinRows - prev->size);
						targetSize = (next->size + (leaf->size + fromPrev)) / 2;
						fromNext = targetSize - (leaf->size + fromPrev);
					}
					else
						goto MERGE_WITH_NEXT;
				}
			}
			else {
				auto targetSize = (prev->size + leaf->size) / 2;
				if (targetSize >= MinRows) {
					fromPrev = targetSize - leaf->size;
				}
				else
					goto MERGE_WITH_PREV;
			}
		}
		else {
			if (next) {
				auto targetSize = (next->size + leaf->size) / 2;
				if (targetSize >= MinRows) {
					fromNext = targetSize - leaf->size;
				}
				else
					goto MERGE_WITH_NEXT;
			}
			else
				assert(false); // Should have neighbors
		}
		assert(leaf->size + fromPrev + fromNext >= MinRows);

		if (fromPrev > 0) {
			for (size_t i = 1; i <= leaf->size; ++i) {
				leaf->rows[leaf->size - i + fromPrev] |= leaf->rows[leaf->size - i];
			}
			for (int_fast8_t i = 0; i < fromPrev; ++i) {
				leaf->rows[i] |= prev->rows[prev->size - fromPrev + i];
			}
			prev->size -= fromPrev;
			leaf->size += fromPrev;

			parent->keys[slot - 1] = getKey(leaf->rows[0]);
		}
		if (fromNext > 0) {
			for (int_fast8_t i = 0; i < fromNext; ++i) {
				leaf->rows[leaf->size + i] |= next->rows[i];
			}
			next->size -= fromNext;
			for (int_fast8_t i = 0; i < next->size; ++i) {
				next->rows[i] |= next->rows[i + fromNext];
			}
			leaf->size += fromNext;

			Key newUpperBound = getKey(next->rows[0]);
			path.leafUpperBound = newUpperBound;
			parent->keys[slot] = newUpperBound;
		}
		return;

	MERGE_WITH_NEXT:
		for (size_t i = 0; i < next->size; ++i) {
			leaf->rows[leaf->size + i] |= next->rows[i];
		}
		leaf->size += next->size;
		leaf->next = next->next;
		if (leaf->next) {
			leaf->next->previous = leaf;
			path.leafUpperBound = getKey(leaf->next->rows[0]);
		}
		else
			path.leafUpperBound = LeastKey;
		if (lastLeaf == next)
			lastLeaf = leaf;

		innerErase(path, path.innerSlot + 1);

		destroyLeaf(next);
		return;

	MERGE_WITH_PREV:
		for (size_t i = 0; i < leaf->size; ++i) {
			prev->rows[prev->size + i] |= leaf->rows[i];
		}
		prev->size += leaf->size;
		prev->next = leaf->next;
		if (prev->next)
			prev->next->previous = prev;
		path.leaf = prev;
		--(path.innerSlot);
		if (lastLeaf == leaf)
			lastLeaf = prev;

		innerErase(path, path.innerSlot + 1);

		destroyLeaf(leaf);
		return;
	}

	template<class TIter> TIter leafEraseSorted(LeafNode* leaf, TIter rangeBegin, TIter rangeEnd) {
		TIter rangeIter = rangeBegin;
		size_t out = 0;
		size_t i = 0;
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
	ThreeLevelBTree() : innerPool(sizeof(InnerNode)), leafPool(sizeof(LeafNode)), lastLeaf(&firstLeaf) {
		rootChildren.push_back(&firstInner);
		firstInner.children[0] = &firstLeaf;
	}

	auto begin() {
		return Iterator(&firstLeaf, 0);
	}

	auto end() {
		return Iterator();
	}

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

	void printStats() {
		std::cout << "Root size: " << rootChildren.size() << "\n";
	}
};

template<class... Ts> inline auto begin(ThreeLevelBTree<Ts...>& t) { return t.begin(); }
template<class... Ts> inline auto end(ThreeLevelBTree<Ts...>& t) { return t.end(); }