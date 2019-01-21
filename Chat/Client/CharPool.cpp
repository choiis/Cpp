/*
 * MPool.cpp
 *
 *  Created on: 2019. 1. 17.
 *      Author: choiis1207
 */

#include "CharPool.h"
// ������
CharPool::CharPool() {
	data = new char[1000 * BLOCK_SIZE]; // size ���ϱ� ��ϼ� ��ŭ �Ҵ�
	arr = new bool[1000]; // idx��° �޸� Ǯ�� �Ҵ� ���θ� ������
	cnt = 1000;
	idx = 0;
	memset(this->arr, 0, 1000);
}
// Singleton Instance
CharPool* CharPool::instance = nullptr;

CharPool::~CharPool() {
	delete data;
	delete arr;
}
// �޸�Ǯ �Ҵ�
char* CharPool::malloc() {
	// �Ҵ��� �ȵ� ����Ҹ� ã�´�
	while (arr[idx]) {
		idx++;
		if (idx == cnt) {
			idx = 0;
		}
	}

	arr[idx] = true; // �Ҵ翩�� true
	idx++;
	if (idx == cnt) { // �ε����� ������ ��°�϶�
		idx = 0;
		return (data + ((cnt - 1) * BLOCK_SIZE));
	} else {
		return (data + ((idx - 1) * BLOCK_SIZE));
	}

}
// �޸�Ǯ ��ȯ
void CharPool::free(char* freePoint) { // ��ȯ�� �������� idx�� ���󺹱�
	DWORD returnIdx = ((((char*) freePoint) - data) / BLOCK_SIZE);
	idx = returnIdx;
	// ��ȯ�� idx�� �Ҵ翩�� No
	arr[returnIdx] = false;
}

