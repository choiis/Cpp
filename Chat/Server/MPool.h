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
#include "ConcurrentQueue.h"
#include "common.h"

class MPool {
private:
	char* data;
	DWORD len;
	ConcurrentQueue<char*> poolQueue;
	MPool();
	~MPool();
	MPool(const MPool& mpool) = delete;
	void operator=(const MPool& mpool) = delete;
	static MPool* instance; // Singleton Instance
public:
	// Singleton Instance 를 반환
	static MPool* getInstance() {
		if (instance == nullptr) {
			cout << "ioInfo Memory 50000 Piece!" << endl;
			instance = new MPool();
		}
		return instance;
	}
	// 메모리풀 할당
	LPPER_IO_DATA Malloc();
	// 메모리풀 반환
	void Free(LPPER_IO_DATA freePoint);
};

#endif /* MPOOL_H_ */
