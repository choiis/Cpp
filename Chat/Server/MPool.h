/*
 * MPool.h
 *
 *  Created on: 2019. 1. 17.
 *      Author: choiis1207
 */

#ifndef MPOOL_H_
#define MPOOL_H_

#include <iostream>
#include <winsock2.h>
#include "common.h"

class MPool {
private:
	char* data; // �����Ҵ� ��� �ּ�
	bool* arr; // block���� �Ҵ� ����
	DWORD cnt; // ��ü ��ϼ�
	DWORD idx; // ��ȯ ��� idx
	MPool();
	~MPool();
	static MPool* instance; // Singleton Instance
public:
	// Singleton Instance �� ��ȯ
	static MPool* getInstance() {
		if (instance == nullptr) {
			cout <<"�޸�Ǯ 1000�� �Ҵ�!"<<endl;
			instance = new MPool();
		}
		return instance;
	}
	// �޸�Ǯ �Ҵ�
	LPPER_IO_DATA malloc();
	// �޸�Ǯ ��ȯ
	void free(LPPER_IO_DATA freePoint);
	// ������
	DWORD blockSize() {
		return cnt;
	}
};

#endif /* MPOOL_H_ */
