#pragma once

#include "Columnar.h"
#include "Sentinels.h"

template<class TIndex, class... TValues> struct MBPlusTree {
	using Index = TIndex;

private:
	using Key = typename Index::Key;
	using GetKey = Index;

	static const size_t KeysInCacheLine = (CACHE_LINE_SIZE * 8) / sizeof(Key);
	static const size_t MaxKeys = KeysInCacheLine < 4 ? 4 : KeysInCacheLine;
	static const size_t MinKeys = MaxKeys / 2;

	static const size_t MaxRows = MaxKeys;
	static const size_t MinRows = MaxRows / 3;

	static const size_t InsertionSortMax = 24;
	static const size_t LinearRowSearchMax = 24;

	struct LeafNode {
		LeafNode* next;
		size_t size;
		bool isUnsorted;
		ColumnarArray<MaxRows, TValues...> rows;
		LeafNode* previous;

		LeafNode() : size(0), next(nullptr), previous(nullptr), isUnsorted(false)
		{}

		bool isFull() {
			return size == MaxRows;
		}

		Key greatestKey() {
			assert(size > 0);
			Key max = GetKey()(rows[0]);
			for (size_t i = 1; i < size; ++i) {
				max = std::max(max, GetKey()(rows[i]));
			}
			return max;
		}

		void insertionSort(size_t left, size_t right) {
			for (size_t i = left + 1; i <= right; ++i) {
				auto value = rows[i].temp();
				auto key = GetKey()(value);
				int j = int(i) - 1;
				while (j >= 0 && GetKey()(rows[j]) > key) {
					rows[j + 1] |= rows[j];
					--j;
				}
				rows[j + 1] |= value;
			}
		}

		size_t choosePivot(size_t left, size_t right) {
			size_t middle = (left + right) / 2;

			if (GetKey()(rows[left]) > GetKey()(rows[middle]))
				swap(rows[left], rows[middle]);

			if (GetKey()(rows[left]) > GetKey()(rows[right]))
				swap(rows[left], rows[right]);

			if (GetKey()(rows[middle]) > GetKey()(rows[right]))
				swap(rows[middle], rows[right]);

			return middle;
		}

		size_t choosePivotForSort(size_t left, size_t right) {
			size_t middle = (left + right) / 2;

			if (GetKey()(rows[left]) > GetKey()(rows[middle]))
				swap(rows[left], rows[middle]);

			if (GetKey()(rows[left]) > GetKey()(rows[right]))
				swap(rows[left], rows[right]);

			if (GetKey()(rows[middle]) > GetKey()(rows[right]))
				swap(rows[middle], rows[right]);

			swap(rows[middle], rows[right - 1]);
			return right - 1;
		}

		size_t quickPartition(int left, int right, size_t pivotIndex) {
			auto pivotKey = GetKey()(rows[pivotIndex]);
			swap(rows[pivotIndex], rows[right]);
			auto storeIndex = left;
			for (auto i = left; i < right; ++i) {
				if (GetKey()(rows[i]) < pivotKey) {
					swap(rows[storeIndex], rows[i]);
					++storeIndex;
				}
			}
			swap(rows[right], rows[storeIndex]);
			return storeIndex;
		}

		size_t quickPartitionHoare(int left, int right, size_t pivotIndex) {
			auto pivotKey = GetKey()(rows[pivotIndex]);
			auto i = left - 1;
			auto j = right + 1;
			for (;;) {
				do {
					--j;
				} while (GetKey()(rows[j]) > pivotKey);
				do {
					++i;
				} while (GetKey()(rows[i]) < pivotKey);
				if (i < j)
					swap(rows[i], rows[j]);
				else
					return j;
			}
		}

		void quickSort(int left, int right) {
			if (right - left <= InsertionSortMax) {
				insertionSort(left, right);
				return;
			}
			auto pivotIndex = choosePivotForSort(left, right);
			pivotIndex = quickPartitionHoare(left, right, pivotIndex);
			quickSort(left, int(pivotIndex));
			quickSort(int(pivotIndex) + 1, right);
		}

		void sort() {
			if (isUnsorted && size > 0) {
				quickSort(0, int(size - 1));
				isUnsorted = false;
			}
		}

		size_t quickPrepare(int left, int right, size_t minLeft, size_t minRight) {
			if (left >= right)
				return left;
			auto pivotIndex = choosePivot(left, right);
			pivotIndex = quickPartition(left, right, pivotIndex);
			if (pivotIndex >= minLeft) {
				if (size - pivotIndex >= minRight)
					return pivotIndex;
				else
					return quickPrepare(left, pivotIndex - 1, minLeft, minRight);
			}
			else
				return quickPrepare(pivotIndex + 1, right, minLeft, minRight);
		}

		size_t prepareBalance(size_t minLeft, size_t minRight) {
			return quickPrepare(0, size - 1, minLeft, minRight);
		}

		int findRow(Key key) {
			size_t lower = 0;
			size_t upper = size;
			if (!isUnsorted)
				while (upper - lower > LinearRowSearchMax) {
					size_t middle = (lower + upper) / 2; // This won't overflow
					bool isLess = key < GetKey()(rows[middle]);
					lower = isLess ? lower : middle;
					upper = isLess ? middle : upper;
				}
			for (; lower < upper; ++lower)
				if (GetKey()(rows[lower]) == key)
					return lower;
			return -1;
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

	class Iterator
		: boost::equality_comparable<Iterator
		, boost::equality_comparable<Iterator, End >> {

		LeafNode* current;
		size_t pos;

	public:
		using reference = Row<TValues&...>;

		Iterator() : current(nullptr), pos(0) {}
		Iterator(LeafNode* beginLeaf, size_t beginPos) : current(beginLeaf), pos(beginPos) {
			if (current && pos == current->size) {
				current = current->next;
				pos = 0;
			}
			if (current)
				current->sort();
		}

		Iterator& operator++() {
			++pos;
			if (pos == current->size) {
				current = current->next;
				if (current)
					current->sort();
				pos = 0;
			}
			return *this;
		}
		Iterator operator++(int) {
			Iterator old = *this;
			this->operator++();
			return old;
		}

		reference operator*() const {
			return current->rows[pos];
		}
		FauxPointer<reference> operator->() const {
			return FauxPointer<reference>{ this->operator*() };
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

	struct Path {
		using iterator = Iterator;

		MBPlusTree& tree;

		size_t rootSlot;
		InnerNode* inner;
		Key innerUpperBound;
		size_t innerSlot;
		LeafNode* leaf;
		Key leafUpperBound;
		int leafPos;
		Key rowKey;

		Path(MBPlusTree& tree) : tree{ tree }, innerUpperBound{ MinKey<Key>()() } {}

		void update(Key key) {
			updateLeaf(key);
			leafPos = leaf->findRow(key);
		}

		void updateLeaf(Key key) {
			if (key >= innerUpperBound || key < rowKey)
				goto FROM_ROOT;
			if (key >= leafUpperBound)
				goto FROM_INNER;
			goto FROM_LEAF;
		FROM_ROOT:
			tree.findInner(*this, key);
		FROM_INNER:
			tree.findLeaf(*this, key);
		FROM_LEAF:
			rowKey = key;
		}

		iterator find(Key key) {
			update(key);
			if (leafPos >= 0)
				return iterator(leaf, leafPos);
			else
				return iterator();
		}
	};

	boost::pool<> leafPool;
	boost::pool<> innerPool;
	std::vector<Key> rootKeys;
	std::vector<InnerNode*> rootChildren;
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
		//leafPool.free(node);
	}

	void findInner(Path& path, Key key) {
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
			path.innerUpperBound = MinKey<Key>()();
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
		rootKeys.insert(rootKeys.begin() + (pos - 1), key);
		rootChildren.insert(rootChildren.begin() + pos, inner);
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
		size_t toNextBegin = leaf->prepareBalance(MinRows, MinRows);

		LeafNode* newLeaf = constructLeaf();
		newLeaf->previous = leaf;
		if (leaf->next) {
			newLeaf->next = leaf->next;
			leaf->next->previous = newLeaf;
		}
		else
			lastLeaf = newLeaf;
		leaf->next = newLeaf;
		newLeaf->isUnsorted = leaf->isUnsorted;

		auto toNext = leaf->size - toNextBegin;
		for (size_t i = 0; i < toNext; ++i) {
			newLeaf->rows[i] |= leaf->rows[toNextBegin + i];
		}
		leaf->size = MaxRows - toNext;
		newLeaf->size = toNext;

		auto insertAt = path.innerSlot + 1;

		Key splitKey = GetKey()(newLeaf->rows[0]);
		if (key < splitKey)
			path.leafUpperBound = splitKey;
		else {
			path.leaf = newLeaf;
			++(path.innerSlot);
		}

		innerInsert(path, splitKey, newLeaf, insertAt);
	}

	void unsafeSplitLeafUnbalanced(Path& path, Key key) {
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

		path.leaf = newLeaf;
		++(path.innerSlot);

		innerInsert(path, key, newLeaf, path.innerSlot);
	}

	void splitLeafUnbalanced(Path& path, Key key) {
		Key maxKey = path.leaf->greatestKey();

		if (key <= maxKey) {
			splitLeaf(path, key);
			return;
		}

		unsafeSplitLeafUnbalanced(path, key);
	}

	template<class TRow> void leafInsert(LeafNode* leaf, const TRow& row) {
		leaf->rows[leaf->size] |= row;
		++(leaf->size);
		leaf->isUnsorted = true;
	}

	void rootErase(size_t pos) {
		assert(pos != 0);
		rootKeys.erase(rootKeys.begin() + (pos - 1));
		rootChildren.erase(rootChildren.begin() + pos);
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

			if ((slot + 1) < rootKeys.size())
				path.innerUpperBound = rootKeys[slot + 1];
			else
				path.innerUpperBound = MinKey<Key>()();

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
				auto prevAvailable = prev->size - MinRows;
				if (prevAvailable + leaf->size >= MinRows) {
					fromPrev = prev->size - prev->prepareBalance(MinRows, MinRows - leaf->size);
				}
				else {
					auto nextAvailable = next->size - MinRows;
					if (nextAvailable + leaf->size >= MinRows) {
						fromNext = next->prepareBalance(MinRows - leaf->size, MinRows);
					}
					else {
						if (prevAvailable + nextAvailable + leaf->size >= MinRows) {
							if (prevAvailable > 0) {
								auto prevRequired = nextAvailable + leaf->size < MinRows ?
									MinRows - (nextAvailable + leaf->size) :
									0;
								fromPrev = prev->size - prev->prepareBalance(MinRows, prevRequired);
							}
							if (fromPrev + leaf->size < MinRows)
								fromNext = next->prepareBalance(MinRows - (fromPrev + leaf->size), MinRows);
						}
						else
							goto MERGE_WITH_PREV;
					}
				}
			}
			else {
				auto prevAvailable = prev->size - MinRows;
				if (prevAvailable + leaf->size >= MinRows) {
					fromPrev = prev->size - prev->prepareBalance(MinRows, MinRows - leaf->size);
				}
				else
					goto MERGE_WITH_PREV;
			}
		}
		else {
			if (next) {
				auto nextAvailable = next->size - MinRows;
				if (nextAvailable + leaf->size >= MinRows) {
					fromNext = next->prepareBalance(MinRows - leaf->size, MinRows);
				}
				else
					goto MERGE_WITH_NEXT;
			}
			else
				assert(false); // Should have neighbors
		}
		assert(leaf->size + fromPrev + fromNext >= MinRows);

		if (fromPrev > 0) {
			for (uint_fast8_t i = 0; i < fromPrev; ++i) {
				leaf->rows[leaf->size + i] |= prev->rows[prev->size - fromPrev + i];
			}
			auto oldEnd = leaf->size;
			prev->size -= fromPrev;
			leaf->size += fromPrev;

			leaf->isUnsorted = true;

			parent->keys[slot - 1] = GetKey()(leaf->rows[oldEnd]);
		}
		if (fromNext > 0) {
			for (uint_fast8_t i = 0; i < fromNext; ++i) {
				leaf->rows[leaf->size + i] |= next->rows[i];
			}
			next->size -= fromNext;
			leaf->size += fromNext;
			for (uint_fast8_t i = 0; i < next->size; ++i) {
				next->rows[i] |= next->rows[i + fromNext];
			}

			leaf->isUnsorted |= next->isUnsorted;

			Key newUpperBound = GetKey()(next->rows[0]);
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
		if (leaf->next)
			leaf->next->previous = leaf;

		if ((slot + 1) < parent->size)
			path.leafUpperBound = parent->keys[slot + 1];
		else
			path.leafUpperBound = path.innerUpperBound;

		if (lastLeaf == next)
			lastLeaf = leaf;

		next->isUnsorted = true;

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

		prev->isUnsorted |= leaf->isUnsorted;

		innerErase(path, path.innerSlot + 1);

		destroyLeaf(leaf);
		return;
	}

	void leafErase(Path& path) {
		if (path.leafPos >= 0) {
			path.leaf->rows[path.leafPos] |= path.leaf->rows[path.leaf->size - 1];
			--(path.leaf->size);
			path.leaf->isUnsorted = true;
		}
	}

	void assertBounds() {
		Key rootLower = MinKey<Key>()();
		Key rootUpper = MinKey<Key>()();
		for (int i = 0; i < rootKeys.size(); ++i) {
			rootUpper = rootKeys[i];
			assertInnerBounds(rootChildren[i], rootLower, rootUpper);
			rootLower = rootUpper;
		}
		assertInnerBounds(rootChildren[rootKeys.size()], rootLower, MinKey<Key>()());
	}

	void assertInnerBounds(InnerNode* inner, Key lower, Key upper) {
		Key innerLower = lower;
		Key innerUpper = upper;
		for (int i = 0; i < inner->size; ++i) {
			innerUpper = inner->keys[i];
			assertLeafBounds(inner->children[i], innerLower, innerUpper);
			innerLower = innerUpper;
		}
		assertLeafBounds(inner->children[inner->size], innerLower, MinKey<Key>()());
	}

	void assertLeafBounds(LeafNode* leaf, Key lower, Key upper) {
		for (int i = 0; i < leaf->size; ++i) {
			assert(GetKey()(leaf->rows[i]) >= lower);
			assert(upper == MinKey<Key>()() || GetKey()(leaf->rows[i]) < upper);
		}
	}

public:
	MBPlusTree() : innerPool(sizeof(InnerNode)), leafPool(sizeof(LeafNode)), lastLeaf(&firstLeaf) {
		rootChildren.push_back(&firstInner);
		firstInner.children[0] = &firstLeaf;
	}

	MBPlusTree(const MBPlusTree&) = delete;
	MBPlusTree& operator=(const MBPlusTree&) = delete;

	auto begin() {
		return Iterator(&firstLeaf, 0);
	}

	auto end() {
		return Iterator();
	}

	auto finder() {
		return Path{ *this };
	}

	auto finderFail() {
		return End{};
	}

	template<class TRow> void insert(const TRow& row) {
		Path path{ *this };
		Key key = GetKey()(row);
		path.updateLeaf(key);
		if (path.leaf->isFull())
			splitLeaf(path, key);
		leafInsert(path.leaf, row);
	}

	template<class TIter, class TSentinel> void insert(TIter rangeBegin, TSentinel rangeEnd) {
		if (rangeBegin == rangeEnd) return;
		TIter iter = rangeBegin;
		Path path{ *this };
		Key key = GetKey()(*iter);
		for (;;) {
			path.updateLeaf(key);
			if (path.leaf->isFull())
				splitLeafUnbalanced(path, key);
			leafInsert(path.leaf, *iter);
			++iter;
			if (iter == rangeEnd) {
				balanceLeaf(path);
				return;
			}
			key = GetKey()(*iter);
			if (path.leafUpperBound != MinKey<Key>()() && key >= path.leafUpperBound)
				balanceLeaf(path);
		}
	}

	template<class TRow> void append(const TRow& row) {
		Path path{ *this };
		path.rootSlot = rootChildren.size() - 1;
		path.inner = rootChildren.back();
		path.innerUpperBound = MinKey<Key>()();
		path.innerSlot = path.inner->size;
		path.leafUpperBound = MinKey<Key>()();
		path.leaf = lastLeaf;
		if (path.leaf->isFull()) {
			Key key = GetKey()(row);
			unsafeSplitLeafUnbalanced(path, key);
		}
		leafInsert(path.leaf, row);
	}

	void erase(Key key) {
		Path path{ *this };
		path.update(key);
		leafErase(path);
		balanceLeaf(path);
	}

	template<class TRow> void erase(const TRow& row) {
		erase(GetKey()(row));
	}

	template<class TIter, class TSentinel> void erase(TIter rangeBegin, TSentinel rangeEnd) {
		if (rangeBegin == rangeEnd) return;
		TIter iter = rangeBegin;
		Path path{ *this };
		Key key = GetKey()(*iter);
		for (;;) {
			path.update(key);
			leafErase(path);
			++iter;
			if (iter == rangeEnd) {
				balanceLeaf(path);
				return;
			}
			key = GetKey()(*iter);
			if (key >= path.leafUpperBound)
				balanceLeaf(path);
		}
	}

	void clear() {
		LeafNode* current = firstLeaf.next;
		while (current) {
			auto next = current->next;
			destroyLeaf(current);
			current = next;
		}
		firstLeaf.next = nullptr;
		firstLeaf.size = 0;
		firstLeaf.isUnsorted = false;
		lastLeaf = &firstLeaf;

		firstInner.size = 0;

		for (size_t i = 1; i < rootChildren.size(); ++i) {
			destroyInner(rootChildren[i]);
		}
		rootChildren.clear();
		rootChildren.push_back(&firstInner);
		rootKeys.clear();
	}

	size_t size() {
		size_t size = 0;
		LeafNode* current = &firstLeaf;
		do {
			size += current->size;
			current = current->next;
		} while (current);
		return size;
	}
};
