/*
 * MPool.cpp
 *
 *  Created on: 2019. 1. 17.
 *      Author: choiis1207
 */

#include "MPool.h"
// ������
MPool::MPool() {
	data = (char*)malloc(sizeof(PER_IO_DATA)* 50000);
	DWORD i = 0;
	len = 50000;
	memset((char*)data, 0, sizeof(PER_IO_DATA)* 50000);
	for (i = 0; i < 50000; i++) {
		poolQueue.push(data + (sizeof(PER_IO_DATA) * i));
	}
}
// Singleton Instance
MPool* MPool::instance = nullptr;

MPool::~MPool() {
	while (!poolQueue.empty()) {
		poolQueue.top();
	}
	free(data);
}
// �޸�Ǯ �Ҵ�
LPPER_IO_DATA MPool::Malloc() {
	if (poolQueue.empty()) { // �� �Ҵ� �ʿ��� ��� 2000��ŭ �߰�
		char* nextP = new char[sizeof(PER_IO_DATA)* 2000];
		cout << "MPool More" << endl;
		memset((char*)nextP, 0, sizeof(PER_IO_DATA)* 2000);
		for (DWORD j = 0; j < 2000; j++) {
			poolQueue.push(nextP + (sizeof(PER_IO_DATA)* j));
		}
	}
	return (LPPER_IO_DATA) poolQueue.top();
}
// �޸�Ǯ ��ȯ
void MPool::Free(LPPER_IO_DATA freePoint) { // ��ȯ�� �������� idx�� ���󺹱�
	memset((char*)freePoint, 0, sizeof(PER_IO_DATA));
	poolQueue.push((char*) freePoint);
}

