/*
 * BusinessService.h
 *
 *  Created on: 2019. 1. 17.
 *      Author: choiis1207
 */

#ifndef BUSINESSSERVICE_H_
#define BUSINESSSERVICE_H_

#include <winsock2.h>
#include <unordered_map>
#include <list>
#include "IocpService.h"
#include "MPool.h"
#include "common.h"
// ������ ������ ���� ����
// client ���Ͽ� �����ϴ� ��������
typedef struct { // socket info
	char userName[NAME_SIZE];
	char roomName[NAME_SIZE];
	char userId[NAME_SIZE];
	int status;
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

// �񵿱� ��ſ� �ʿ��� ����ü
typedef struct { // buffer info
	char nickname[NAME_SIZE];
	string password;
	int logMode;
} USER_DATA, *P_USER_DATA;

namespace Service {

class BusinessService {
private:
	unordered_map<string, USER_DATA> idMap;
	// ������ ������ ���� �ڷ� ����
	unordered_map<SOCKET, LPPER_HANDLE_DATA> userMap;
	// ������ �� ���� ����
	unordered_map<string, list<SOCKET> > roomMap;

	// �Ӱ迵���� �ʿ��� ��ü
	// Ŀ�θ�� �ƴ϶� ���������� ����ȭ ����� ��
	// �� ���μ������� ����ȭ �̹Ƿ� ũ��Ƽ�ü��� ���

	// idMap ����ȭ
	CRITICAL_SECTION idCs;
	// userMap ����ȭ
	CRITICAL_SECTION userCs;
	// roomMap ����ȭ
	CRITICAL_SECTION roomCs;

	IocpService::IocpService *iocpService;
public:
	// ������
	BusinessService();
	// �Ҹ���
	virtual ~BusinessService();
	// �ʱ� �α���
	// �������� �߰�
	void InitUser(const char *id, SOCKET sock);
	// ���� �������� ����
	void ClientExit(SOCKET sock);
	// �α��� ���� ����ó��
	// ���ǰ� ���� �� ����
	void StatusLogout(SOCKET sock, int status, int direction, char *message);
	// ���ǿ����� ���� ó��
	// ���ǰ� ����
	void StatusWait(SOCKET sock, int status, int direction, char *message);
	// ä�ù濡���� ���� ó��
	// ���ǰ� ����
	void StatusChat(SOCKET sock, int status, int direction, char *message);
	// Ŭ���̾�Ʈ���� ���� ������ ������ ����ü ����
	char* DataCopy(LPPER_IO_DATA ioInfo, int *status, int *direction);
	// ��Ŷ ������ �б�
	void PacketReading(LPPER_IO_DATA ioInfo, DWORD bytesTrans);

	const unordered_map<string, USER_DATA>& getIdMap() const {
		return idMap;
	}

	const unordered_map<string, list<SOCKET> >& getRoomMap() const {
		return roomMap;
	}

	const unordered_map<SOCKET, LPPER_HANDLE_DATA>& getUserMap() const {
		return userMap;
	}

	IocpService::IocpService* getIocpService();
};

} /* namespace Service */

#endif /* BUSINESSSERVICE_H_ */
