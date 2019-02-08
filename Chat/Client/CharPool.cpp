/*
* MPool.cpp
*
*  Created on: 2019. 1. 17.
*      Author: choiis1207
*/

#include "CharPool.h"
// ������
CharPool::CharPool() {
	data = (char*)malloc(BLOCK_SIZE * 20000);
	DWORD i = 0;
	len = 20000;
	memset((char*)data, 0, BLOCK_SIZE * 20000);
	for (i = 0; i < 20000; i++) {
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

	if (poolQueue.empty()) { // �� �Ҵ� �ʿ��� ��� len(�ʱ� blocks��ŭ �߰�)
		char* nextP = new char[BLOCK_SIZE* len];
		cout << "CharPool More" << endl;
		memset((char*)nextP, 0, BLOCK_SIZE * len);
		for (DWORD j = 0; j < len; j++) {
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
	poolQueue.push((char*)freePoint);
}

