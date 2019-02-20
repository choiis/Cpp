/*
 * BusinessService.h
 *
 *  Created on: 2019. 1. 17.
 *      Author: choiis1207
 */

#ifndef BUSINESSSERVICE_H_
#define BUSINESSSERVICE_H_

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <queue>
#include <direct.h>
#include "IocpService.h"
#include "FileService.h"
#include "Dao.h"

// #pragma comment(lib, "Dao.h") 

// SQLwork���� ������ ������
#define UPDATE_USER 1
#define INSERT_LOGIN 2
#define INSERT_DIRECTION 3
#define INSERT_CHATTING 4

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
	int direction;
	Vo vo;
} SQL_DATA, *P_SQL_DATA;

namespace BusinessService {

class BusinessService {
private:
	// DB connection Object
	Dao* dao;
	// �ߺ��α��� ������ ���� ����ü
	unordered_set<string> idSet;
	// ������ ������ ���� �ڷ� ����
	unordered_map<SOCKET, PER_HANDLE_DATA> userMap;
	// ������ �� ���� ����
	unordered_map<string, shared_ptr<ROOM_DATA>> roomMap;

	queue<SQL_DATA> sqlQueue;

	queue<Send_DATA> sendQueue;
	// �Ӱ迵���� �ʿ��� ��ü
	// Ŀ�θ�� �ƴ϶� ���������� ����ȭ ����� ��
	// �� ���μ������� ����ȭ �̹Ƿ� ũ��Ƽ�ü��� ���

	// idMap ����ȭ
	CRITICAL_SECTION idCs;
	// userMap ����ȭ
	CRITICAL_SECTION userCs;
	// roomMap ����ȭ
	CRITICAL_SECTION roomCs;
	// sqlQueue ����ȭ
	CRITICAL_SECTION sqlCs;
	// sendQueue ����ȭ
	CRITICAL_SECTION sendCs;

	// ���Ӳ����� socket�� Send���� ����
	// UDP ���� Case������ �� Ŭ���̾�Ʈ socket�� IP�� �����Ѵ�
	unordered_map<SOCKET, string> liveSocket;

	CRITICAL_SECTION liveSocketCs;

	IocpService::IocpService *iocpService;

	FileService::FileService *fileService;

public:
	// ������
	BusinessService();
	// �Ҹ���
	virtual ~BusinessService();
	// SQLThread���� ������ �κ�
	void SQLwork();
	// SendThread���� ������ �κ�
	void Sendwork();
	// InsertSendQueue ����ȭ
	void InsertSendQueue(int direction, const string& msg, const string& roomName, SOCKET socket, int status);
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
	// ä�ù濡���� ���� ����� ���̽�
	void StatusFile(SOCKET sock);
	// Ŭ���̾�Ʈ���� ���� ������ ������ ����ü ����
	string DataCopy(LPPER_IO_DATA ioInfo, int *status, int *direction);
	// ��Ŷ ������ �б�
	short PacketReading(LPPER_IO_DATA ioInfo, short bytesTrans);
	// �α��� ���� üũ �� ��ȯ
	bool SessionCheck(SOCKET sock);
	// Ŭ���̾�Ʈ�� �������� ��ȯ
	int GetStatus(SOCKET sock);
	// ģ���߰� ���
	void AddFriend(SOCKET sock, const string& msg, const string& id, int status);
	// ������ socket Insert
	void InsertLiveSocket(SOCKET& hClientSock,SOCKADDR_IN& addr);
	// socket �׾����� Ȯ��
	bool IsSocketDead(SOCKET socket);
	// node �������� �����ϱ�
	void BanUser(SOCKET socket, const char* nickName);
	// node ������ �α��� ������ ��ȯ
	void CallCnt(SOCKET socket, const DWORD& cnt);

	const unordered_set<string>& getIdSet() const {
		return idSet;
	}

	const unordered_map<string, std::shared_ptr<ROOM_DATA>>& getRoomMap() const {
		return roomMap;
	}

	const unordered_map<SOCKET, PER_HANDLE_DATA>& getUserMap() const {
		return userMap;
	}

	IocpService::IocpService* getIocpService() ;

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
