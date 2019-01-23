/*
 * MPool.cpp
 *
 *  Created on: 2019. 1. 17.
 *      Author: choiis1207
 */

#include "MPool.h"
// ������
MPool::MPool() {
	data = (char*) malloc(sizeof(PER_IO_DATA) * 1000);
	DWORD i = 0;
	len = 1000;
	InitializeCriticalSectionAndSpinCount(&cs, 2000);
	for (i = 0; i < 1000; i++) {
		poolQueue.push(data + (sizeof(PER_IO_DATA) * i));
	}
}
// Singleton Instance
MPool* MPool::instance = nullptr;

MPool::~MPool() {
	DeleteCriticalSection(&cs);
	while (!poolQueue.empty()) {
		poolQueue.pop();
	}
	free(data);
}
// �޸�Ǯ �Ҵ�
LPPER_IO_DATA MPool::Malloc() {
	EnterCriticalSection(&cs);
	if (poolQueue.empty()) { // �� �Ҵ� �ʿ��� ��� len(�ʱ� blocks��ŭ �߰�)
		data = (char*) realloc(data, (len + len) * sizeof(PER_IO_DATA));
		DWORD i = 0;
		for (i = 0; i < len; i++) {
			poolQueue.push(data + (sizeof(PER_IO_DATA) * (len + i)));
		}
		len += len;
	}
	LPPER_IO_DATA pointer = (LPPER_IO_DATA) poolQueue.front();
	poolQueue.pop();
	LeaveCriticalSection(&cs);
	return pointer;
}
// �޸�Ǯ ��ȯ
void MPool::Free(LPPER_IO_DATA freePoint) { // ��ȯ�� �������� idx�� ���󺹱�
	EnterCriticalSection(&cs);
	poolQueue.push((char*) freePoint);
	LeaveCriticalSection(&cs);
}

