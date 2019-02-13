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
#include <concurrent_queue.h>
#include "common.h"

class MPool {
private:
	char* data;
	DWORD len;
	Concurrency::concurrent_queue<char*> poolQueue;
	MPool();
	~MPool();
	static MPool* instance; // Singleton Instance
public:
	// Singleton Instance �� ��ȯ
	static MPool* getInstance() {
		if (instance == nullptr) {
			cout << "ioInfo �޸�Ǯ 10000�� �Ҵ�!" << endl;
			instance = new MPool();
		}
		return instance;
	}
	// �޸�Ǯ �Ҵ�
	LPPER_IO_DATA Malloc();
	// �޸�Ǯ ��ȯ
	void Free(LPPER_IO_DATA freePoint);
};

#endif /* MPOOL_H_ */
