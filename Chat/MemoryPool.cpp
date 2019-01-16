//============================================================================
// Name        : MemoryPool.cpp
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

class MemoryPool {
private:
	unsigned char* data;
	unsigned char* nowPoint;
	DWORD num;
	DWORD idx;
public:
	MemoryPool(DWORD blocks) {

		data = new unsigned char[blocks * sizeof(PER_IO_DATA)];
		nowPoint = data; // ù ��ȯ ���� �ּҴ� �޸� Ǯ�� ù �ּ�
		idx = 0;
		num = blocks;
		unsigned char *po = data;
		for (DWORD i = 0; i < blocks; i++) {
			*po = i;
			po += sizeof(PER_IO_DATA);
		}
	}

	LPPER_IO_DATA malloc() {

		unsigned char* allockPoint = &data[sizeof(PER_IO_DATA) * idx];
		idx = *allockPoint;
		num--;

		return (LPPER_IO_DATA) allockPoint;
	}

	void free(LPPER_IO_DATA p) {

		unsigned char* freePoint = (unsigned char*) p;

		*freePoint = idx;
		idx = (freePoint - data) / sizeof(PER_IO_DATA);
		num++;
	}
};

int main() {

	MemoryPool pool(50);
	clock_t begin, end;

	begin = clock();
	for (int i = 0; i < 1000000; i++) {
		LPPER_IO_DATA p = pool.malloc();
		pool.free(p);
	}
	end = clock();

	cout << (end - begin) << endl;

	clock_t begin2, end2;
	begin2 = clock();
	for (int i = 0; i < 1000000; i++) {
		LPPER_IO_DATA p = new PER_IO_DATA;
		delete p;
	}
	end2 = clock();
	cout << (end2 - begin2) << endl;
	return 0;
}
