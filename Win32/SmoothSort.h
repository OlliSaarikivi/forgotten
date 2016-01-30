#pragma once

namespace leonardo {

const size_t LNums[] =    { 1, 1, 3, 5, 9, 15, 25, 41, 67, 109, 177, 287, 465, 753, 1219,
							1973, 3193, 5167, 8361, 13529, 21891, 35421u, 57313u, 92735u,
							150049u, 242785u, 392835u, 635621u, 1028457u, 1664079u, 2692537u,
							4356617u, 7049155u, 11405773u, 18454929u, 29860703u, 48315633u,
							78176337u, 126491971u, 204668309u, 331160281u, 535828591u, 866988873u,
							1402817465u, 2269806339u, 3672623805u, 5942430145u, 9615053951u,
							15557484097u, 25172538049u, 40730022147u, 65902560197u, 106632582345u,
							172535142543u, 279167724889u, 451702867433u, 730870592323u,
							1182573459757u, 1913444052081u, 3096017511839u, 5009461563921u,
							8105479075761u, 13114940639683u, 21220419715445u, 34335360355129u,
							55555780070575u, 89891140425705u, 145446920496281u, 235338060921987u,
							380784981418269u, 616123042340257u, 996908023758527u, 1613031066098785u,
							2609939089857313u, 4222970155956099u, 6832909245813413u,
							11055879401769513u, 17888788647582927u, 28944668049352441u,
							46833456696935369u, 75778124746287811u, 122611581443223181u,
							198389706189510993u, 321001287632734175u, 519390993822245169u,
							840392281454979345u, 1359783275277224515u, 2200175556732203861u,
							3559958832009428377u, 5760134388741632239u, 9320093220751060617u,
							15080227609492692857u };

template<unsigned N> struct HeapShape {
	HeapShape() : size(0) {
		// Dummy values to make the addTree comparisons work
		orders[0] = 32;
		orders[1] = 64;
	}

	void addTree() {
		if (orders[size + 1] == (orders[size] - 1)) {
			orders[size] += 1;
			--size;
		}
		else {
			orders[size + 2] = orders[size + 1] == 1 ? 0 : 1;
			++size;
		}
	}

	uint8_t& lowestOrder() {
		assert(size > 0);
		return orders[size + 1];
	}

	uint8_t& order(size_t i) {
		return orders[size + 1 - i];
	}

	size_t invalidate(size_t numValid) {
		size_t sum = 0;
		for (int i = 0; i < size; ++i) {
			size_t newSum = sum + LNums[orders[2 + i]];
			if (newSum > numValid) {
				size = i;
				break;
			}
			sum = newSum;
		}
		return sum;
	}

	size_t size;

private:
	array<uint8_t, N + 2> orders;
};

size_t rightChild(size_t root) {
	return root - 1;
}

size_t leftChild(size_t root, uint8_t order) {
	return rightChild(root) - LNums[order - 2];
}

template<class TIter, class TGetKey> size_t largerChild(TIter heap, size_t root, uint_fast8_t order, TGetKey getKey) {
	auto left = leftChild(root, order);
	auto right = rightChild(root);
	return getKey(heap[left]) < getKey(heap[right]) ? right : left;
}

template<class TIter, class TGetKey>
void rebalanceOne(TIter heap, size_t root, uint_fast8_t order, TGetKey getKey) {
	while (order > 1) {
		auto left = leftChild(root, order);
		auto right = rightChild(root);

		auto leftKey = getKey(heap[left]);
		auto rightKey = getKey(heap[right]);

		auto largerKey = leftKey;
		auto child = left;
		auto childOrder = order - 1;
		if (leftKey < rightKey) {
			largerKey = rightKey;
			child = right;
			childOrder = order - 2;
		}

		if (getKey(heap[root]) >= largerKey)
			return;

		swap(heap[root], heap[child]);
		root = child;
		order = childOrder;
	}
}

template<class TIter, unsigned N, class TGetKey>
void rectify(TIter heap, size_t root, size_t rootOrderIndex, HeapShape<N>& s, TGetKey getKey) {
	auto current = root;
	size_t currentOrderIndex = rootOrderIndex;

	for (;;) {
		if (currentOrderIndex == s.size - 1)
			break;

		auto toCompare = current;
		auto toCompareKey = getKey(heap[toCompare]);
		auto currentOrder = s.order(currentOrderIndex);
		if (currentOrder > 1) {
			auto left = leftChild(current, currentOrder);
			auto right = rightChild(current);
			auto leftKey = getKey(heap[left]);
			auto rightKey = getKey(heap[right]);
			auto largerKey = leftKey;
			auto child = left;
			if (leftKey < rightKey) {
				largerKey = rightKey;
				child = right;
			}
			if (toCompareKey < largerKey) {
				toCompareKey = largerKey;
				toCompare = child;
			}
		}

		auto prior = current - LNums[currentOrder];
		auto priorKey = getKey(heap[prior]);
		if (toCompareKey >= priorKey)
			break;

		swap(heap[current], heap[prior]);
		current = prior;
		++currentOrderIndex;
	}

	rebalanceOne(heap, current, s.order(currentOrderIndex), getKey);
}

template<class TIter, unsigned N, class TGetKey>
void heapPushOne(TIter heap, size_t newSize, HeapShape<N>& s, TGetKey getKey) {
	s.addTree();
	rectify(heap, newSize - 1, 0, s, getKey);
}

template<class TIter, unsigned N, class TGetKey>
void heapPush(TIter heap, size_t newSize, size_t finalSize, HeapShape<N>& s, TGetKey getKey) {
	s.addTree();

	bool isLast;
	switch (s.lowestOrder()) {
	case 0:
		isLast = newSize == finalSize;
		break;
	case 1:
		isLast = newSize == finalSize || (newSize + 1 == finalSize && s.lowestOrder() != s.order(1) - 1);
		break;
	default:
		isLast = (finalSize - newSize) < LNums[s.lowestOrder() - 1] + 1;
	}

	if (!isLast)
		rebalanceOne(heap, newSize - 1, s.lowestOrder(), getKey);
	else
		rectify(heap, newSize - 1, 0, s, getKey);
}

template<class TIter, unsigned N, class TGetKey>
void heapPop(TIter heap, size_t newSize, HeapShape<N>& s, TGetKey getKey) {
	if (s.lowestOrder() <= 1) {
		--(s.size);
		return;
	}

	auto lowestOrder = s.lowestOrder();
	++(s.size);
	s.order(1) = lowestOrder - 1;
	s.order(0) = lowestOrder - 2;

	auto leftRoot = leftChild(newSize, lowestOrder);
	auto rightRoot = rightChild(newSize);

	rectify(heap, leftRoot, 1, s, getKey);
	rectify(heap, rightRoot, 0, s, getKey);
}

template<class TIter, unsigned N, class TGetKey>
void smoothSort(TIter begin, size_t size, TGetKey getKey) {
	if (size <= 1) return;
	HeapShape<N> s;
	for (size_t i = 1; i <= size; ++i)
		heapPush(begin, i, size, s, getKey);
	for (size_t i = 1; i <= size; ++i)
		heapPop(begin, size - i, s, getKey);
}

}