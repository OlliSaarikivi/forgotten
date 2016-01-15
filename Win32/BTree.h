#pragma once

#include "Columnar.h"
#include "Row.h"

template<class TKey, class... TValues> class BTree {
	using KeyType = typename TKey::KeyType;
	using KeyLess = typename TKey::Less;
	using GetKey = TKey;
	static const KeyType LeastKey = TKey::LeastKey;

	static const size_t KeysInCacheLine = ((CACHE_LINE_SIZE * 3) / 2 - 1) / sizeof(KeyType);
	static const size_t MaxKeys = KeysInCacheLine < 2 ? 2 : KeysInCacheLine;
	static const size_t MaxInnerLevels = 32; // Gives at least 2^32

	static const size_t MinRows = 32;
	static const size_t MaxRows = 64;

	using Rows = ColumnarArray<MaxRows, TValues...>;

	struct InnerNode;
	struct LeafNode;
	struct NodePtr {
		uintptr_t value;
		InnerNode* innerDebug;
		LeafNode* leafDebug;

		NodePtr() : value(reinterpret_cast<uintptr_t>(nullptr)) {}
		NodePtr(InnerNode* node) : value(reinterpret_cast<uintptr_t>(node)), innerDebug(node) {}
		NodePtr(LeafNode* node) : value(reinterpret_cast<uintptr_t>(node) | 1u), leafDebug(node) {}

		operator bool() { return value != reinterpret_cast<uintptr_t>(nullptr); }

		NodePtr& operator=(InnerNode* node) {
			value = reinterpret_cast<uintptr_t>(node);
			innerDebug = node;
			return *this;
		}
		NodePtr& operator=(LeafNode* node) {
			value = reinterpret_cast<uintptr_t>(node) | 1u;
			leafDebug = node;
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
		uint8_t size;
		array<NodePtr, MaxKeys + 1> children;

		InnerNode() : size(0) {}

		bool isFull() {
			return size == MaxKeys;
		}
	};
	struct LeafNode {
		LeafNode* next;
		Rows rows;
		uint8_t size;
		LeafNode* previous;

		LeafNode() : size(0), next(nullptr), previous(nullptr) {}

		bool isFull() {
			return size == MaxRows;
		}
	};

	struct PathEntry {
		NodePtr node;
		KeyType upperBound;
	};
	struct Path {
		PathEntry entries[32]; // Actual capacity 16
		size_t end_;
		size_t free;

		Path(NodePtr root) : end_(16), free(15) {
			entries[end_].node = root;
			entries[end_].upperBound = LeastKey;
		}

		PathEntry& operator[](size_t pos) { return entries[end_ - pos]; }
		PathEntry* descend() { return entries + (++end_); }
		void ascend() { --end_; }
		PathEntry* addRoot() { return entries + (free--); }
	};

	static_assert(alignof(InnerNode) >= 2, "Can not use low byte for type info because InnerNode has alignment of 1 byte.");
	static_assert(alignof(LeafNode) >= 2, "Can not use low byte for type info because LeafNode has alignment of 1 byte.");

	template<class... TSomeValues> class Iterator {
		using IteratorType = Rows::Iterator<TSomeValues...>;

		LeafNode* current;
		size_t pos;

	public:
		using reference = typename IteratorType::reference;
		using pointer = typename IteratorType::pointer;
		using iterator_category = std::forward_iterator_tag;

		Iterator(LeafNode* current) : current(current), pos(0) {}
		Iterator(LeafNode* current, size_t pos) : current(current), pos(pos) {}

		Iterator& operator++() {
			++pos;
			if (current->next != nullptr && pos == current->size) {
				current = current->next;
				pos = 0;
				_mm_prefetch((const char*)current->next, _MM_HINT_T0);
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
	size_t innerLevels = 0;
	LeafNode* firstLeaf;
	LeafNode* lastLeaf;

	void findLeaf(KeyType key, Path& path) {
		PathEntry* current = &path[0];
		while (current->node.isInner()) {
			InnerNode* inner = current->node.inner();
			KeyType newBound = current->upperBound;
			size_t slot = 0;
			for (; slot < inner->size; ++slot) {
				if (KeyLess()(key, inner->keys[slot])) {
					newBound = inner->keys[slot];
					break;
				}
			}
			current = path.descend();
			current->node = inner->children[slot];
			current->upperBound = newBound;
		}
	}

	void updateKey(InnerNode* inner, KeyType key, KeyType newKey) {
		size_t slot = 0;
		for (; slot < inner->size; ++slot) {
			if (KeyLess()(key, inner->keys[slot])) {
				break;
			}
		}
		if (slot == 0) {
			if (inner->parent)
				updateKey(inner->parent, key, newKey);
		}
		else
			inner->keys[slot - 1] = newKey;
	}

	void updateParentKeys(LeafNode* leaf, KeyType oldLowKey) {
		assert(leaf->parent);
		KeyType key = GetKey()(leaf->rows[0]);
		updateKey(leaf->parent, oldLowKey, key);
	}

	void innerInsert(Path& path, size_t myLevel, KeyType key, NodePtr node) {
		InnerNode* inner = path[myLevel].node.inner();

		if (inner->isFull()) {
			// Split
			InnerNode* newSibling = innerPool.construct();
			uint_fast8_t numRight = (MaxKeys + 1) / 2;
			uint_fast8_t numLeft = (MaxKeys + 1) - numRight;
			for (uint_fast8_t i = 0; i < numRight; ++i) {
				newSibling->children[i] = inner->children[numLeft + i];
			}
			for (uint_fast8_t i = 0; i < numRight - 1; ++i) {
				newSibling->keys[i] = inner->keys[numLeft + i];
			}
			inner->size = numLeft - 1;
			newSibling->size = numRight - 1;

			KeyType middleKey = inner->keys[numLeft - 1];
			InnerNode* target = inner;
			if (KeyLess()(key, middleKey))
				path[myLevel].upperBound = middleKey;
			else {
				path[myLevel].node = newSibling;
				target = newSibling;
			}

			if (inner != root.inner())
				innerInsert(path, myLevel + 1, middleKey, newSibling);
			else {
				InnerNode* parent = innerPool.construct();
				parent->keys[0] = middleKey;
				parent->children[0] = inner;
				parent->children[1] = newSibling;
				parent->size = 1;

				PathEntry* rootEntry = path.addRoot();
				rootEntry->node = parent;
				rootEntry->upperBound = LeastKey;
				root = parent;
				++innerLevels;
			}

			inner = target;
		}

		size_t pos = inner->size;
		while (pos > 0) {
			if (KeyLess()(inner->keys[pos - 1], key))
				break;
			inner->keys[pos] = inner->keys[pos - 1];
			inner->children[pos + 1] = inner->children[pos];
			--pos;
		}
		inner->keys[pos] = key;
		inner->children[pos + 1] = node;
		++(inner->size);
	}

	void split(Path& path, KeyType key) {
		assert(path[0].node.isLeaf());
		LeafNode* leaf = path[0].node.leaf();
		assert(leaf->isFull());

		LeafNode* newLeaf = leafPool.construct();
		newLeaf->previous = leaf;
		if (lastLeaf == leaf)
			lastLeaf = newLeaf;
		if (leaf->next) {
			newLeaf->next = leaf->next;
			leaf->next->previous = newLeaf;
		}
		leaf->next = newLeaf;

		uint_fast8_t toNext = (MaxRows + 1) / 2;
		for (uint_fast8_t i = 0; i < toNext; ++i) {
			newLeaf->rows[i] |= leaf->rows[i + (MaxRows - toNext)];
		}
		leaf->size = MaxRows - toNext;
		newLeaf->size = toNext;

		KeyType splitKey = GetKey()(newLeaf->rows[0]);
		if (KeyLess()(key, splitKey))
			path[0].upperBound = splitKey;
		else
			path[0].node = newLeaf;

		if (!root.isLeaf())
			innerInsert(path, 1, splitKey, NodePtr(newLeaf));
		else {
			InnerNode* parent = innerPool.construct();
			parent->keys[0] = splitKey;
			parent->children[0] = leaf;
			parent->children[1] = newLeaf;
			parent->size = 1;

			PathEntry* rootEntry = path.addRoot();
			rootEntry->node = parent;
			rootEntry->upperBound = LeastKey;
			root = parent;
			++innerLevels;
		}
	}

	void innerErase(Path& path, size_t myLevel, KeyType key) {

	}

	void balance(Path& path, KeyType oldLeast) {
		LeafNode* leaf = path[0].node.leaf();
		uint_fast8_t needed = leaf->size < MinRows ? MinRows - leaf->size : 0;
		if (needed > 0) {
			LeafNode* prev = leaf->previous;
			uint_fast8_t prevAvailable = prev && prev->size > MinRows ? prev->size - MinRows : 0;
			LeafNode* next = leaf->next;
			uint_fast8_t nextAvailable = next && next->size > MinRows ? next->size - MinRows : 0;

			if (prevAvailable + nextAvailable > needed) {
				auto fromPrev = std::min(needed, prevAvailable);
				if (fromPrev > 0) {
					for (uint_fast8_t i = 1; i <= leaf->size; ++i) {
						leaf->rows[leaf->size - i + fromPrev] = leaf->rows[leaf->size - i]
					}
					for (int_fast8_t i = 0; i < fromPrev; ++i) {
						leaf->rows[i] = prev->rows[prev->size - fromPrev + i];
					}
					prev->size -= fromPrev;
					leaf->size += fromPrev;
					needed -= fromPrev;

					updateParentKeys(leaf, oldLeast);
				}
				auto fromNext = std::min(needed, nextAvailable);
				if (fromNext > 0) {
					KeyType nextOldLeast = GetKey()(next->rows[0]);

					for (int_fast8_t i = 0; i < fromNext; ++i) {
						leaf->rows[leaf->size + i] = next->rows[i];
					}
					next->size -= fromNext;
					for (int_fast8_t i = 0; i < next->size; ++i) {
						next->rows[i] = next->rows[i + fromNext];
					}
					leaf->size += fromNext;

					path[0].upperBound = GetKey()(next->rows[0]);
					updateParentKeys(next, nextOldLeast);
				}
			}
			else {
				// Merge
				if (next) {
					for (uint_fast8_t i = 0; i < next->size; ++i) {
						leaf->rows[leaf->size + i] = next->rows[i];
					}
					leaf->size += next->size;
					leaf->next = next->next;
					if (leaf->next) {
						leaf->next->previous = leaf;
						path[0].upperBound = GetKey()(leaf->next->rows[0]);
					}
					else
						path[0].upperBound = leastKey;

					assert(leaf->size <= MaxRows);
					innerErase(path)
				}
				else if (prev) {

				}
			}
		}
	}

	struct NoInsertBound {
		bool operator()(KeyType key) { return true; }
	};
	struct InsertBound {
		KeyType upperBound;
		bool operator()(KeyType key) { return KeyLess()(key, upperBound); }
	};
	template<class TIter, class TBound = NoInsertBound> TIter leafInsertSorted(LeafNode* leaf, TIter rangeBegin, TIter rangeEnd, TBound inBound = NoInsertBound{}) {
		uint_fast8_t space = MaxRows - leaf->size;

		TIter actualEnd = rangeBegin;
		for (uint_fast8_t i = 0; i < leaf->size; ++i) {
			KeyType key = GetKey()(leaf->rows[i]);
			for (;;) {
				if (actualEnd == rangeEnd || space == 0) goto FIND_RANGE_DONE;
				KeyType rangeKey = GetKey()(*actualEnd);
				if (!inBound(rangeKey)) goto FIND_RANGE_DONE;
				if (!KeyLess()(rangeKey, key)) {
					while (!KeyLess()(key, rangeKey)) {
						++actualEnd;
						if (actualEnd == rangeEnd) goto FIND_RANGE_DONE;
						rangeKey = GetKey()(*actualEnd);
					}
					if (!inBound(rangeKey)) goto FIND_RANGE_DONE;
					break;
				}
				++actualEnd;
				--space;
			}
		}
		while (actualEnd != rangeEnd && space != 0 && inBound(GetKey()(*actualEnd))) {
			++actualEnd;
			--space;
		}
	FIND_RANGE_DONE:

		auto& rows = leaf->rows;
		auto rowsBegin = ::begin(rows);
		auto rowsBack = rowsBegin + (leaf->size - 1);
		KeyType rowsKey = GetKey()(*rowsBack);
		auto rangeBack = actualEnd - 1;
		auto out = ::begin(rows) + (MaxRows - 1 - space);

		if (rowsBack < rowsBegin) {
			while (rangeBack >= rangeBegin)
				*out-- |= *rangeBack--;
			goto INSERT_DONE;
		}

		if (rangeBack >= rangeBegin) {
			KeyType rangeKey = GetKey()(*rangeBack);
			for (;;) {
				if (KeyLess()(rowsKey, rangeKey)) {
					*out-- |= *rangeBack--;
					if (rangeBack < rangeBegin) goto INSERT_DONE;
					rangeKey = GetKey()(*rangeBack);
				}
				else if (KeyLess()(rangeKey, rowsKey)) {
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

	template<class TIter> TIter leafEraseSorted(LeafNode* leaf, TIter rangeBegin, TIter rangeEnd) {
		TIter rangeIter = rangeBegin;
		uint_fast8_t out = 0;
		uint_fast8_t i = 0;
		if (rangeIter == rangeEnd) goto MATCH_DONE;
		KeyType rangeKey = GetKey()(*rangeIter);
		for (; i < leaf->size; ++i) {
			KeyType key = GetKey()(leaf->rows[i]);
			for (;;) {
				if (!KeyLess()(rangeKey, key)) {
					if (KeyLess()(key, rangeKey)) {
						if (out != i) leaf->rows[out] |= leaf->rows[i];
						++out;
					}
					else { // Not necessary for correctness
						++rangeIter;
						if (rangeIter == rangeEnd) goto MATCH_DONE;
						KeyType rangeKey = GetKey()(*rangeIter);
					}
					break;
				}
				++rangeIter;
				if (rangeIter == rangeEnd) goto MATCH_DONE;
				KeyType rangeKey = GetKey()(*rangeIter);
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
		leaf->size = out;
		return rangeIter;
	}

	void assertBounds(NodePtr node, KeyType lowerBound, KeyType upperBound) {
		if (node.isInner()) {
			auto inner = node.inner();
			for (int i = 0; i < inner->size; ++i) {
				KeyType key = inner->keys[i];
				assert(!KeyLess()(key, lowerBound));
				assert(KeyLess()(key, upperBound));
			}
			assertBounds(inner->children[0], lowerBound, inner->keys[0]);
			for (int i = 1; i < inner->size; ++i) {
				assertBounds(inner->children[i], inner->keys[i - 1], inner->keys[i]);
			}
			assertBounds(inner->children[inner->size], inner->keys[inner->size - 1], upperBound);
		}
		else {
			auto leaf = node.leaf();
			for (int i = 0; i < leaf->size; ++i) {
				KeyType key = GetKey()(leaf->rows[i]);
				assert(!KeyLess()(key, lowerBound));
				assert(KeyLess()(key, upperBound));
			}
		}
	}

public:
	BTree() : root(leafPool.construct()), firstLeaf(root.leaf()), lastLeaf(firstLeaf) {}

	template<class T, class... Ts> auto begin() {
		return Iterator<T, Ts...>(firstLeaf);
	}
	auto begin() {
		return begin<TValues...>();
	}

	template<class T, class... Ts> auto end() {
		return Iterator<T, Ts...>(lastLeaf, lastLeaf->size);
	}
	auto end() {
		return end<TValues...>();
	}

	template<class TRow> void insert(const TRow& row) {
		auto key = GetKey()(row);
		Path path{ root };
		findLeaf(key, path);

		if (path[0].node.leaf()->isFull())
			split(path, key);

		leafInsertSorted(path[0].node.leaf(), &row, &row + 1);
	}

	template<class TIter> void insertSorted(TIter rangeBegin, TIter rangeEnd) {
		TIter iter = rangeBegin;
		Path path{ root };
		while (iter != rangeEnd) {
			auto key = GetKey()(*iter);
			while (path[0].upperBound != LeastKey && !(KeyLess()(key, path[0].upperBound)))
				path.ascend();
			findLeaf(key, path);

			if (path[0].node.leaf()->isFull())
				split(path, key);

			if (path[0].upperBound == LeastKey)
				iter = leafInsertSorted(path[0].node.leaf(), iter, rangeEnd);
			else
				iter = leafInsertSorted(path[0].node.leaf(), iter, rangeEnd, InsertBound{ path[0].upperBound });
		}
	}

	template<class TRow> void unsafeAppend(const TRow& row) {
		if (lastLeaf->isFull()) {
			Path path{ root };
			while (path[0].node.isInner()) {
				InnerNode* inner = path[0].node.inner();
				path.descend();
				path[0].node = inner->children[inner->size];
				path[0].upperBound = LeastKey;
			}
			split(path, LeastKey);
		}

		lastLeaf->rows[lastLeaf->size++] |= row;
	}

	template<class TRow> void erase(const TRow& row) {
		auto key = GetKey()(row);
		Path path{ root };
		findLeaf(key, path);

		LeafNode* leaf = path[0].node.leaf();
		KeyType oldLeast = GetKey()(leaf->rows[0]);
		leafEraseSorted(leaf, &row, &row + 1);
		balance(path, oldLeast);
	}

	void validate() {
		optional<KeyType> prevKey;
		auto iter = begin();
		while (iter != end()) {
			auto row = *iter;
			KeyType key = GetKey()(*iter);
			if (prevKey) {
				if (!KeyLess()(*prevKey, key)) {
					for (auto row2 : *this) {
						std::cout << int(row2) << " ";
					}
					assert(false);
				}
			}
			prevKey = key;
			++iter;
		}

		assertBounds(root, std::numeric_limits<KeyType>::min(), std::numeric_limits<KeyType>::max());
	}
};

template<class... Ts> inline auto begin(BTree<Ts...>& t) { return t.begin(); }
template<class... Ts> inline auto end(BTree<Ts...>& t) { return t.end(); }