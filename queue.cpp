#include <iostream>
#include "queue.h"

using namespace std;

inline int parent(int i) { return (i - 1) / 2; }
inline int left(int i) { return 2 * i + 1; }
inline int right(int i) { return 2 * i + 2; }

Queue* init(void) {
	Queue* queue = new Queue();
	return queue;
}


void release(Queue* queue) {
	delete queue;
}


Node* nalloc(Item item) {
	// Node 생성 Item으로로 초기화
	Node* node = new Node();
	node->item = item;
	node->next = nullptr;
	
	return node;
}


void nfree(Node* node) {
	delete node;
}


Node* nclone(Node* node) {
	return nalloc(node->item);
}


Reply enqueue(Queue* queue, Item item) {
	lock_guard<mutex> lock(queue->lock);

	if (queue->size >= MAX_SIZE) {
		return { false, item };
	}

	int i = queue->size++;
	queue->data[i] = item;
	while(i > 0 && queue->data[parent(i)].key < queue->data[i].key) {
		// 부모 노드와 비교하여 우선순위가 낮으면 교환
		swap(queue->data[i], queue->data[parent(i)]);
		i = parent(i);
	}

	return { true, item };
}

Reply dequeue(Queue* queue) {
	lock_guard<mutex> lock(queue->lock);

	if(queue->size == 0) {
		return { false, {0, nullptr} };
	}

	Item top = queue->data[0];
	queue->data[0] = queue->data[--queue->size];

	int i = 0;
	while(true) {
		int largest = i;
		int leftNode = left(i);
		int rightNode = right(i);
		if(leftNode < queue->size && queue->data[leftNode].key > queue->data[largest].key) {
			largest = leftNode;
		}
		if(rightNode < queue->size && queue->data[rightNode].key > queue->data[largest].key) {
			largest = rightNode;
		}
		if(largest == i) {
			break; // 더 이상 교환할 필요 없음
		}
		swap(queue->data[i], queue->data[largest]);
		i = largest;
	}

	return { true, top };
}

Queue* range(Queue* queue, Key start, Key end) {
	return NULL;
}
