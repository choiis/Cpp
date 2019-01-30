//============================================================================
// Name        : BClient.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>
#include <queue>
#include <algorithm>
#include <unordered_map>
#include <ctime>
#include <string>
#include "common.h"
#include "MPool.h"
#include "CharPool.h"
#include <list>

//#define _SCL_SECURE_NO_WARNINGS
using namespace std;

CRITICAL_SECTION userCs;

typedef struct { // socket info
	SOCKET Sock;
	int clientStatus;
	int direction;
	string id;
	string message;
	int job;
} INFO_CLIENT, *P_INFO_CLIENT;

class ClientQueue {
private:
	unordered_map<SOCKET, INFO_CLIENT> clientMap;
	queue<INFO_CLIENT> makeQueue;
	queue<INFO_CLIENT> sendQueue;
	CRITICAL_SECTION makeCs;
	CRITICAL_SECTION sendCs;
public:
	ClientQueue()	{
		InitializeCriticalSectionAndSpinCount(&makeCs, 2000);

		InitializeCriticalSectionAndSpinCount(&sendCs, 2000);
	}
	~ClientQueue(){
		DeleteCriticalSection(&makeCs);

		DeleteCriticalSection(&sendCs);
	}
	const queue<INFO_CLIENT>& getMakeQueue() const {
		return makeQueue;
	}

	void setMakeQueue(const queue<INFO_CLIENT> recvQueue) {
		this->makeQueue = recvQueue;
	}

	const queue<INFO_CLIENT>& getSendQueue() const {
		return sendQueue;
	}

	void setSendQueue(const queue<INFO_CLIENT> sendQueue) {
		this->sendQueue = sendQueue;
	}

	void pushSendQueue(INFO_CLIENT infoClinet) {
		EnterCriticalSection(&sendCs);
		this->sendQueue.push(infoClinet);
		LeaveCriticalSection(&sendCs);
	}

	INFO_CLIENT popSendQueue() {
		INFO_CLIENT infoClinet;
		EnterCriticalSection(&sendCs);
		if (!sendQueue.empty()){
			infoClinet = sendQueue.front();
			infoClinet.job = 1;
			sendQueue.pop();
		}
		LeaveCriticalSection(&sendCs);
		return infoClinet;
	}

	void pushMakeQueue(INFO_CLIENT infoClinet) {
		EnterCriticalSection(&makeCs);
		this->makeQueue.push(infoClinet);
		LeaveCriticalSection(&makeCs);
	}

	INFO_CLIENT popMakeQueue() {
		INFO_CLIENT infoClinet;
		EnterCriticalSection(&makeCs);
		if (!makeQueue.empty()) {
			infoClinet = makeQueue.front();
			infoClinet.job = 1;
			makeQueue.pop();
		}
		LeaveCriticalSection(&makeCs);
		return infoClinet;
	}

	// client���º��� ������ const ����
	unordered_map<SOCKET, INFO_CLIENT>& getClientMap() {
		return clientMap;
	}

	void setClientMap(const unordered_map<SOCKET, INFO_CLIENT>& clientMap) {
		this->clientMap = clientMap;
	}
};

ClientQueue* clientQueue;

// Recv ��� �����Լ�
void RecvMore(SOCKET sock, LPPER_IO_DATA ioInfo) {
	DWORD recvBytes = 0;
	DWORD flags = 0;
	memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
	ioInfo->wsaBuf.len = BUF_SIZE;
	memset(ioInfo->buffer, 0, BUF_SIZE);
	ioInfo->wsaBuf.buf = ioInfo->buffer;
	ioInfo->serverMode = READ_MORE;

	// ��� Recv
	WSARecv(sock, &(ioInfo->wsaBuf), 1, &recvBytes, &flags,
		&(ioInfo->overlapped),
		NULL);
}
// Recv �����Լ�
void Recv(SOCKET sock) {
	DWORD recvBytes = 0;
	DWORD flags = 0;
	MPool* mp = MPool::getInstance();
	LPPER_IO_DATA ioInfo = mp->Malloc();

	memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
	ioInfo->wsaBuf.len = BUF_SIZE;
	ioInfo->wsaBuf.buf = ioInfo->buffer;
	ioInfo->serverMode = READ;
	ioInfo->recvByte = 0;
	ioInfo->totByte = 0;

	// ��� Recv
	WSARecv(sock, &(ioInfo->wsaBuf), 1, &recvBytes, &flags,
		&(ioInfo->overlapped),
		NULL);
}

