#include <iostream>
#include "queue.h"

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
	

}

Reply dequeue(Queue* queue) {
	Reply reply = { false, NULL };
	return reply;
}

Queue* range(Queue* queue, Key start, Key end) {
	return NULL;
}
