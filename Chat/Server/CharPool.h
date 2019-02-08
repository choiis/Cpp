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
#include <concurrent_queue.h>
#include "common.h"

#define BLOCK_SIZE 1024

class CharPool {
private:
	char* data;
	Concurrency::concurrent_queue<char*> poolQueue;
	DWORD len;
	CharPool();
	~CharPool();
	static CharPool* instance; // Singleton Instance
public:
	// Singleton Instance �� ��ȯ
	static CharPool* getInstance() {
		if (instance == nullptr) {
			cout << "char �޸�Ǯ 2000�� �Ҵ�!" << endl;
			instance = new CharPool();
		}
		return instance;
	}
	// �޸�Ǯ �Ҵ�
	char* Malloc();
	// �޸�Ǯ ��ȯ
	void Free(char* freePoint);

};

#endif /* CHARPOOL_H_ */