// WSASend�� call
void SendMsg(SOCKET clientSock, const char* msg, int direction) {
	MPool* mp = MPool::getInstance();
	LPPER_IO_DATA ioInfo = mp->Malloc();

	unsigned short len = min((unsigned short)strlen(msg) + 11, BUF_SIZE); // �ִ� ������ �ִ� ���� 500Byte
	CharPool* charPool = CharPool::getInstance();
	char* packet = charPool->Malloc(); // 512 Byte���� �б� ����

	copy((char*)&len, (char*)&len + 2, packet); // dataSize
	copy((char*)&direction, (char*)&direction + 4, packet + 6);  // direction
	copy(msg, msg + len, packet + 10);  // msg
	ioInfo->wsaBuf.buf = (char*)packet;
	ioInfo->wsaBuf.len = len;
	ioInfo->serverMode = WRITE;
	WSASend(clientSock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped),
		NULL);
}

// ��Ŷ ������ �б�
short PacketReading(LPPER_IO_DATA ioInfo, short bytesTrans) {
	// IO �Ϸ��� ���� �κ�
	if (READ == ioInfo->serverMode) {
		if (bytesTrans >= 2) {
			copy(ioInfo->buffer, ioInfo->buffer + 2,
				(char*)&(ioInfo->bodySize));
			CharPool* charPool = CharPool::getInstance();
			ioInfo->recvBuffer = charPool->Malloc(); // 512 Byte���� ī�� ����
			if (bytesTrans - ioInfo->bodySize > 0) { // ��Ŷ �����ִ� ���
				copy(ioInfo->buffer, ioInfo->buffer + ioInfo->bodySize,
					((char*)ioInfo->recvBuffer));

				copy(ioInfo->buffer + ioInfo->bodySize, ioInfo->buffer + bytesTrans, ioInfo->buffer); // ������ �� �� byte�迭 ����
				return bytesTrans - ioInfo->bodySize; // ���� ����Ʈ ��
			}
			else if (bytesTrans - ioInfo->bodySize == 0) { // �������� ������ remainByte 0
				copy(ioInfo->buffer, ioInfo->buffer + ioInfo->bodySize,
					((char*)ioInfo->recvBuffer));
				return 0;
			}
			else { // �ٵ� ���� ���� => RecvMore���� ���� �κ� ����
				copy(ioInfo->buffer, ioInfo->buffer + bytesTrans, ((char*)ioInfo->recvBuffer));
				ioInfo->recvByte = bytesTrans;
				return bytesTrans - ioInfo->bodySize;
			}
		}
		else if (bytesTrans == 1) { // ��� ����
			copy(ioInfo->buffer, ioInfo->buffer + bytesTrans,
				(char*)&(ioInfo->bodySize)); // �������ִ� Byte���� ī��
			ioInfo->recvByte = bytesTrans;
			ioInfo->totByte = 0;
			return -1;
		}
		else {
			return 0;
		}
	}
	else { // �� �б� (BodySize©�����)
		ioInfo->serverMode = READ;
		if (ioInfo->recvByte < 2) { // Body���� ����
			copy(ioInfo->buffer, ioInfo->buffer + (2 - ioInfo->recvByte),
				(char*)&(ioInfo->bodySize) + ioInfo->recvByte); // �������ִ� Byte���� ī��
			CharPool* charPool = CharPool::getInstance();
			ioInfo->recvBuffer = charPool->Malloc(); // 512 Byte���� ī�� ����
			copy(ioInfo->buffer + (2 - ioInfo->recvByte), ioInfo->buffer + (ioInfo->bodySize + 2 - ioInfo->recvByte),
				((char*)ioInfo->recvBuffer) + 2);

			if (bytesTrans - ioInfo->bodySize > 0) { // ��Ŷ �����ִ� ���
				copy(ioInfo->buffer + (ioInfo->bodySize - ioInfo->recvByte), ioInfo->buffer + bytesTrans, ioInfo->buffer); // ���⹮��?
				return bytesTrans - (ioInfo->bodySize - ioInfo->recvByte); // ���� ����Ʈ ��  ���⹮��?
			}
			else { // �������� ������ remainByte 0
				return 0;
			}
		}
		else { // body������ ����
			copy(ioInfo->buffer, ioInfo->buffer + (ioInfo->bodySize - ioInfo->recvByte),
				((char*)ioInfo->recvBuffer) + ioInfo->recvByte);

			// cout << "bodySize readmore" << ioInfo->bodySize << endl;

			if (bytesTrans - ioInfo->bodySize > 0) { // ��Ŷ �����ִ� ���
				copy(ioInfo->buffer + (ioInfo->bodySize - ioInfo->recvByte), ioInfo->buffer + bytesTrans, ioInfo->buffer);
				return bytesTrans - (ioInfo->bodySize - ioInfo->recvByte); // ���� ����Ʈ ��
			}
			else if (bytesTrans - ioInfo->bodySize == 0) { // �������� ������ remainByte 0
				return 0;
			}
			else { // �ٵ� ���� ���� => RecvMore���� ���� �κ� ����
				copy(ioInfo->buffer + 2, ioInfo->buffer + bytesTrans, ioInfo->buffer);
				ioInfo->recvByte = bytesTrans;
				return bytesTrans - (ioInfo->bodySize - ioInfo->recvByte);
			}
		}

	}
}

