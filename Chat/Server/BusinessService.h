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
#include <map>
#include <string>
#include <queue>
#include <direct.h>
#include <mutex>
#include "IocpService.h"
#include "FileService.h"
#include "Dao.h"

// #pragma comment(lib, "Dao.h") 

// SQLwork���� ������ ������
enum class SqlWork{
	UPDATE_USER,
	INSERT_LOGIN,
	INSERT_DIRECTION,
	INSERT_CHATTING
};

// ������ ������ ���� ����
// client ���Ͽ� �����ϴ� ��������
typedef struct { // socket info
	char userName[NAME_SIZE];
	char roomName[NAME_SIZE];
	char userId[NAME_SIZE];
	ClientStatus status;
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

// �񵿱� ��ſ� �ʿ��� ����ü
typedef struct { // buffer info
	SqlWork direction;
	LogVo vo;
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
	map<string, shared_ptr<ROOM_DATA>> roomMap;

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
	mutex sqlCs;
	// sendQueue ����ȭ
	CRITICAL_SECTION sendCs;

	// ���Ӳ����� socket�� Send���� ����
	// UDP ���� Case������ �� Ŭ���̾�Ʈ socket�� IP�� �����Ѵ�
	map<SOCKET, string> liveSocket;

	mutex liveSocketCs;

	IocpService::IocpService *iocpService;

	FileService::FileService *fileService;

	BusinessService(const BusinessService& rhs) = delete;
	void operator=(const BusinessService& rhs) = delete;
	BusinessService(BusinessService&& rhs) = delete;
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
	void InsertSendQueue(SendTo direction, const string& msg, const string& roomName, SOCKET socket, ClientStatus status);
	// �ʱ� �α���
	// �������� �߰�
	void InitUser(const char *id, SOCKET sock ,const char *nickName);
	// ���� �������� ����
	void ClientExit(SOCKET sock);
	// �α��� ���� ����ó��
	// ���ǰ� ���� �� ����
	void StatusLogout(SOCKET sock, Direction direction, const char *message);
	// ���ǿ����� ���� ó��
	// ���ǰ� ����
	void StatusWait(SOCKET sock, ClientStatus status, Direction direction, const char *message);
	// ä�ù濡���� ���� ó��
	// ���ǰ� ����
	void StatusChat(SOCKET sock, ClientStatus status, Direction direction, const char *message);
	// ä�ù濡���� ���� ����� ���̽�
	void StatusFile(SOCKET sock);
	// Ŭ���̾�Ʈ���� ���� ������ ������ ����ü ����
	string DataCopy(LPPER_IO_DATA ioInfo, ClientStatus *status, Direction *direction);
	// ��Ŷ ������ �б�
	short PacketReading(LPPER_IO_DATA ioInfo, short bytesTrans);
	// �α��� ���� üũ �� ��ȯ
	bool SessionCheck(SOCKET sock);
	// Ŭ���̾�Ʈ�� �������� ��ȯ
	ClientStatus GetStatus(SOCKET sock);
	// ģ���߰� ���
	void AddFriend(SOCKET sock, const string& msg, const string& id, ClientStatus status);
	// ������ socket Insert
	void InsertLiveSocket(const SOCKET& hClientSock, const SOCKADDR_IN& addr);
	// socket �׾����� Ȯ��
	bool IsSocketDead(SOCKET socket);
	// node �������� �����ϱ�
	void BanUser(SOCKET socket, const char* nickName);
	// node ������ �α��� ������ ��ȯ
	void CallCnt(SOCKET socket, const DWORD& cnt);

	const unordered_set<string>& getIdSet() const {
		return idSet;
	}

	const map<string, std::shared_ptr<ROOM_DATA>>& getRoomMap() const {
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
