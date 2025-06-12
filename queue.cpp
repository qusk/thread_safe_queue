#include <iostream>
#include <cstdlib>
#include <cstring>
#include "queue.h"

using namespace std;

// 힙을 만들기 위한 간단한 함수들
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
	// Node 생성 Item으로 초기화
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
			if(!allocated_value) return { false, item }; // 메모리 할당 가능 여부 확인
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

	// 메모리 할당 가능 여부 확인
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

	// MAX_Heap 구조 유지를 위한 과정
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
	
  // 임시 보관 배열
	Item temp_items[MAX_SIZE];
	int temp_count = 0;
	
	// start와 end 사이의 값 찾아서 temp_items에 넣음
	{
		lock_guard<mutex> guard(queue->lock);
		for(int i = 0; i < queue->size; ++i) {
			Key key = queue->data[i].key;
			if(key >= start && key <= end) {
				if(temp_count >= MAX_SIZE) break;

				void* copy_data = malloc(queue->data[i].value_size);
				if(!copy_data) {
					for(int j = 0; j < temp_count; ++j) { free(temp_items[j].value); }
					return nullptr;
				}

				temp_items[temp_count].key = key;
				temp_items[temp_count].value_size = queue->data[i].value_size;
				temp_items[temp_count].value = copy_data;
				memcpy(copy_data, queue->data[i].value, queue->data[i].value_size);
				temp_count++;
			}
		}
	}
	
	Queue* new_queue = init();
	for(int i = 0; i < temp_count; ++i) {
		if(new_queue->size >= MAX_SIZE) {
			free(temp_items[i].value);
			continue;
		}

		int j = new_queue->size++;
		new_queue->data[j] = temp_items[i];

		// MAX_heap 구조 유지를 위한 과정
		while(j > 0 && new_queue->data[parent(j)].key < new_queue->data[j].key) {
			Item temp = new_queue->data[j];
			new_queue->data[j] = new_queue->data[parent(j)];
			new_queue->data[parent(j)] = temp;
			j = parent(j);
		}
	}

	return new_queue;
}