// Ŭ���̾�Ʈ���� ���� ������ ������ ����ü ����
char* DataCopy(LPPER_IO_DATA ioInfo, int *status) {
	copy(((char*)ioInfo->recvBuffer) + 2, ((char*)ioInfo->recvBuffer) + 6,
		(char*)status);
	CharPool* charPool = CharPool::getInstance();
	char* msg = charPool->Malloc(); // 512 Byte���� ī�� ����

	copy(((char*)ioInfo->recvBuffer) + 10,
		((char*)ioInfo->recvBuffer) + 10
		+ min(ioInfo->bodySize, (DWORD)BUF_SIZE), msg); //������ġ

	// �� ���� �޾����� �Ҵ� ����
	charPool->Free(ioInfo->recvBuffer);

	return msg;
}

// �۽��� ����� ������
unsigned WINAPI SendMsgThread(void *arg) {

	while (1) {
		INFO_CLIENT info = clientQueue->popSendQueue();
		// ������ ť�� ��ȯ�޴´�
		if (info.job == 1) {

			SendMsg(info.Sock, info.message.c_str(), info.direction);
			info.job = 0;
			clientQueue->pushMakeQueue(info);
		}
	}
	return 0;
}

// Ŭ���̾�Ʈ�� ����� ����� Send���� ���� ������9
unsigned WINAPI MakeMsgThread(void *arg) {

	while (1) {
		// �ޱ� ť�� ��ȯ�޴´�

		string alpha1 = "abcdefghijklmnopqrstuvwxyz";
		string alpha2 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		string password = "1234";

		INFO_CLIENT info = clientQueue->popMakeQueue();
		Sleep(1);
		if (info.job == 1) {
	
			EnterCriticalSection(&userCs);
			int cStatus =
				clientQueue->getClientMap().find(info.Sock)->second.clientStatus;
			string idName =
				clientQueue->getClientMap().find(info.Sock)->second.id;
			LeaveCriticalSection(&userCs);

			if (cStatus == STATUS_LOGOUT) {
				int randNum3 = (rand() % 2);
				if (randNum3 % 2 == 1) {
					int randNum1 = (rand() % 2);
					int randNum2 = (rand() % 26);
					string nickName = "";
					if (randNum1 == 0) {
						nickName += alpha1.at(randNum2);
					}
					else {
						nickName += alpha2.at(randNum2);
					}
					string msg1 = idName;
					msg1.append("\\");
					msg1.append(password); // ��й�ȣ
					msg1.append("\\");
					msg1.append(idName); // �г���
					info.direction = USER_MAKE;

					info.message = msg1;

				}
				else {
					int randNum1 = (rand() % 2);
					int randNum2 = (rand() % 26);
					string nickName = "";
					if (randNum1 == 0) {
						nickName += alpha1.at(randNum2);
					}
					else {
						nickName += alpha2.at(randNum2);
					}
					string msg2 = idName;
					msg2.append("\\");
					msg2.append(password); // ��й�ȣ
					info.direction = USER_ENTER;
					info.message = msg2;
				}
			}
			else if (cStatus == STATUS_WAITING) {

				int directionNum = (rand() % 10);
				if ((directionNum % 2) == 0) { // 0 2 4 6 8 ������
					int randNum1 = (rand() % 2);
					int randNum2 = (rand() % 10);
					string roomName = "";
					if (randNum1 == 0) {
						roomName += alpha1.at(randNum2);
					}
					else {
						roomName += alpha2.at(randNum2);
					}
					info.direction = ROOM_ENTER;
					info.message = roomName;
				}
				else if ((directionNum % 3) == 0) { // 3 6 9 �游���
					int randNum1 = (rand() % 2);
					int randNum2 = (rand() % 10);
					string roomName = "";
					if (randNum1 == 0) {
						roomName += alpha1.at(randNum2);
					}
					else {
						roomName += alpha2.at(randNum2);
					}
					info.direction = ROOM_MAKE;

					info.message = roomName;
				}
				else if (directionNum == 7) { // 7 ������
					info.direction = ROOM_INFO;
					info.message = "";
				}
				else if (directionNum == 1) { // 1 ��������
					info.direction = ROOM_USER_INFO;

					info.message = "";
				}
			}
			else if (cStatus == STATUS_CHATTIG) {
				int directionNum = (rand() % 60);
				string msg = "";
				if (directionNum < 59) {
					int randNum1 = (rand() % 2);
					for (int i = 0; i <= (rand() % 60) + 2; i++) {
						int randNum2 = (rand() % 26);
						if (randNum1 == 0) {
							msg += alpha1.at(randNum2);
						}
						else {
							msg += alpha2.at(randNum2);
						}
					}
				}
				else { // 60���� �ѹ� ����
					msg = "\\out";
				}
				info.direction = -1;
				info.message = msg;
			}

			info.job = 0;
			clientQueue->pushSendQueue(info);
		}
	}
	return 0;
}
// ������ ����� ������
unsigned WINAPI RecvMsgThread(LPVOID hComPort) {

	SOCKET sock;
	short bytesTrans;
	LPPER_IO_DATA ioInfo;

	while (1) {
		bool success = GetQueuedCompletionStatus(hComPort, (LPDWORD)&bytesTrans,
			(LPDWORD) &sock, (LPOVERLAPPED*)&ioInfo, INFINITE);

		if (bytesTrans == 0 && !success) { // ���� ���� �ܼ� ���� ����
			// ���� �ܼ� �������� ó��
			cout << "���� ����" << sock << endl;
			closesocket(sock);
			MPool* mp = MPool::getInstance();
			mp->Free(ioInfo);
		}
		else if (READ_MORE == ioInfo->serverMode
			|| READ == ioInfo->serverMode) {

			// ������ �б� ����
			short remainByte = min(bytesTrans, BUF_SIZE); // �ʱ� Remain Byte
			bool recvMore = false;

			while (1) {
				remainByte = PacketReading(ioInfo, remainByte);

				// �� ���� �� ���� ����
				// DataCopy������ ��� �޸� ���� ��ȯ
				if (remainByte >= 0) {
					int status;
					char *msg = DataCopy(ioInfo, &status);

					// Client�� ���� ���� ���� �ʼ�
					// �������� �ذ����� ����
					if (status == STATUS_LOGOUT || status == STATUS_WAITING
						|| status == STATUS_CHATTIG) {
						EnterCriticalSection(&userCs);

						unordered_map<SOCKET, INFO_CLIENT> clientMap =
							clientQueue->getClientMap();
						clientMap.find(sock)->second.clientStatus = status; // clear���� client���º��� ���ش�
						clientQueue->setClientMap(clientMap);

						LeaveCriticalSection(&userCs);
					}
					CharPool* charPool = CharPool::getInstance();
					charPool->Free(msg);
				}

				if (remainByte == 0) {
					MPool* mp = MPool::getInstance();
					mp->Free(ioInfo);
					break;
				}
				else if (remainByte < 0) {// ���� ��Ŷ ���� || ��� �� ������ -> ���޾ƾ���
					RecvMore(sock, ioInfo); // ��Ŷ ���ޱ� & �⺻ ioInfo ����
					recvMore = true;
					break;
				}
			}
			if (!recvMore) {
				Recv(sock);
			}

		}
		else if (WRITE == ioInfo->serverMode) {
			MPool* mp = MPool::getInstance();
			CharPool* charPool = CharPool::getInstance();
			charPool->Free(ioInfo->wsaBuf.buf);
			mp->Free(ioInfo);
		}
	}
	return 0;
}

