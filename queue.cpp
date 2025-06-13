#include <iostream>
#include <cstdlib>
#include <cstring>
#include "queue.h"

using namespace std;

Queue* init(void) {
	Queue* queue = new Queue;
	queue->head = nullptr;
	queue->tail = nullptr;

	return queue;
}


void release(Queue* queue) {
	{
		lock_guard<mutex> guard(queue->lock);

		Node* current = queue->head;
		while(current) {
			Node* next = current->next;
			free(current->item.value);
			delete current;
			current = next;
		}
	}
	delete queue;
}


Node* nalloc(Item item) {
	// Node 생성 Item으로 초기화
	Node* node = new Node();
	node->item.key = item.key;
	node->item.value_size = item.value_size;

	node->item.value = malloc(item.value_size);
	if(!node->item.value) {
		delete node;
		return nullptr;
	}

	memcpy(node->item.value, item.value, item.value_size);
	node->next = nullptr;

	return node;
}


void nfree(Node* node) {
	if(!node) return;
	if(node->item.value) free(node->item.value);
	delete node;
}


Node* nclone(Node* node) {
	if(!node) return nullptr;
	return nalloc(node->item);
}


Reply enqueue(Queue* queue, Item item) {
	// 깊은 복사
	void* value_copy = malloc(item.value_size);
	if(!value_copy) return {false, item};
	memcpy(value_copy, item.value, item.value_size);

	// 노드 생성
	Node* node = new Node;
	node->item.key = item.key;
	node->item.value = value_copy;
	node->item.value_size = item.value_size;
	node->next = nullptr;

	// 리스트 index 추가 및 정렬, key값 중복 처리 
	{
		lock_guard<mutex> guard(queue->lock);

		Node* prev = nullptr;
		Node* current = queue->head;

		while(current) {
			if(current->item.key == item.key) {
				free(current->item.value);
				current->item.value = value_copy;
				current->item.value_size = item.value_size;
				delete node;
				return {true, item};
			}
			if(current->item.key < item.key) break;
			prev = current;
			current = current->next;
		}

		if(!prev) {
			node->next = queue->head;
			queue->head = node;
			if(!queue->tail) queue->tail = node;
		} else {
			node->next = prev->next;
			prev->next = node;
			if(!node->next) queue->tail = node;
		}
	}

	return {true, node->item};
}

Reply dequeue(Queue* queue) {
	Node* node;
	
	// lock 안에서 head제거
	{
		lock_guard<mutex> guard(queue->lock);
		if(!queue->head) return {false, {0, nullptr, 0}};

		node = queue->head;
		queue->head = node->next;
		if(queue->head) queue->tail = node;
	}

	Item return_val;
	return_val.key = node->item.key;
	return_val.value_size = node->item.value_size;
	return_val.value = malloc(return_val.value_size);
	memcpy(return_val.value, node->item.value, return_val.value_size);

	free(node->item.value);
	delete node;

	return {true, return_val};
}

Queue* range(Queue* queue, Key start, Key end) {

	// 원본 큐의 최소한의 정보만 담아두기 위한 Temp구조체
	struct Temp { 
		Key key;
		int size;
		void* ptr;
	};

	// snapshot배열을 할당하기 위한 노드 몇개인지 카운트(malloc으로 할당해야해서 몇 개가 있는지 알기 위해)
	int count = 0;
	{
		lock_guard<mutex> guard(queue->lock);
		for(Node* current = queue->head; current; current = current->next) {
			if(current->item.key >= start && current->item.key <= end) {
				++count;
			}
		}
	}

	// temp 메모리 할당
	Temp* temp = (Temp*)malloc(sizeof(Temp) * count);
	if(!temp) return nullptr;


	// snapshot배열 안에 범위 안에 들어있는 해당 노드의 key, value_size, value값 복사
	{
		lock_guard<mutex> guard(queue->lock);
		int index = 0;
		for(Node* current = queue->head; current; current = current->next) {
			if(current->item.key >= start && current->item.key <= end) {
				temp[index++] = {current->item.key, current->item.value_size, current->item.value};
			}
		}
	}


	Queue* new_queue = init();
	for(int i = 0; i < count; ++i) {
		void* temp_size = malloc(temp[i].size);
		if(!temp_size) break;
		memcpy(temp_size, temp[i].ptr, temp[i].size);

		Node* node = new Node;
		node->item.key = temp[i].key;
		node->item.value_size = temp[i].size;
		node->item.value = temp_size;
		node->next = nullptr;

		Node* prev = nullptr;
		Node* new_current = new_queue->head;

		// 정렬(내림차순)
		while(new_current && new_current->item.key >= node->item.key) {
			prev = new_current;
			new_current = new_current->next;
		}

		if(!prev) {
			node->next = new_queue->head;
			new_queue->head = node;
			if(!new_queue->tail) new_queue->tail = node;
		} else {
			node->next = prev->next;
			prev->next = node;
			if(!node->next) new_queue->tail = node;
		}
	}
	free(temp);
	return new_queue;
}
