/*
 * MPool.cpp
 *
 *  Created on: 2019. 1. 17.
 *      Author: choiis1207
 */

#include "MPool.h"
// ������
MPool::MPool() {
	data = new char[1000 * sizeof(PER_IO_DATA)]; // size ���ϱ� ��ϼ� ��ŭ �Ҵ�
	arr = new bool[1000]; // idx��° �޸� Ǯ�� �Ҵ� ���θ� ������
	cnt = 1000;
	idx = 0;
	InitializeCriticalSectionAndSpinCount(&cs, 2000);
	memset(this->arr, 0, 1000);
}
// Singleton Instance
MPool* MPool::instance = nullptr;

MPool::~MPool() {
	delete data;
	delete arr;
	DeleteCriticalSection(&cs);
}
// �޸�Ǯ �Ҵ�
LPPER_IO_DATA MPool::malloc() {
	EnterCriticalSection(&cs);
	// �Ҵ��� �ȵ� ����Ҹ� ã�´�
	while (arr[idx]) {
		idx++;
		if (idx == cnt) {
			idx = 0;
		}
	}

	arr[idx] = true; // �Ҵ翩�� true
	idx++;
	char* alloc;
	if (idx == cnt) { // �ε����� ������ ��°�϶�
		idx = 0;
		alloc = (data + ((cnt - 1) * (sizeof(PER_IO_DATA))));
	} else {
		alloc = (data + ((idx - 1) * (sizeof(PER_IO_DATA))));
	}
	LeaveCriticalSection(&cs);
	return (LPPER_IO_DATA) alloc;
}
// �޸�Ǯ ��ȯ
void MPool::free(LPPER_IO_DATA freePoint) { // ��ȯ�� �������� idx�� ���󺹱�

	DWORD returnIdx = ((((char*) freePoint) - data) / sizeof(PER_IO_DATA));
	EnterCriticalSection(&cs);
	memset(freePoint, 0, sizeof(PER_IO_DATA));
	idx = returnIdx; // ��ȯ�Ȱ��� �ٷ� ��ȯ
	// ��ȯ�� idx�� �Ҵ翩�� No
	arr[returnIdx] = false;
	LeaveCriticalSection(&cs);
}

