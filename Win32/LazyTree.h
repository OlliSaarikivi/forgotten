#pragma once

#include "Columnar.h"

template<class TIndex, class... TValues> class LazyTree {
	using Index = TIndex;
	using Key = typename Index::Key;
	using GetKey = typename Index::GetKey;

	GetKey getKey = GetKey{};
	static const Key LeastKey = Index::LeastKey;

	static const size_t KeysInCacheLine = (CACHE_LINE_SIZE * 8) / sizeof(Key);
	static const size_t MaxKeys = KeysInCacheLine < 4 ? 4 : KeysInCacheLine;
	static const size_t MinKeys = MaxKeys / 2;

	static const size_t MaxRows = MaxKeys;
	static const size_t MinRows = MaxRows / 3;

	struct LeafNode {
		LeafNode* next;
		size_t size;
		ColumnarArray<MaxRows, TValues...> rows;
		LeafNode* previous;
		bool dirty;

		LeafNode() : size(0), next(nullptr), previous(nullptr), dirty(false)
		{}

		bool isFull() {
			return size == MaxRows;
		}

		Key leastKey(size_t pos) {
			assert(size > pos);
			Key min = GetKey()(rows[pos]);
			for (size_t i = pos + 1; i < size; ++i) {
				min = std::min(min, GetKey()(rows[i]));
			}
			return min;
		}

		void insertionSort(size_t left, size_t right) {
			for (size_t i = left + 1; i <= right; ++i) {
				auto value = rows[i].temp();
				auto key = GetKey()(value);
				auto j = i - 1;
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

			swap(rows[middle], rows[right - 1]);
			return right - 1;
		}

		size_t quickPartition(int left, int right, int pivotIndex) {
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

		size_t quickPartitionHoare(int left, int right, int pivotIndex) {
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
			if (right - left <= 24) {
				insertionSort(left, right);
				return;
			}
			auto pivotIndex = choosePivot(left, right);
			pivotIndex = quickPartitionHoare(left, right, pivotIndex);
			quickSort(left, pivotIndex);
			quickSort(pivotIndex + 1, right);
		}

		void sort() {
			if (dirty && size > 0) {
				quickSort(0, size - 1);
				dirty = false;
			}
		}

		size_t quickPrepare(int left, int right, size_t minLeft, size_t minRight) {
			if (left == right)
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

		void leonardoPrepareBalance(size_t toRight) {
			while (heapSize < size) {
				++heapSize;
				leonardo::heapPush(::begin(rows), heapSize, size, heapShape, GetKey());
			}
			for (int i = 0; i < toRight; ++i) {
				--heapSize;
				leonardo::heapPop(::begin(rows), heapSize, heapShape, GetKey());
			}
		}

		size_t prepareBalance(size_t minLeft, size_t minRight) {
			return quickPrepare(0, size - 1, minLeft, minRight);
		}

		size_t prepareSplit() {
			assert(size != 0);
			return quickPrepare(0, size - 1, MinRows, MinRows);
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
		size_t toNextBegin = leaf->prepareSplit();

		LeafNode* newLeaf = constructLeaf();
		newLeaf->previous = leaf;
		if (leaf->next) {
			newLeaf->next = leaf->next;
			leaf->next->previous = newLeaf;
		}
		else
			lastLeaf = newLeaf;
		leaf->next = newLeaf;
		newLeaf->dirty = leaf->dirty;

		auto toNext = leaf->size - toNextBegin;
		for (size_t i = 0; i < toNext; ++i) {
			newLeaf->rows[i] |= leaf->rows[toNextBegin + i];
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

	template<class TRow> void leafInsert(LeafNode* leaf, const TRow& row) {
		leaf->rows[leaf->size] |= row;
		++(leaf->size);
		leaf->dirty = true;
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

			if ((slot + 1) < rootKeys.size())
				path.innerUpperBound = rootKeys[slot + 1];
			else
				path.innerUpperBound = LeastKey;

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

			leaf->dirty = true;

			parent->keys[slot - 1] = getKey(leaf->rows[oldEnd]);
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

			leaf->dirty |= next->dirty;

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
		if (leaf->next)
			leaf->next->previous = leaf;

		if ((slot + 1) < parent->size)
			path.leafUpperBound = parent->keys[slot + 1];
		else
			path.leafUpperBound = path.innerUpperBound;

		if (lastLeaf == next)
			lastLeaf = leaf;

		next->dirty = true;

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

		prev->dirty |= leaf->dirty;

		innerErase(path, path.innerSlot + 1);

		destroyLeaf(leaf);
		return;
	}

	void leafErase(LeafNode* leaf, Key key) {
		for (size_t i = 0; i < leaf->size; ++i) {
			if (getKey(leaf->rows[i]) == key) {
				leaf->rows[i] |= leaf->rows[leaf->size - 1];
				--(leaf->size);

				leaf->dirty = true;
				return;
			}
		}
	}

public:
	LazyTree() : innerPool(sizeof(InnerNode)), leafPool(sizeof(LeafNode)), lastLeaf(&firstLeaf) {
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

		leafInsert(path.leaf, row);
	}

	template<class TRow> void erase(const TRow& row) {
		Key key = getKey(row);
		Path path;
		findInner(path, key);
		findLeaf(path, key);

		leafErase(path.leaf, key);
		balanceLeaf(path);

		//LeafNode* current = &firstLeaf;
		//do {
		//	assert(current->size >= MinRows);
		//	current = current->next;
		//} while (current);

		//set<int> s;
		//for (auto row : *this) {
		//	int r = int(row);
		//	assert(s.find(r) == ::end(s));
		//	s.emplace(r);
		//}
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

template<class... Ts> inline auto begin(LazyTree<Ts...>& t) { return t.begin(); }
template<class... Ts> inline auto end(LazyTree<Ts...>& t) { return t.end(); }