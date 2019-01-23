/*
 * MPool.cpp
 *
 *  Created on: 2019. 1. 17.
 *      Author: choiis1207
 */

#include "CharPool.h"
// ������
CharPool::CharPool() {
	data = (char*) malloc(BLOCK_SIZE * 1000);
	DWORD i = 0;
	len = 1000;
	InitializeCriticalSectionAndSpinCount(&cs, 2000);
	for (i = 0; i < 1000; i++) {
		poolQueue.push(data + (BLOCK_SIZE * i));
	}

}
// Singleton Instance
CharPool* CharPool::instance = nullptr;

CharPool::~CharPool() {
	DeleteCriticalSection(&cs);
	while (!poolQueue.empty()) {
		poolQueue.pop();
	}
	free(data);
}
// �޸�Ǯ �Ҵ�
char* CharPool::Malloc() {
	EnterCriticalSection(&cs);
	if (poolQueue.empty()) { // �� �Ҵ� �ʿ��� ��� len(�ʱ� blocks��ŭ �߰�)
		data = (char*) realloc(data, (len + len) * BLOCK_SIZE);
		DWORD i = 0;
		for (i = 0; i < len; i++) {
			poolQueue.push(data + (BLOCK_SIZE * (len + i)));
		}
		len += len;
	}
	char* pointer = poolQueue.front();
	poolQueue.pop();
	LeaveCriticalSection(&cs);
	return pointer;
}
// �޸�Ǯ ��ȯ
void CharPool::Free(char* freePoint) { // ��ȯ�� �������� idx�� ���󺹱�
	EnterCriticalSection(&cs);
	poolQueue.push((char*) freePoint);
	LeaveCriticalSection(&cs);
}

