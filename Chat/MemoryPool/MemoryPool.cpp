//============================================================================
// Name        : MemoryPool.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <stdlib.h>
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <queue>
#include <stack>

using namespace std;
#define BLOCK_SIZE 256

#define BUF_SIZE 100

// �񵿱� ��ſ� �ʿ��� ����ü
typedef struct { // buffer info
	char buffer[BUF_SIZE];  // ���ۿ� PACKET_DATA�� �� ���̹Ƿ� ũ�� ����
} PER_IO_DATA, *LPPER_IO_DATA;

class ListPool {
private:
	unsigned char *data;
	unsigned char *first;
	unsigned char *last;
public:
	ListPool(int block) {
		data = new unsigned char[sizeof(PER_IO_DATA) * block];
		first = data;
		last = &data[sizeof(PER_IO_DATA) * (block - 1)];
		unsigned char** pointer = (unsigned char**) (first);
		unsigned char* next = data;
		for (int i = 0; i < block; i++) {
			next += sizeof(PER_IO_DATA);
			*pointer = next;
			pointer = (unsigned char**) next;
		}
		*pointer = 0;
	}
	~ListPool() {
		first = nullptr;
		last = nullptr;
		delete data;
	}
	LPPER_IO_DATA malloc() {
		unsigned char* alloc = first;
		first = *reinterpret_cast<unsigned char**>(alloc);
		return (LPPER_IO_DATA) alloc;
	}

	void free(LPPER_IO_DATA p) {
		unsigned char** lastPoint = (unsigned char**) (last);
		*lastPoint = (unsigned char*) p;
		last = *reinterpret_cast<unsigned char**>(lastPoint);
	}
};

class QueuePool {
private:
	char* data;
	queue<char*> poolQueue;
public:
	QueuePool(DWORD blocks) {
		data = new char[sizeof(PER_IO_DATA) * blocks];
		DWORD i = 0;
		for (i = 0; i < blocks; i++) {
			poolQueue.push(data + (sizeof(PER_IO_DATA) * i));
		}
	}
	~QueuePool() {
		while (!poolQueue.empty()) {
			poolQueue.pop();
		}
		delete data;
	}
	LPPER_IO_DATA malloc() {
		LPPER_IO_DATA pointer = (LPPER_IO_DATA) poolQueue.front();
		poolQueue.pop();
		return pointer;
	}

	void free(LPPER_IO_DATA freePoint) {
		poolQueue.push((char*) freePoint);
	}

};

class StackPool {
private:
	char* data;
	stack<int> st; // ��ȯ���� idx�� ����
public:
	StackPool(DWORD blocks) {
		data = new char[blocks * sizeof(PER_IO_DATA)]; // size ���ϱ� ��ϼ� ��ŭ �Ҵ�
		for (int i = blocks - 1; i >= 0; i--) {
			st.push(i);
		}
	}
	~StackPool() {
		while (!st.empty()) {
			st.pop();
		}
		delete data;
	}
	LPPER_IO_DATA malloc() {
		if (st.empty()) {
			return NULL;
		}
		int idx = st.top();
		st.pop();

		return (LPPER_IO_DATA) (data + (idx * (sizeof(PER_IO_DATA))));
	}
	void free(LPPER_IO_DATA freePoint) {
		DWORD returnIdx = ((((char*) freePoint) - data) / sizeof(PER_IO_DATA));
		st.push(returnIdx);
	}
};

class ArrayPool {
private:
	char* data;
	bool* arr;
	DWORD cnt;
	DWORD idx;
	DWORD len;
	CRITICAL_SECTION cs; // �޸�Ǯ ����ȭ ũ��Ƽ�ü���
public:
	ArrayPool(DWORD size) {
		data = (char*) malloc(size * sizeof(PER_IO_DATA));
		arr = (bool*) malloc(size);
		cnt = size;
		idx = 0;
		len = size;
		memset(arr, 0, size);
		InitializeCriticalSection(&cs);
	}
	~ArrayPool() {
		cnt = 0;
		idx = 0;
		free(data);
		free(arr);
		DeleteCriticalSection(&cs);
	}
	LPPER_IO_DATA Malloc() {
		EnterCriticalSection(&cs);

		if (cnt == 0) { // 5 ��� ���Ҵ�
			len += 5;
			data = (char*) realloc(data, (len + 5) * sizeof(PER_IO_DATA));
			arr = (bool*) realloc(arr, (len + 5) * sizeof(bool));
			cnt += len;
			memset(arr + 1, 0, 5);
		}

		while (arr[idx]) {
			idx++;
			if (idx == cnt) {
				idx = 0;
			}
		}

		arr[idx] = true; // �Ҵ翩�� true
		idx++;
		cnt--; // ��üī��Ʈ ���̳ʽ�
		char* alloc;
		if (idx == cnt) { // �ε����� ������ ��°�϶�
			idx = 0;
			alloc = (data + ((cnt - 1) * (sizeof(PER_IO_DATA))));
		} else {
			alloc = (data + ((idx - 1) * (sizeof(PER_IO_DATA))));
		}
		LeaveCriticalSection(&cs);
		return (LPPER_IO_DATA) alloc;
	}

