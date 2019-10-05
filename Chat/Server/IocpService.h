/*
 * IocpService.h
 *
 *  Created on: 2019. 1. 17.
 *      Author: choiis1207
 */

#ifndef IOCPSERVICE_H_
#define IOCPSERVICE_H_

#include <winsock2.h>
#include <algorithm>
#include <list>
#include "common.h"
#include "MPool.h"
#include "CharPool.h"

namespace IocpService {

class IocpService {
private:
	IocpService(const IocpService& rhs) = delete;
	void operator=(const IocpService& rhs) = delete;
	IocpService(IocpService&& rhs) = delete;
public:
	enum Operation {
		RECV = 1,
		SEND = 2
	};

	IocpService();
	virtual ~IocpService();

	// �Ѹ��� �޼��� ����
	void SendToOneMsg(const char *msg, SOCKET mySock, ClientStatus status);
	// ���� ���� ����鿡�� �޼��� ����
	void SendToRoomMsg(const char *msg, const list<SOCKET> &lists, ClientStatus status);
	// Recv ��� �����Լ�
	void RecvMore(SOCKET sock, LPPER_IO_DATA ioInfo);

	// Recv �����Լ�
	void Recv(SOCKET sock);

};
/* namespace IocpService */
}
#endif /* IOCPSERVICE_H_ */
