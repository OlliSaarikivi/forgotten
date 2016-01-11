#pragma once

#include "Columnar.h"
#include "Row.h"

template<class TKey, class... TValues> class BTree {
	using KeyType = typename TKey::KeyType;
	using KeyLess = typename TKey::Less;
	using GetKey = TKey;
	static const KeyType LeastKey = TKey::LeastKey;

	static const size_t KeysInCacheLine = (CACHE_LINE_SIZE - 1) / sizeof(KeyType);
	static const size_t MaxKeys = KeysInCacheLine < 2 ? 2 : KeysInCacheLine;
	static const size_t MaxInnerLevels = 32; // Gives at least 2^32

	static const size_t MaxRecords = MaxKeys - 1;

	using Records = ColumnarArray<MaxRecords, TValues...>;

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

	alignas(2) struct InnerNode {
		array<KeyType, MaxKeys> keys;
		uint8_t size;
		array<NodePtr, MaxKeys + 1> children;
		InnerNode* parent;

		InnerNode() : parent(nullptr) {
			keys.fill(LeastKey);
			children.fill(NodePtr{});
		}
	};
	alignas(2) struct LeafNode {
		Records records;
		uint8_t size;
		LeafNode* next;
		LeafNode* previous;
		InnerNode* parent;

		LeafNode() : next(nullptr), previous(nullptr), parent(nullptr) {}
	};

	template<class... TSomeValues> class Iterator {
		using IteratorType = Records::Iterator<TSomeValues...>;

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
			}
			return *this;
		}
		Iterator operator++(int) {
			Iterator old = *this;
			this->operator++();
			return old;
		}

		reference operator*() const {
			return current->records[pos];
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

	//uint8_t findSlot(InnerNode* inner, KeyType key) {
	//	for (uint_fast8_t i = 0; i < inner->size; ++i) {
	//		if (KeyLess()(key, inner->keys[i]))
	//			return i;
	//	}
	//	return inner->size;
	//}

	pair<LeafNode*, optional<KeyType>> findLeaf(KeyType key) {
		NodePtr current = root;
		optional<KeyType> leafBound;
		while (current.isInner()) {
			InnerNode* inner = current.inner();
			uint_fast8_t slot = 0;
			for (; slot < inner->size; ++slot) {
				if (KeyLess()(key, inner->keys[slot])) {
					leafBound = inner->keys[slot];
					break;
				}
			}
			current = inner->children[slot];
		}
		return make_pair(current.leaf(), leafBound);
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

	template<class TIter> void insertSorted(TIter rangeBegin, TIter rangeEnd) {
		TIter iter = rangeBegin;
		while (iter != rangeEnd) {
			auto key = GetKey()(*iter);
			auto result = findLeaf(key);

			optional<KeyType> bound = result.second;
			TIter boundedEnd = rangeEnd;
			if (bound) {
				boundedEnd = iter + 1;
				while (KeyLess()(GetKey()(*boundedEnd), *bound))
					++boundedEnd;
			}

			LeafNode* leaf = result.first;
			size_t boundedSize = boundedEnd - iter;
			if (leaf->size + boundedSize)
		}
	}
};

template<class... Ts> inline auto begin(BTree<Ts...>& t) { return t.begin(); }
template<class... Ts> inline auto end(BTree<Ts...>& t) { return t.end(); }