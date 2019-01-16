//============================================================================
// Name        : QueuePool.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <cstdlib>
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <queue>

using namespace std;

#define BUF_SIZE 500

// �񵿱� ��ſ� �ʿ��� ����ü
typedef struct { // buffer info
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	char buffer[BUF_SIZE];  // ���ۿ� PACKET_DATA�� �� ���̹Ƿ� ũ�� ����
	int serverMode;
	DWORD recvByte; // ���ݱ��� ���� ����Ʈ�� �˷���, ���ݱ��� ���� ���
	DWORD totByte; //��ü ���� ����Ʈ�� �˷��� , �� ���� ���
	int bodySize; // ��Ŷ�� body ������
	char *recvBuffer;
} PER_IO_DATA, *LPPER_IO_DATA;

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

	LPPER_IO_DATA malloc() {
		LPPER_IO_DATA pointer = (LPPER_IO_DATA) poolQueue.front();
		poolQueue.pop();
		return pointer;
	}

	void free(LPPER_IO_DATA freePoint) {
		poolQueue.push((char*) freePoint);
	}
	~QueuePool(){
		delete data;
	}
};

int main() {

	QueuePool pool(200);

	cout << "�Ҵ� �Ϸ�" << endl;

	clock_t begin, end;
	begin = clock();
	for (int i = 0; i < 10000000; i++) {
		LPPER_IO_DATA p = pool.malloc();
		pool.free(p);
	}
	end = clock();
	cout << (end - begin) << endl;
	clock_t begin2, end2;
	begin2 = clock();
	for (int i = 0; i < 10000000; i++) {
		LPPER_IO_DATA p = new PER_IO_DATA;
		delete p;
	}
	end2 = clock();
	cout << (end2 - begin2) << endl;

	return 0;
}
