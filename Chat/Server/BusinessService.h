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
#include <unordered_set>
#include <list>
#include <string>
#include "IocpService.h"
#include "MPool.h"
#include "common.h"
#include "CharPool.h"
#include "Dao.h"

// #pragma comment(lib, "Dao.h") 

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
	list<SOCKET> userList;
	CRITICAL_SECTION listCs;
} ROOM_DATA, *P_ROOM_DATA;

namespace Service {

class BusinessService {
private:
	// DB connection Object
	Dao* dao;
	// �ߺ��α��� ������ ���� ����ü
	unordered_set<string> idSet;
	// ������ ������ ���� �ڷ� ����
	unordered_map<SOCKET, PER_HANDLE_DATA> userMap;
	// ������ �� ���� ����
	unordered_map<string, ROOM_DATA> roomMap;

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
	void InitUser(const char *id, SOCKET sock ,const char *nickName);
	// ���� �������� ����
	void ClientExit(SOCKET sock);
	// �α��� ���� ����ó��
	// ���ǰ� ���� �� ����
	void StatusLogout(SOCKET sock, int direction, const char *message);
	// ���ǿ����� ���� ó��
	// ���ǰ� ����
	void StatusWait(SOCKET sock, int status, int direction, const char *message);
	// ä�ù濡���� ���� ó��
	// ���ǰ� ����
	void StatusChat(SOCKET sock, int status, int direction, const char *message);
	// Ŭ���̾�Ʈ���� ���� ������ ������ ����ü ����
	string DataCopy(LPPER_IO_DATA ioInfo, int *status, int *direction);
	// ��Ŷ ������ �б�
	short PacketReading(LPPER_IO_DATA ioInfo, short bytesTrans);

	bool SessionCheck(SOCKET sock);

	int GetStatus(SOCKET sock);

	const unordered_set<string>& getIdSet() const {
		return idSet;
	}

	const unordered_map<string, ROOM_DATA>& getRoomMap() const {
		return roomMap;
	}

	const unordered_map<SOCKET, PER_HANDLE_DATA>& getUserMap() const {
		return userMap;
	}

	IocpService::IocpService* getIocpService();

	const CRITICAL_SECTION& getIdCs() const {
		return idCs;
	}

	const CRITICAL_SECTION& getRoomCs() const {
		return roomCs;
	}

	const CRITICAL_SECTION& getUserCs() const {
		return userCs;
	}
};

} /* namespace Service */

#endif /* BUSINESSSERVICE_H_ */
