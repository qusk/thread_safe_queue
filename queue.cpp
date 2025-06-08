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
			void* allocated_value = malloc(item.value_size);
			if(!allocated_value) return { false, item }; // 할당 가능 여부 확인인
			free(queue->data[i].value);
			queue->data[i].value = allocated_value;
			memcpy(queue->data[i].value, item.value, item.value_size);
			queue->data[i].value_size = item.value_size;
			return { true, item };
		}
	}

	if (queue->size >= MAX_SIZE) {
		return { false, item };
	}

	void* allocated_value = malloc(item.value_size);
	if(!allocated_value) return { false, item };


	int i = queue->size++;
	queue->data[i].key = item.key;
	queue->data[i].value = allocated_value;
	queue->data[i].value_size = item.value_size;
	memcpy(queue->data[i].value, item.value, item.value_size);

	// 크기 비교 후 교환작업
	while(i > 0 && queue->data[parent(i)].key < queue->data[i].key) {
		Item temp = queue->data[i];
		queue->data[i] = queue->data[parent(i)];
		queue->data[parent(i)] = temp;

		i = parent(i);
	}

	lock.unlock();
	queue->cv.notify_one();

	return { true, item };
}

Reply dequeue(Queue* queue) {
	unique_lock<mutex> lock(queue->lock);

	queue->cv.wait(lock, [&]() { return queue->size > 0; });

	Item top = queue->data[0];
	
	// 최상위 노드 복제 후 해제
	void* top_copy = malloc(top.value_size);
	if(!top_copy) return { false, {0, nullptr, 0} };
	
	memcpy(top_copy, top.value, top.value_size);

	free(queue->data[0].value);
	queue->data[0] = queue->data[--queue->size];

	// maxheap 구조 유지를 위한 과정
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

	lock.unlock();


	return { true, {top.key, top_copy, top.value_size} };
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