	void Free(LPPER_IO_DATA freePoint) {
		DWORD returnIdx = ((((char*) freePoint) - data) / sizeof(PER_IO_DATA));
		EnterCriticalSection(&cs);
		memset(freePoint, 0, sizeof(PER_IO_DATA));
		cnt++; // ��üī��Ʈ ���󺹱�
		idx = returnIdx; // ��ȯ�Ȱ��� �ٷ� ��ȯ
		arr[returnIdx] = false;
		LeaveCriticalSection(&cs);
	}
};

class CharPool {
private:
	char* data; // �����Ҵ� ��� �ּ�
	bool* arr; // block���� �Ҵ� ����
	DWORD cnt; // ��ü ��ϼ�
	DWORD idx; // ��ȯ ��� idx
	DWORD len;
	CRITICAL_SECTION cs; // �޸�Ǯ ����ȭ ũ��Ƽ�ü���
public:
	CharPool(DWORD size) {
		data = (char*) malloc(size * BLOCK_SIZE);
		arr = (bool*) malloc(size);
		cnt = size;
		idx = 0;
		len = size;
		memset(arr, 0, size);
		InitializeCriticalSection(&cs);
	}
	~CharPool() {
		cnt = 0;
		idx = 0;
		free(data);
		free(arr);
		DeleteCriticalSection(&cs);
	}
	char* Malloc() {
		EnterCriticalSection(&cs);

		if (cnt == 0) { // 5 ��� ���Ҵ�
			len += 5;
			data = (char*) realloc(data, (len + 5) * BLOCK_SIZE);
			arr = (bool*) realloc(arr, (len + 5) * sizeof(bool));
			cnt += len;
			memset(arr + 1, 0, 5);
		}

		while (arr[idx]) {
			idx++;
			if (idx == cnt) {
				idx = 0;
			}
		}

		arr[idx] = true; // �Ҵ翩�� true
		idx++;
		cnt--; // ��üī��Ʈ ���̳ʽ�
		char* alloc;
		if (idx == cnt) { // �ε����� ������ ��°�϶�
			idx = 0;
			alloc = (data + ((cnt - 1) * BLOCK_SIZE));
		} else {
			alloc = (data + ((idx - 1) * BLOCK_SIZE));
		}
		LeaveCriticalSection(&cs);
		return alloc;
	}

	void Free(char* freePoint) {
		DWORD returnIdx = ((freePoint - data) / BLOCK_SIZE);
		EnterCriticalSection(&cs);
		memset(freePoint, 0, BLOCK_SIZE);
		cnt++; // ��üī��Ʈ ���󺹱�
		idx = returnIdx; // ��ȯ�Ȱ��� �ٷ� ��ȯ
		arr[returnIdx] = false;
		LeaveCriticalSection(&cs);
	}
};

int main() {

//	clock_t begin, end;
//	ArrayPool* ap = new ArrayPool(3);
//
//	LPPER_IO_DATA p1 = ap->Malloc();
//	printf("%u\n", p1);
//	LPPER_IO_DATA p2 = ap->Malloc();
//	printf("%u\n", p2);
//	LPPER_IO_DATA p3 = ap->Malloc();
//	printf("%u\n", p3);
//	ap->Free(p1);
//	ap->Free(p2);
//	LPPER_IO_DATA p4 = ap->Malloc();
//	printf("%u\n", p4);
//	LPPER_IO_DATA p5 = ap->Malloc();
//	printf("%u\n", p5);

//	cout << "ArrayPool" << endl;
//	ListPool* lp = new ListPool(100);
//	cout << "ListPool" << endl;
//	QueuePool* qp = new QueuePool(100);
//	cout << "QueuePool" << endl;
//	StackPool* sp = new StackPool(100);
//	cout << "StackPool" << endl;
//	cout << "Start" << endl;
//	begin = clock();
//	for (int i = 0; i < 1000000; i++) {
//		LPPER_IO_DATA p = ap->Malloc();
//		ap->Free(p);
//	}
//	end = clock();
//	cout << (end - begin) << endl;
//
//	begin = clock();
//	for (int i = 0; i < 1000000; i++) {
//		LPPER_IO_DATA p = lp->malloc();
//		lp->free(p);
//	}
//	end = clock();
//	cout << (end - begin) << endl;
//
//	begin = clock();
//	for (int i = 0; i < 1000000; i++) {
//		LPPER_IO_DATA p = qp->malloc();
//		qp->free(p);
//	}
//	end = clock();
//	cout << (end - begin) << endl;
//
//	begin = clock();
//	for (int i = 0; i < 1000000; i++) {
//		LPPER_IO_DATA p = sp->malloc();
//		sp->free(p);
//	}
//	end = clock();
//	cout << (end - begin) << endl;
//	string str;
//	cin >> str;

	return 0;
}
