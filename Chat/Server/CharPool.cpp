/*
 * MPool.cpp
 *
 *  Created on: 2019. 1. 17.
 *      Author: choiis1207
 */

#include "CharPool.h"
// ������
CharPool::CharPool() {
	data = (char*)malloc(BLOCK_SIZE * 2000);
	DWORD i = 0;
	len = 2000;
	memset((char*)data, 0, BLOCK_SIZE* 2000);
	InitializeCriticalSectionAndSpinCount(&cs, 2000);
	for (i = 0; i < 2000; i++) {
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
		char* nextP = new char[BLOCK_SIZE* len];
		memset((char*)nextP, 0, BLOCK_SIZE * len);
		for (DWORD j = 0; j < len; j++) {
			poolQueue.push(nextP + (BLOCK_SIZE* j));
		}
	}
	char* pointer = poolQueue.front();
	poolQueue.pop();
	LeaveCriticalSection(&cs);
	return pointer;
}
// �޸�Ǯ ��ȯ
void CharPool::Free(char* freePoint) { // ��ȯ�� �������� idx�� ���󺹱�
	EnterCriticalSection(&cs);
	memset((char*)freePoint, 0, sizeof(BLOCK_SIZE));
	poolQueue.push((char*) freePoint);
	LeaveCriticalSection(&cs);
}

