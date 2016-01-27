#pragma once

// TODO: this code is GPL'd :( do a reimplementation

const size_t LNumbers[] =     { 1, 1, 3, 5, 9, 15, 25, 41, 67, 109, 177, 287, 465, 753, 1219,
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

template<class TIndex, class TIter> class LeonardoHeap {
	using Index = TIndex;
	using Key = typename Index::Key;
	using GetKey = typename Index::GetKey;

	GetKey getKey = GetKey{};

	TIter rows;
	TIter rowsEnd;

	size_t treeVector;	// Bitvector, where bit k marks the presence or absence of Lt_{k+m} in the heap, where m denotes <first_tree>
	unsigned short int firstTree;	// Order of rightmost tree in heap

public:
	LeonardoHeap(TIter rowsBegin, TIter rowsEnd) : rows(rowsBegin), rowsEnd(rowsEnd) {
		treeVector = 3;
		firstTree = 0;
		if (rowsEnd - rows > 1 && getKey(rows[0]) > getKey(rows[1]))
			swapRows(rows, rows + 1);
		for (TIter rowIter = rows; rowIter != rowsEnd; ++rowIter)
			insertionSort(rowIter);
	}

	void insertionSort(TIter rowIter)
		if ((treeVector & 1) && (treeVector & 2)) {
			treeVector = (treeVector >> 2) | 1;
			firstTree += 2;
		}
		else if (firstTree == 1) {
			treeVector = (treeVector << 1) | 1;
			firstTree = 0;
		}
		else {
			treeVector = (treeVector << (firstTree - 1)) | 1;
			firstTree = 1;
		}
		filter(rowIter, firstTree);
	}

	void dequeueMax(TIter rowIter) {
		if (firstTree >= 2) {
			treeVector = (treeVector << 1) ^ 3;
			firstTree -= 2;
			filter(rowIter - LNumbers[firstTree] - 1, firstTree + 1, false);
			// Restore the ascending root and max-heap properties of the exposed tree to the right
			treeVector = (treeVector << 1) | 1;	// w01 -> w011
			filter(rowIter - 1, firstTree, false);
		}
		else if (firstTree == 0) {
			treeVector >>= 1;
			firstTree = 1;
		}
		else {
			treeVector >>= 1;
			firstTree++;
			for (; firstTree < (rowsEnd - rows) + 1; firstTree++, treeVector >>= 1)
				if (treeVector & 1)
					break;
		}
	}

private:
	void heapify(size_t root, size_t order) {
		size_t comp, comp_order;
		while (true) {
			if (order <= 1) { break; }	// Break if root is a singleton node

										// Determine which of the two children is greater
			if (data[root - 1] > data[root - 1 - LNumbers[order - 2]]) {
				comp = root - 1;
				comp_order = order - 2;
			}
			else {
				comp = root - 1 - LNumbers[order - 2];
				comp_order = order - 1;
			}

			// Compare the root with the greater of the two children
			if (data[comp] > data[root]) { swap(root, comp); }
			else { break; }

			// shift the root downwards
			root = comp;
			order = comp_order;
		}
	}


	void filter(TIter rowIter, size_t order, bool testChildren = true) {
		auto current = rowIter;
		size_t orderCurrent = order;
		size_t sizeCurrent;
		size_t bitvectorMask = 2;
		while (true) {
			// Check that there is a tree to the left
			sizeCurrent = LNumbers[orderCurrent];
			if (sizeCurrent > current) break;

			// Determine whether root needs to be swapped with next tree to the left
			if (!(getKey(*(current - sizeCurrent)) > getKey(*current)))
				break;
			else if ((sizeCurrent == 1) || !testChildren) // Root of next tree greater than root of current tree
				swap(current, current - sizeCurrent);	// Singleton node, or current tree is already heapified
			else {	// Current tree not singleton node and child nodes must be compared
					// Root of next tree greater than both children of root of current tree
				if ((getKey(*(current - sizeCurrent)) > getKey(*(current - 1))) &&
					(getKey(*(current - sizeCurrent)) > getKey(*(current - 1 - LNumbers[orderCurrent - 2])))) {

					swap(current, current - sizeCurrent);
				}
				else
					break;
			}

			// Find the order of the next tree to the left
			orderCurrent++;
			for (; orderCurrent < (rowsEnd - rows) + 1; orderCurrent++, bitvectorMask <<= 1) { // For is used just for the purposes of safety. A while loop would work too.
				if (treeVector & bitvectorMask) {
					bitvectorMask <<= 1;
					break;
				}
			}
			// Shift the position marker leftwards
			current -= sizeCurrent;
		}
		heapify(current, orderCurrent);
	}

	void swap(TIter left, TIter right) {
		T tmp;
		tmp = data[element_1];
		data[element_1] = data[element_2];
		data[element_2] = tmp;
	}
};


// Sort an array, swapping elements in place
template<class T>
void smoothsort(T* data, size_t N) {
	LeonardoHeap<T> lh(data, N);
	for (size_t i = 1; i<N - 1; i++) { lh.dequeue_max(N - i); }
}
