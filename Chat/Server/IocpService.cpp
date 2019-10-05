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

	PACKET_DATA packet;
	packet.nowStatus = status;
	strncpy(packet.msg, msg, CHAR_SIZE);

	ioInfo->wsaBuf.buf = (char*)&packet;
	ioInfo->wsaBuf.len = sizeof(PACKET_DATA);
	ioInfo->serverMode = Operation::SEND;
	WSASend(mySock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped),
	NULL);

}
// ���� ���� ����鿡�� �޼��� ����
void IocpService::SendToRoomMsg(const char *msg, const list<SOCKET> &lists,
	ClientStatus status) {
	MPool* mp = MPool::getInstance();
	CharPool* charPool = CharPool::getInstance();

	for_each(lists.begin(), lists.end(), [&] (SOCKET socket) {
		// ioInfo�� ���� ���� ������
		LPPER_IO_DATA ioInfo = mp->Malloc();

		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		PACKET_DATA packet;
		packet.nowStatus = status;
		strncpy(packet.msg, msg, CHAR_SIZE);

		ioInfo->wsaBuf.buf = (char*)& packet;
		ioInfo->wsaBuf.len = sizeof(PACKET_DATA);
		ioInfo->serverMode = Operation::SEND;
		ioInfo->recvByte = 0;

		WSASend(socket, &(ioInfo->wsaBuf), 1,
			NULL, 0, &(ioInfo->overlapped), NULL);
	
	});
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
