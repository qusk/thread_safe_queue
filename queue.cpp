#include <iostream>
#include <cstdlib>
#include <cstring>
#include "queue.h"

using namespace std;

inline int parent(int i) { return (i - 1) / 2; }
inline int left(int i) { return 2 * i + 1; }
inline int right(int i) { return 2 * i + 2; }

Queue* init(void) {
	Queue* queue = new Queue;
	queue->size = 0;
	return queue;
}


void release(Queue* queue) {
	lock_guard<mutex> guard(queue->lock);
	for(int i = 0; i < queue->size; ++i) {
		if(queue->data[i].value)
			free(queue->data[i].value);
			queue->data[i].value = nullptr;
	}

	delete queue;
}


Node* nalloc(Item item) {
	// Node 생성 Item으로로 초기화
	Node* node = new Node();
	node->item.key = item.key;
	node->item.value = item.value;
	node->item.value_size = item.value_size;
	node->item.value = malloc(item.value_size);
	memcpy(node->item.value, item.value, item.value_size);
	node->next = nullptr;
	return node;
}


void nfree(Node* node) {
	free(node->item.value);
	delete node;
}


Node* nclone(Node* node) {
	return nalloc(node->item);
}


Reply enqueue(Queue* queue, Item item) {
	unique_lock<mutex> lock(queue->lock);

	// 중복된 key가 들어오면 새로운 key값으로 덮어쓰기
	for(int i = 0; i < queue->size; ++i) {
		if(queue->data[i].key == item.key) {
			free(queue->data[i].value);
			queue->data[i].value = malloc(item.value_size);
			memcpy(queue->data[i].value, item.value, item.value_size);
			queue->data[i].value_size = item.value_size;
			return { true, item };
		}
	}

	if (queue->size >= MAX_SIZE) {
		return { false, item };
	}

	int i = queue->size++;
	queue->data[i].key = item.key;
	queue->data[i].value = malloc(item.value_size);
	queue->data[i].value_size = item.value_size;
	memcpy(queue->data[i].value, item.value, item.value_size);

	while(i > 0 && queue->data[parent(i)].key < queue->data[i].key) {
		// 부모 노드와 비교하여 우선순위가 낮으면 교환
		Item temp = queue->data[i];
		queue->data[i] = queue->data[parent(i)];
		queue->data[parent(i)] = temp;

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
	Item top_copy;
	top_copy.key = top.key;
	top_copy.value_size = top.value_size;
	top_copy.value = malloc(top_copy.value_size);
	memcpy(top_copy.value, top.value, top_copy.value_size);

	free(queue->data[0].value);
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
		Item temp = queue->data[i];
		queue->data[i] = queue->data[largest];
		queue->data[largest] = temp;

		i = largest;
	}

	return { true, top_copy };
}

Queue* range(Queue* queue, Key start, Key end) {
	lock_guard<mutex> guard(queue->lock);

	Queue* newQueue = new Queue;
	newQueue->size = 0;

	for(int i = 0; i < queue->size; ++i) {
		if(queue->data[i].key >= start && queue->data[i].key <= end) {
			if(newQueue->size >= MAX_SIZE) {
				break;
			}

			int index = newQueue->size++;
			newQueue->data[index].key = queue->data[i].key;
			newQueue->data[index].value_size = queue->data[i].value_size;

			newQueue->data[index].value = malloc(queue->data[i].value_size);
			memcpy(newQueue->data[index].value, queue->data[i].value, queue->data[i].value_size);
		}
	}

	return newQueue;
}
