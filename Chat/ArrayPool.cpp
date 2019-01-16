//============================================================================
// Name        : SimplePool.cpp
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

class SimplePool {
private:
	char* data;
	bool* arr;
	DWORD cnt;
	DWORD idx;
public:
	SimplePool(DWORD size) {
		data = new char[size * sizeof(PER_IO_DATA)];
		arr = new bool[size];
		cnt = size;
		idx = 0;
		memset(arr, 0, size);

	}

	LPPER_IO_DATA malloc() {

		while (arr[idx]) {
			idx++;
			if (idx == cnt) {
				idx = 0;
			}
		}

		arr[idx] = true;
		idx++;
		if (idx == cnt) {
			idx = 0;
			return (LPPER_IO_DATA) (data + ((cnt - 1) * (sizeof(PER_IO_DATA))));
		} else {
			return (LPPER_IO_DATA) (data + ((idx - 1) * (sizeof(PER_IO_DATA))));
		}

	}

	void free(LPPER_IO_DATA freePoint) {
		DWORD returnIdx = ((((char*) freePoint) - data) / sizeof(PER_IO_DATA));
		arr[returnIdx] = false;
	}
};

int main() {

	SimplePool sp(3);

	clock_t begin, end;

	begin = clock();
	for (int i = 0; i < 1000000; i++) {
		LPPER_IO_DATA p = sp.malloc();
		sp.free(p);
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