int main() {

	WSADATA wsaData;

	// Socket lib�� �ʱ�ȭ
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("WSAStartup() error!");
		exit(1);
	}

	cout << "��Ʈ��ȣ�� �Է��� �ּ��� :";
	string port;
	cin >> port;

	DWORD clientCnt;
	cout << "Ŭ���̾�Ʈ ���� �Է��� �ּ���";
	cin >> clientCnt;
	queue<INFO_CLIENT> recvQueues;
	queue<INFO_CLIENT> sendQueues;
	unordered_map<SOCKET, INFO_CLIENT> userMap;
	clientQueue = new ClientQueue();
	InitializeCriticalSectionAndSpinCount(&userCs, 2000);

	// Completion Port ����
	HANDLE hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	srand((unsigned int)time(NULL));

	for (DWORD i = 0; i < clientCnt; i++) {
		SOCKET clientSocket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP,
			NULL, 0,
			WSA_FLAG_OVERLAPPED);
		SOCKADDR_IN servAddr;
		memset(&servAddr, 0, sizeof(servAddr));
		servAddr.sin_family = PF_INET;
		servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
		servAddr.sin_port = htons(atoi(port.c_str()));

		if (connect(clientSocket, (SOCKADDR*)&servAddr,
			sizeof(servAddr)) == SOCKET_ERROR) {
			printf("connect() error!");
		}

		// Completion Port �� ���� ����
		CreateIoCompletionPort((HANDLE)clientSocket, hComPort,
			(DWORD)clientSocket, 0);
		// �ʱ�ȭ�� ���¸� queue�� insert
		INFO_CLIENT info;
		info.Sock = clientSocket;
		info.clientStatus = STATUS_LOGOUT;
		string alpha1 = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
		info.id = "";
		int j = i % 52;
		for (DWORD k = 0; k <= i / 52; k++) {
			info.id += alpha1.at(j);
		}

		// Recv���� ����
		info.job = 0;
		recvQueues.push(info);
		// userMap ä���
		userMap[clientSocket] = info;
		// cout << "sock num " << clientSocket << " id " << info.id << endl;
		// �� socket�� Recv ��������
		Recv(clientSocket);

	}
	// ��ü clientUser
	clientQueue->setClientMap(userMap);
	// Recvť�� ä����
	clientQueue->setMakeQueue(recvQueues);
	// Sendť�� ����·�
	clientQueue->setSendQueue(sendQueues);

	cout << "���� ���� Ŭ���̾�Ʈ ��" << clientCnt << endl;

	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	int process = sysInfo.dwNumberOfProcessors;

	// ���� ������ ����
	// ������� RecvMsg�� hComPort CP ������Ʈ�� �Ҵ��Ѵ�
	// RecvMsg���� Recv�� �Ϸ�Ǹ� ������ �κ��� �ִ�
	for (int i = 0; i < process / 2; i++) {
		_beginthreadex(NULL, 0, RecvMsgThread, (LPVOID)hComPort, 0,
			NULL);
	}

	// ������ ������� ������ ����
	// ������� ����ť�� hComPort SendMsgThread���� ó���ϰ� �ȴ�
	_beginthreadex(NULL, 0, MakeMsgThread,
		NULL, 0,
		NULL);
	

	// �۽� ������ ����
	// Thread�ȿ��� clientSocket���� Send���ٰŴϱ� ���ڷ� �Ѱ��ش�
	// CP�� Send�� ���� �ȵǾ����� GetQueuedCompletionStatus���� Send �Ϸ�ó�� �ʿ����
	for (int i = 0; i < 3; i++) {
		_beginthreadex(NULL, 0, SendMsgThread,
			NULL, 0,
			NULL);
	}

	while (true) {
		int moreClient;
		int speed;

		//	cout << " ������ Ŭ���̾�Ʈ ���� �Է��ϼ���" << endl;
		cin >> moreClient;

		//	cout << " ��Ŷ ���� 1/1000 �� ������ �Է��ϼ���" << endl;
		//	cin >> speed;

	}
	// �Ӱ迵�� Object ��ȯ
	DeleteCriticalSection(&userCs);

	delete clientQueue;
	WSACleanup();
	return 0;
}
