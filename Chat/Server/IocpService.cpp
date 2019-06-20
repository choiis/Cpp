/*
 * IocpService.h
 *
 *  Created on: 2019. 1. 17.
 *      Author: choiis1207
 */

#include "IocpService.h"

namespace IocpService {

IocpService::IocpService() {

}

IocpService::~IocpService() {
}
// �Ѹ��� �޼��� ����
void IocpService::SendToOneMsg(const char *msg, SOCKET mySock, ClientStatus status) {
	MPool* mp = MPool::getInstance();
	LPPER_IO_DATA ioInfo = mp->Malloc();

	memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));

	unsigned short len = min((unsigned short)strlen(msg) + 11, BUF_SIZE); // �ִ� ������ �ִ� ���� 502Byte
	CharPool* charPool = CharPool::getInstance();
	char* packet = charPool->Malloc(); // 512 Byte���� ������ ����

	copy((char*)&len, (char*)&len + 2, packet); // dataSize
	copy((char*)&status, (char*)&status + 4, packet + 2);  // status
	copy(msg, msg + len, packet + 10);  // msg
	memset(((char*)packet) + 6, 0, 4); // direction;

	ioInfo->wsaBuf.buf = (char*)packet;
	ioInfo->wsaBuf.len = len;
	ioInfo->serverMode = Operation::SEND;
	WSASend(mySock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped),
	NULL);

}
// ���� ���� ����鿡�� �޼��� ����
void IocpService::SendToRoomMsg(const char *msg, const list<SOCKET> &lists,
	ClientStatus status) {
	MPool* mp = MPool::getInstance();
	CharPool* charPool = CharPool::getInstance();

	list<SOCKET>::const_iterator iter; // ���� �Ұ��� ��ü�� ����Ű�� �ݺ���

	for (iter = lists.begin(); iter != lists.end(); iter++) {
		// ioInfo�� ���� ���� ������
		LPPER_IO_DATA ioInfo = mp->Malloc();

		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		unsigned short len = min((unsigned short)strlen(msg) + 11, BUF_SIZE); // �ִ� ������ �ִ� ���� 502Byte

		char* packet = charPool->Malloc(); // 512 Byte���� �б� ����

		copy((char*)&len, (char*)&len + 2, packet); // dataSize
		copy((char*)&status, (char*)&status + 4, packet + 2);  // status
		copy(msg, msg + len, packet + 10);  // msg
		memset(((char*)packet) + 6, 0, 4); // direction;

		ioInfo->wsaBuf.buf = (char*)packet;
		ioInfo->wsaBuf.len = len;
		ioInfo->serverMode = Operation::SEND;
		ioInfo->recvByte = 0;

		WSASend((*iter), &(ioInfo->wsaBuf), 1,
			NULL, 0, &(ioInfo->overlapped), NULL);
	}
}

// Recv ��� �����Լ�
void IocpService::RecvMore(SOCKET sock, LPPER_IO_DATA ioInfo) {
	DWORD recvBytes = 0;
	DWORD flags = 0;

	memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
	ioInfo->wsaBuf.len = BUF_SIZE;
	memset(ioInfo->buffer, 0, BUF_SIZE);
	ioInfo->wsaBuf.buf = ioInfo->buffer;
	ioInfo->serverMode = Operation::RECV_MORE;
	// ��� Recv
	WSARecv(sock, &(ioInfo->wsaBuf), 1, &recvBytes, &flags,
			&(ioInfo->overlapped),
			NULL);
}

// Recv �����Լ�
void IocpService::Recv(SOCKET sock) {
	MPool* mp = MPool::getInstance();
	LPPER_IO_DATA ioInfo = mp->Malloc();

	DWORD recvBytes = 0;
	DWORD flags = 0;

	memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
	ioInfo->wsaBuf.len = BUF_SIZE;
	ioInfo->wsaBuf.buf = ioInfo->buffer;
	ioInfo->serverMode = Operation::RECV;
	ioInfo->recvByte = 0;
	ioInfo->totByte = 0;
	ioInfo->bodySize = 0;
	// ��� Recv
	WSARecv(sock, &(ioInfo->wsaBuf), 1, &recvBytes, &flags,
			&(ioInfo->overlapped),
			NULL);
}

}
/* namespace IocpService */
