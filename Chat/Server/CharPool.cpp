/*
 * MPool.cpp
 *
 *  Created on: 2019. 1. 17.
 *      Author: choiis1207
 */

#include "CharPool.h"
// ������
CharPool::CharPool() {
	data = (char*)malloc(BLOCK_SIZE * 30000);
	DWORD i = 0;
	len = 30000;
	memset((char*)data, 0, BLOCK_SIZE * 30000);
	for (i = 0; i < 30000; i++) {
		poolQueue.push(data + (BLOCK_SIZE * i));
	}
}
// Singleton Instance
CharPool* CharPool::instance = nullptr;

CharPool::~CharPool() {
	char *popPoint;
	while (!poolQueue.empty()) {
		poolQueue.try_pop(popPoint);
	}
	free(data);
}
// �޸�Ǯ �Ҵ�
char* CharPool::Malloc() {
	
	if (poolQueue.empty()) { // �� �Ҵ� �ʿ��� ��� 2000��ŭ �߰�
		char* nextP = new char[BLOCK_SIZE * 2000];
		cout << "CharPool More" << endl;
		memset((char*)nextP, 0, BLOCK_SIZE * 2000);
		for (DWORD j = 0; j < 2000; j++) {
			poolQueue.push(nextP + (BLOCK_SIZE* j));
		}
	}
	char* pointer;
	poolQueue.try_pop(pointer);
	
	return pointer;
}
// �޸�Ǯ ��ȯ
void CharPool::Free(char* freePoint) { // ��ȯ�� �������� idx�� ���󺹱�
	memset((char*)freePoint, 0, sizeof(BLOCK_SIZE));
	poolQueue.push((char*) freePoint);
}

