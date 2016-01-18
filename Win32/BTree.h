#pragma once

#include "Columnar.h"
#include "Row.h"

template<class TKey, class... TValues> class BTree {
	using KeyType = typename TKey::KeyType;
	using KeyLess = typename TKey::Less;
	using GetKey = TKey;
	static const KeyType LeastKey = TKey::LeastKey;

	static const size_t KeysInCacheLine = ((CACHE_LINE_SIZE * 3) / 2) / sizeof(KeyType);
	static const size_t MaxKeys = KeysInCacheLine < 2 ? 2 : KeysInCacheLine;
	static const size_t MinKeys = MaxKeys / 2;

	static const size_t MaxInnerLevels = 16;

	static const size_t MaxRows = 64;
	static const size_t MinRows = 32;

	using Rows = ColumnarArray<MaxRows, TValues...>;

	struct InnerNode;
	struct LeafNode;
	struct NodePtr {
		uintptr_t value;
		InnerNode* innerDebug;
		LeafNode* leafDebug;

		NodePtr() : value(reinterpret_cast<uintptr_t>(nullptr)) {}
		NodePtr(nullptr_t n) : value(reinterpret_cast<uintptr_t>(nullptr)) {}
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
			assert(isLeaf() || value == reinterpret_cast<uintptr_t>(nullptr));
			return reinterpret_cast<LeafNode*>(value & ~((uintptr_t)1u));
		}

		friend bool operator==(const NodePtr& left, const NodePtr& right) {
			return left.value == right.value;
		}
	};

	struct InnerNode {
		uint8_t size;
		array<KeyType, MaxKeys> keys;
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
		uint_fast8_t slot;
	};
	struct Path {
		PathEntry entries[MaxInnerLevels * 2];
		size_t end_;
		size_t free;

		Path(NodePtr root) : end_(16), free(15) {
			entries[end_].node = root;
			entries[end_].upperBound = LeastKey;
			entries[end_].slot = 0;
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
	LeafNode* firstLeaf;
	LeafNode* lastLeaf;

	void findLeaf(KeyType key, Path& path) {
		PathEntry* current = &path[0];
		while (current->node.isInner()) {
			InnerNode* inner = current->node.inner();
			KeyType newBound = current->upperBound;
			uint_fast8_t slot = 0;
			for (; slot < inner->size; ++slot) {
				if (KeyLess()(key, inner->keys[slot])) {
					newBound = inner->keys[slot];
					break;
				}
			}
			current = path.descend();
			current->node = inner->children[slot];
			current->upperBound = newBound;
			current->slot = slot;
		}
	}

	NodePtr prevSibling(InnerNode* inner, uint_fast8_t slot) {
		return slot != 0 ? inner->children[slot - 1] : nullptr;
	}

	NodePtr nextSibling(InnerNode* inner, uint_fast8_t slot) {
		return slot != inner->size ? inner->children[slot + 1] : nullptr;
	}

	void innerInsert(Path& path, size_t myLevel, KeyType key, NodePtr node, uint_fast8_t pos) {
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

			uint_fast8_t insertAt = path[myLevel].slot + 1;

			KeyType middleKey = inner->keys[numLeft - 1];
			InnerNode* target = inner;
			if (KeyLess()(key, middleKey))
				path[myLevel].upperBound = middleKey;
			else {
				path[myLevel].node = newSibling;
				++(path[myLevel].slot);
				path[myLevel - 1].slot -= numLeft;
				pos -= numLeft;
				target = newSibling;
			}

			if (inner != root.inner())
				innerInsert(path, myLevel + 1, middleKey, newSibling, insertAt);
			else {
				InnerNode* parent = innerPool.construct();
				parent->keys[0] = middleKey;
				parent->children[0] = inner;
				parent->children[1] = newSibling;
				parent->size = 1;

				PathEntry* rootEntry = path.addRoot();
				rootEntry->node = parent;
				rootEntry->upperBound = LeastKey;
				rootEntry->slot = 0;
				root = parent;
			}

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

	void split(Path& path, KeyType key) {
		assert(path[0].node.isLeaf());
		LeafNode* leaf = path[0].node.leaf();
		assert(leaf->isFull());

		LeafNode* newLeaf = leafPool.construct();
		newLeaf->previous = leaf;
		if (leaf->next) {
			newLeaf->next = leaf->next;
			leaf->next->previous = newLeaf;
		} else
			lastLeaf = newLeaf;
		leaf->next = newLeaf;

		uint_fast8_t toNext = (MaxRows + 1) / 2;
		for (uint_fast8_t i = 0; i < toNext; ++i) {
			newLeaf->rows[i] |= leaf->rows[i + (MaxRows - toNext)];
		}
		leaf->size = MaxRows - toNext;
		newLeaf->size = toNext;

		uint_fast8_t insertAt = path[0].slot + 1;

		KeyType splitKey = GetKey()(newLeaf->rows[0]);
		if (KeyLess()(key, splitKey))
			path[0].upperBound = splitKey;
		else {
			path[0].node = newLeaf;
			++(path[0].slot);
		}

		if (!root.isLeaf())
			innerInsert(path, 1, splitKey, NodePtr(newLeaf), insertAt);
		else {
			InnerNode* parent = innerPool.construct();
			parent->keys[0] = splitKey;
			parent->children[0] = leaf;
			parent->children[1] = newLeaf;
			parent->size = 1;

			PathEntry* rootEntry = path.addRoot();
			rootEntry->node = parent;
			rootEntry->upperBound = LeastKey;
			rootEntry->slot = 0;
			root = parent;
		}
	}

	void removeRedundantRoot() {
		if (root.inner()->size == 0) {
			InnerNode* oldRoot = root.inner();
			root = oldRoot->children[0];
			innerPool.destroy(oldRoot);
		}
	}

	void innerErase(Path& path, size_t myLevel, uint_fast8_t slot) {
		InnerNode* inner = path[myLevel].node.inner();

		for (uint_fast8_t i = slot; i < inner->size; ++i) {
			inner->keys[i - 1] = inner->keys[i];
			inner->children[i] = inner->children[i + 1];
		}
		--(inner->size);

		if (inner->size < MinKeys && root.inner() != inner) {
			InnerNode* parent = path[myLevel + 1].node.inner();
			uint_fast8_t slot = path[myLevel].slot;
			InnerNode* prev = prevSibling(parent, slot).inner();
			InnerNode* next = nextSibling(parent, slot).inner();

			if (prev && prev->size > MinKeys) {
				for (uint_fast8_t i = 0; i < inner->size; ++i) {
					inner->keys[inner->size - i] = inner->keys[inner->size - i - 1];
					inner->children[inner->size + 1 - i] = inner->children[inner->size - i];
				}
				inner->children[1] = inner->children[0];

				inner->children[0] = prev->children[prev->size];
				inner->keys[0] = parent->keys[slot - 1];
				parent->keys[slot - 1] = prev->keys[prev->size - 1];

				++(inner->size);
				--(prev->size);
			}
			else if (next && next->size > MinKeys) {
				inner->children[inner->size] = next->children[0];
				inner->keys[inner->size - 1] = parent->keys[slot];
				parent->keys[slot] = next->keys[0];

				++(inner->size);
				--(next->size);

				next->children[0] = next->children[1];
				for (uint_fast8_t i = 1; i <= next->size; ++i) {
					next->keys[i - 1] = next->keys[i];
					next->children[i] = next->children[i + 1];
				}
			}
			else if (next) {
				inner->keys[inner->size] = parent->keys[slot];
				for (uint_fast8_t i = 0; i < next->size; ++i) {
					inner->keys[inner->size + 1 + i] = next->keys[i];
					inner->children[inner->size + 1 + i] = next->children[i];
				}
				inner->children[inner->size + next->size + 1] = next->children[next->size];

				inner->size += next->size;

				innerErase(path, myLevel + 1, slot + 1);

				innerPool.destroy(next);
				removeRedundantRoot();
			}
			else if (prev) {
				prev->keys[prev->size] = parent->keys[slot - 1];
				for (uint_fast8_t i = 0; i < inner->size; ++i) {
					prev->keys[prev->size + 1 + i] = inner->keys[i];
					prev->children[prev->size + 1 + i] = inner->children[i];
				}
				prev->children[prev->size + inner->size + 1] = inner->children[inner->size];

				prev->size += inner->size;

				innerErase(path, myLevel + 1, slot);

				innerPool.destroy(inner);
				removeRedundantRoot();
			}
		}
	}

	void balance(Path& path) {
		LeafNode* leaf = path[0].node.leaf();
		uint_fast8_t needed = leaf->size < MinRows ? MinRows - leaf->size : 0;
		if (needed > 0 && !root.isLeaf()) {
			InnerNode* parent = path[1].node.inner();
			uint_fast8_t slot = path[0].slot;
			LeafNode* prev = prevSibling(parent, slot).leaf();
			LeafNode* next = nextSibling(parent, slot).leaf();
			uint_fast8_t prevAvailable = prev && prev->size > MinRows ? prev->size - MinRows : 0;
			uint_fast8_t nextAvailable = next && next->size > MinRows ? next->size - MinRows : 0;

			if (prevAvailable + nextAvailable > needed) {
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

					parent->keys[slot - 1] = GetKey()(leaf->rows[0]);
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

					KeyType newUpperBound = GetKey()(next->rows[0]);
					path[0].upperBound = newUpperBound;
					parent->keys[slot] = newUpperBound;
				}
			}
			else {
				// Merge
				if (next) {
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

					assert(leaf->size <= MaxRows);
					innerErase(path, 1, path[0].slot + 1);

					leafPool.destroy(next);
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

					assert(prev->size <= MaxRows);
					innerErase(path, 1, path[0].slot + 1);

					leafPool.destroy(leaf);
					removeRedundantRoot();
				}
				else
					assert(false);
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
		if (rangeIter == rangeEnd) goto ALL_DONE;
		KeyType rangeKey = GetKey()(*rangeIter);
		for (; i < leaf->size; ++i) {
			KeyType key = GetKey()(leaf->rows[i]);
			for (;;) {
				if (!KeyLess()(rangeKey, key)) {
					if (KeyLess()(key, rangeKey)) {
						if (out != i) leaf->rows[out] |= leaf->rows[i];
						++out;
					}
					//else { // Not necessary for correctness
					//	++rangeIter;
					//	if (rangeIter == rangeEnd) {
					//		++i;
					//		goto MATCH_DONE;
					//	}
					//	KeyType rangeKey = GetKey()(*rangeIter);
					//}
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
		leaf->size -= i - out;
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

			if (path[0].node.leaf()->isFull()) {
				LeafNode* leaf = path[0].node.leaf();

				if (KeyLess()(GetKey()(leaf->rows[leaf->size - 1]), key)) {
					TIter countIter = iter;
					while (countIter - iter < MinRows && countIter != rangeEnd &&
						(path[0].upperBound == LeastKey || KeyLess()(GetKey()(*countIter), path[0].upperBound))) {

						++countIter;
					}

					if (countIter - iter >= MinRows) { // Split without moving anything
						LeafNode* newLeaf = leafPool.construct();
						newLeaf->previous = leaf;
						if (leaf->next) {
							newLeaf->next = leaf->next;
							leaf->next->previous = newLeaf;
						}
						else
							lastLeaf = newLeaf;
						leaf->next = newLeaf;

						uint_fast8_t insertAt = path[0].slot + 1;

						path[0].node = newLeaf;
						++(path[0].slot);

						if (!root.isLeaf())
							innerInsert(path, 1, key, NodePtr(newLeaf), insertAt);
						else {
							InnerNode* parent = innerPool.construct();
							parent->keys[0] = key;
							parent->children[0] = leaf;
							parent->children[1] = newLeaf;
							parent->size = 1;

							PathEntry* rootEntry = path.addRoot();
							rootEntry->node = parent;
							rootEntry->upperBound = LeastKey;
							rootEntry->slot = 0;
							root = parent;
						}
					}
					else
						split(path, key);
				}
				else
					split(path, key);
			}

			if (path[0].upperBound == LeastKey)
				iter = leafInsertSorted(path[0].node.leaf(), iter, rangeEnd);
			else
				iter = leafInsertSorted(path[0].node.leaf(), iter, rangeEnd, InsertBound{ path[0].upperBound });
		}
	}

	template<class TRow> void unsafeAppend(const TRow& row) {
		KeyType key = GetKey()(row);
		if (lastLeaf->isFull()) {
			Path path{ root };
			while (path[0].node.isInner()) {
				InnerNode* inner = path[0].node.inner();
				path.descend();
				path[0].node = inner->children[inner->size];
				path[0].upperBound = LeastKey;
				path[0].slot = inner->size;
			}

			LeafNode* leaf = path[0].node.leaf();
			assert(leaf->isFull());
			assert(leaf == lastLeaf);

			LeafNode* newLeaf = leafPool.construct();
			newLeaf->previous = leaf;
			lastLeaf = newLeaf;
			leaf->next = newLeaf;

			uint_fast8_t insertAt = path[0].slot + 1;

			if (!root.isLeaf())
				innerInsert(path, 1, key, NodePtr(newLeaf), insertAt);
			else {
				InnerNode* parent = innerPool.construct();
				parent->keys[0] = key;
				parent->children[0] = leaf;
				parent->children[1] = newLeaf;
				parent->size = 1;
				root = parent;
			}
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

		validate();

		balance(path);

		validate();
	}

	void validate() {
		optional<KeyType> prevKey;
		auto iter = begin();
		while (iter != end()) {
			auto row = *iter;
			KeyType key = GetKey()(*iter);
			if (prevKey) {
				if (!KeyLess()(*prevKey, key)) {
					assert(false);
				}
			}
			prevKey = key;
			++iter;
		}

		assertBounds(root, std::numeric_limits<KeyType>::min(), std::numeric_limits<KeyType>::max());
	}

	void printStats() {
		double sum = 0;
		uint64_t count = 0;
		LeafNode* current = firstLeaf;
		while (current) {
			sum += current->size;
			++count;
			current = current->next;
		}
		std::cout << "Avg leaf size: " << (sum / count) << "\n";
	}
};

template<class... Ts> inline auto begin(BTree<Ts...>& t) { return t.begin(); }
template<class... Ts> inline auto end(BTree<Ts...>& t) { return t.end(); }