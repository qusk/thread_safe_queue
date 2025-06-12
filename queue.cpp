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
		lock_guard<mutex> lock(queue->lock);

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

	void* value_copy = malloc(item.value_size);
	if(!value_copy) return {false, item};
	memcpy(value_copy, item.value, item.value_size);

	Node* node = new Node;
	node->item.key = item.key;
	node->item.value = value_copy;
	node->item.value_size = item.value_size;
	node->next = nullptr;

	{
		lock_guard<mutex> lock(queue->lock);

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
	
	{
		lock_guard<mutex> lock(queue->lock);
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
	Queue* new_queue = new Queue;
	new_queue->head = nullptr;
	new_queue->tail = nullptr;
	
	lock_guard<mutex> lock(queue->lock);
	Node* current = queue->head;

	while(current) {
		Key key = current->item.key;
		if(key >= start && key <= end) {
			void* value_copy = malloc(current->item.value_size);
			if(!value_copy) break;
			memcpy(value_copy, current->item.value, current->item.value_size);

			Node* node = new Node;
			node->item.key = key;
			node->item.value = value_copy;
			node->item.value_size = current->item.value_size;
			node->next = nullptr;

			Node* prev = nullptr;
			Node* new_current = new_queue->head;
			while(new_current && new_current->item.key >= key) {
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
		current = current->next;
	}
	return new_queue;
}
