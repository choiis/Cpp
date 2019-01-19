/*
 * IocpService.h
 *
 *  Created on: 2019. 1. 17.
 *      Author: choiis1207
 */

#ifndef IOCPSERVICE_H_
#define IOCPSERVICE_H_

#include <winsock2.h>
#include <unordered_map>
#include <list>
#include "common.h"
#include "MPool.h"
namespace IocpService {

class IocpService {
public:
	IocpService();
	virtual ~IocpService();

	// �Ѹ����� �޼��� ����
	void SendToOneMsg(const char *msg, SOCKET mySock, int status);
	// ���� ���� ����鿡�� �޼��� ����
	void SendToRoomMsg(const char *msg, const list<SOCKET> &lists, int status);
	// Recv ��� �����Լ�
	void RecvMore(SOCKET sock, LPPER_IO_DATA ioInfo);

// Recv �����Լ�
	void Recv(SOCKET sock);
};
/* namespace IocpService */
}
#endif /* IOCPSERVICE_H_ */