/*
 * MPool.h
 *
 *  Created on: 2019. 1. 17.
 *      Author: choiis1207
 */

#ifndef CHARPOOL_H_
#define CHARPOOL_H_

#include <iostream>
#include <winsock2.h>

#define BLOCK_SIZE 256

class CharPool {
private:
	char* data; // �����Ҵ� ��� �ּ�
	bool* arr; // block���� �Ҵ� ����
	DWORD cnt; // ��ü ��ϼ�
	DWORD idx; // ��ȯ ��� idx
	CRITICAL_SECTION cs; // �޸�Ǯ ����ȭ ũ��Ƽ�ü���
	CharPool();
	~CharPool();
	static CharPool* instance; // Singleton Instance
public:
	// Singleton Instance �� ��ȯ
	static CharPool* getInstance() {
		if (instance == nullptr) {
			instance = new CharPool();
		}
		return instance;
	}
	// �޸�Ǯ �Ҵ�
	char* malloc();
	// �޸�Ǯ ��ȯ
	void free(char* freePoint);
	// ������
	DWORD blockSize() {
		return cnt;
	}
};

#endif /* CHARPOOL_H_ */
