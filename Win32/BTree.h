#pragma once

#include "Columnar.h"

template<class TKeyExtractor, class TKeyLess, class TLess, class... TValues> class BTree {
	using KeyType = typename TKeyExtractor::KeyType;

	static const size_t KeysInCacheLine = (CACHE_LINE_SIZE - 1) / sizeof(KeyType);
	static const size_t MaxKeys = KeysInCacheLine < 2 ? 2 : KeysInCacheLine;

	struct LeafData;
	struct Node {
		using ChildrenType = array<unique_ptr<Node>, MaxKeys + 1>;

		array<KeyType, MaxKeys> keys;
		uint8_t typeAndSize;
		Node* parent;
		union {
			ChildrenType children;
			LeafData leafData;
		};

		Node(Node* parent, Node* next) : typeAndSize(0x80), parent(parent) {
			new (&leafData) LeafData();
			leafData.next = next;
		}
		Node(Node* parent) : typeAndSize(0), parent(parent) {
			new (&children) ChildrenType();
		}
		~Node() {
			if (isLeaf())
				leafData.~LeafData();
			else
				children.~ChildrenType();
		}

		uint8_t isLeaf() {
			return typeAndSize & 0x80;
		}
		uint8_t size() {
			return typeAndSize & 0x7F;
		}
		void addSize(uint8_t x) {
			typeAndSize += x;
		}
	};
	struct LeafData {
		array<size_t, MaxKeys> offsets;
		Node* next;
		Columnar<TValues...> records;
	};

	template<class... TSomeValues> class Iterator {
		using IteratorType = Columnar<TValues...>::Iterator<TSomeValues...>;

		Node* current;
		IteratorType pos;
		IteratorType currentEnd;

		void initIterators() {
			pos = current->leafData.records.begin<TSomeValues...>();
			currentEnd = current->leafData.records.end<TSomeValues...>();
		}

	public:
		using reference = typename IteratorType::reference;
		using pointer = typename IteratorType::pointer;
		using iterator_category = std::forward_iterator_tag;

		Iterator() : current(nullptr) {}
		Iterator(Node* current) : current(current) {
			initIterators();
		}
		Iterator(Node* current, IteratorType pos) : current(current), pos(pos) {
			currentEnd = current->leafData.records.end<TSomeValues...>();
		}

		Iterator& operator++() {
			++pos;
			if (pos == currentEnd) {
				current = current->leafData.next;
				if (current != nullptr)
					initIterators();
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
			return left.current == right.current && (left.current == nullptr || left.pos == right.pos);
		}
		friend bool operator!=(const Iterator& left, const Iterator& right) {
			return !(left == right);
		}

		friend void swap(Iterator& left, Iterator& right) {
			using std::swap;
			swap(left.pos, right.pos);
		}
	};

	unique_ptr<Node> root;
	Node* firstLeaf;

	template<int N> uint8_t findSlot(const array<KeyType, N>& keys, uint_fast8_t lower, uint_fast8_t upper, KeyType key) {
		while (lower != upper) {
			uint_fast8_t middle = (lower + upper) / 2;
			if (TKeyLess()(key, keys[middle]))
				upper = middle;
			else
				lower = middle + 1;
		}
		return lower;
	}

	Node* findLeaf(KeyType key) {
		Node* current = root.get();
		while (!current->isLeaf()) {
			auto slot = findSlot(current->keys, 0, current->size(), key);
			current = current->children[slot].get();
		}
		return current;
	}

public:
	BTree() : root(std::make_unique<Node>(nullptr)), firstLeaf(root.get()) {}

	template<class T, class... Ts> auto begin() {
		return Iterator<T, Ts...>(firstLeaf);
	}
	auto begin() {
		return begin<TValues...>();
	}

	template<class T, class... Ts> auto end() {
		return Iterator<T, Ts...>();
	}
	auto end() {
		return end<TValues...>();
	}

	template<class... Ts> pair<Iterator<TValues...>, bool> insert(const Row<Ts...>& row) {
		KeyType key = TKeyExtractor()(row);
		Node* leaf = findLeaf(key);
		auto size = leaf->size();
		auto slot = findSlot(leaf->keys, 0, size, key);
		if (slot == 0 || TKeyLess()(leaf->keys[slot-1], key)) {
			if (size == MaxKeys) {
				// Insert new and balance and all that
				// Fix up leaf and slot for later code
			}
			for (auto i = size; i > slot; --i) {
				leaf->keys[i] = leaf->keys[i - 1];
				leaf->leafData.offsets[i] = leaf->leafData.offsets[i - 1];
			}
			leaf->keys[slot] = key;
			// offsets[slot] is already correct
		}
		else {
			slot = slot - 1;
		}

		auto& records = leaf->leafData.records;

		// Find upper bound of actual records with binary search
		size_t lower = leaf->leafData.offsets[slot];
		size_t end = slot + 1 == leaf->size() ? records.size() : leaf->leafData.offsets[slot + 1];
		size_t upper = end;
		while (lower != upper) {
			size_t middle = (lower + upper) / 2;
			if (TLess()(row, records[middle]))
				upper = middle;
			else
				lower = middle + 1;
		}

		// Insert new or return existing
		auto pos = records.begin() + lower;
		if (lower == 0 || TLess()(*(pos - 1), row)) {
			records.insert(pos, row);
			return std::make_pair(Iterator<TValues...>(leaf, records.begin() + lower), true);
		}
		else
			return std::make_pair(Iterator<TValues...>(leaf, pos), false);
	}
};
