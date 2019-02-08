/*
* MPool.cpp
*
*  Created on: 2019. 1. 17.
*      Author: choiis1207
*/

#include "MPool.h"
// ������
MPool::MPool() {
	data = (char*)malloc(sizeof(PER_IO_DATA)* 20000);
	DWORD i = 0;
	len = 20000;
	memset((char*)data, 0, sizeof(PER_IO_DATA)* 20000);
	for (i = 0; i < 20000; i++) {
		poolQueue.push(data + (sizeof(PER_IO_DATA)* i));
	}
}
// Singleton Instance
MPool* MPool::instance = nullptr;

MPool::~MPool() {
	char *popPoint;
	while (!poolQueue.empty()) {
		poolQueue.try_pop(popPoint);
	}
	free(data);
}
// �޸�Ǯ �Ҵ�
LPPER_IO_DATA MPool::Malloc() {
	if (poolQueue.empty()) { // �� �Ҵ� �ʿ��� ��� len(�ʱ� blocks��ŭ �߰�)
		char* nextP = new char[sizeof(PER_IO_DATA)* len];
		cout << "MPool More" << endl;
		memset((char*)nextP, 0, sizeof(PER_IO_DATA)* len);
		for (DWORD j = 0; j < len; j++) {
			poolQueue.push(nextP + (sizeof(PER_IO_DATA)* j));
		}
	}
	char* pointer;
	poolQueue.try_pop(pointer);
	return (LPPER_IO_DATA)pointer;
}
// �޸�Ǯ ��ȯ
void MPool::Free(LPPER_IO_DATA freePoint) { // ��ȯ�� �������� idx�� ���󺹱�
	memset((char*)freePoint, 0, sizeof(PER_IO_DATA));
	poolQueue.push((char*)freePoint);
}

