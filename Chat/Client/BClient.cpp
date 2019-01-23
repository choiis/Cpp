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
#include <string>
#include "common.h"
#include "MPool.h"
#include "CharPool.h"
#include <list>

//#define _SCL_SECURE_NO_WARNINGS
using namespace std;

CRITICAL_SECTION cs;
//CRITICAL_SECTION sendCs;
//CRITICAL_SECTION userCs;

typedef struct { // socket info
	SOCKET Sock;
	int clientStatus;
	int direction;
	string id;
	string message;
} INFO_CLIENT, *P_INFO_CLIENT;

class ClientQueue {
private:
	unordered_map<SOCKET, INFO_CLIENT> clientMap;
	queue<INFO_CLIENT> recvQueue;
	queue<INFO_CLIENT> sendQueue;
public:
	const queue<INFO_CLIENT> getRecvQueue() const {
		return recvQueue;
	}

	void setRecvQueue(const queue<INFO_CLIENT> recvQueue) {
		this->recvQueue = recvQueue;
	}

	const queue<INFO_CLIENT> getSendQueue() const {
		return sendQueue;
	}

	void setSendQueue(const queue<INFO_CLIENT> sendQueue) {
		this->sendQueue = sendQueue;
	}

	unordered_map<SOCKET, INFO_CLIENT> getClientMap() {
		return clientMap;
	}

	void setClientMap(const unordered_map<SOCKET, INFO_CLIENT>& clientMap) {
		this->clientMap = clientMap;
	}
};

ClientQueue clientQueue;

// Recv ��� �����Լ�
void RecvMore(SOCKET sock, LPPER_IO_DATA ioInfo) {
	DWORD recvBytes = 0;
	DWORD flags = 0;
	memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
	ioInfo->wsaBuf.len = SIZE;
	memset(ioInfo->buffer, 0, SIZE);
	ioInfo->wsaBuf.buf = ioInfo->buffer;
	ioInfo->serverMode = READ_MORE; // GetQueuedCompletionStatus ���� �бⰡ Recv�� ���� �ְ�

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
	ioInfo->wsaBuf.len = SIZE;
	ioInfo->wsaBuf.buf = ioInfo->buffer;
	ioInfo->serverMode = READ; // GetQueuedCompletionStatus ���� �бⰡ Recv�� ���� �ְ�
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

	memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));

	int len = strlen(msg) + 1;
	CharPool* charPool = CharPool::getInstance();
	char* packet = charPool->Malloc(); // char[len + (3 * sizeof(int))];
	memcpy(packet, &len, 4); // dataSize;
	//memcpy(((char*) packet) + 4, &status, 4); // status;
	memcpy(((char*)packet) + 8, &direction, 4); // direction;
	memcpy(((char*)packet) + 12, msg, len); // status;

	ioInfo->wsaBuf.buf = (char*)packet;
	ioInfo->wsaBuf.len = min(len + 12, SIZE);
	ioInfo->serverMode = WRITE;

	WSASend(clientSock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped),
		NULL);
}

// ��Ŷ ������ �б�
void PacketReading(LPPER_IO_DATA ioInfo, DWORD bytesTrans) {
	// IO �Ϸ��� ���� �κ�
	if (READ == ioInfo->serverMode) {
		if (bytesTrans < 4) { // ����� �� �� �о�� ��Ȳ
			memcpy(((char*)&(ioInfo->bodySize)) + ioInfo->recvByte,
				ioInfo->buffer, bytesTrans);
		}
		else {
			memcpy(&(ioInfo->bodySize), ioInfo->buffer, 4);
			ioInfo->bodySize = min(ioInfo->bodySize, SIZE - 12);
			CharPool* charPool = CharPool::getInstance();
			ioInfo->recvBuffer = charPool->Malloc(); // char[ioInfo->bodySize + 12]; // BodySize��ŭ ���� �Ҵ�
			memcpy(((char*)ioInfo->recvBuffer), ioInfo->buffer, bytesTrans);
		}
		ioInfo->recvByte += bytesTrans; // ���ݱ��� ���� ������ �� ����
	}
	else { // �� �б�
		if (ioInfo->recvByte >= 4) { // ��� �� �о���
			memcpy(((char*)ioInfo->recvBuffer) + ioInfo->recvByte,
				ioInfo->buffer, bytesTrans);
			ioInfo->recvByte += bytesTrans; // ���ݱ��� ���� ������ �� ����
		}
		else { // ����� �� ���о��� ���
			int recv = min(4 - ioInfo->recvByte, bytesTrans);
			memcpy(((char*)&(ioInfo->bodySize)) + ioInfo->recvByte,
				ioInfo->buffer, recv); // ������� ä���
			ioInfo->bodySize = min(ioInfo->bodySize, SIZE - 12);
			ioInfo->recvByte += bytesTrans; // ���ݱ��� ���� ������ �� ����
			if (ioInfo->recvByte >= 4) {
				CharPool* charPool = CharPool::getInstance();
				ioInfo->recvBuffer = charPool->Malloc(); // char[ioInfo->bodySize + 12]; // BodySize��ŭ ���� �Ҵ�
				memcpy(((char*)ioInfo->recvBuffer) + 4,
					((char*)ioInfo->buffer) + recv, bytesTrans - recv); // ���� ����� �ʿ���� => �̶��� ���� �����͸� ����
			}
		}
	}

	if (ioInfo->totByte == 0 && ioInfo->recvByte >= 4) { // ����� �� �о�� ��Ż ����Ʈ ���� �� �� �ִ�
		ioInfo->totByte = min(ioInfo->bodySize + 12, SIZE);
	}
}

// Ŭ���̾�Ʈ���� ���� ������ ������ ����ü ����
char* DataCopy(LPPER_IO_DATA ioInfo, int *status) {
	copy(((char*)ioInfo->recvBuffer) + 4, ((char*)ioInfo->recvBuffer) + 8,
		(char*)status);
	//	copy(((char*) ioInfo->recvBuffer) + 8, ((char*) ioInfo->recvBuffer) + 12,
	//			(char*) direction);

	CharPool* charPool = CharPool::getInstance();
	char* msg = charPool->Malloc(); // char[ioInfo->bodySize];	// Msg

	copy(((char*)ioInfo->recvBuffer) + 12,
		((char*)ioInfo->recvBuffer) + 12 + ioInfo->bodySize, msg); //������ġ

	// �� ���� �޾����� �Ҵ� ����
	charPool->Free(ioInfo->recvBuffer);

	MPool* mp = MPool::getInstance();
	mp->Free(ioInfo);

	return msg;
}
// �۽��� ����� ������
unsigned WINAPI SendMsgThread(void *arg) {

	while (1) {
		Sleep(1);

		// ������ ť�� ��ȯ�޴´�
		EnterCriticalSection(&cs);
		queue<INFO_CLIENT> sendQueues = clientQueue.getSendQueue();
		LeaveCriticalSection(&cs);
		if (sendQueues.size() != 0) {
			Sleep(1);
			queue<INFO_CLIENT> recvQueues;
			while (!sendQueues.empty()) {
				INFO_CLIENT info = sendQueues.front();
				sendQueues.pop();

				SendMsg(info.Sock, info.message.c_str(),

					info.direction);
				recvQueues.push(info);
			}
			EnterCriticalSection(&cs);
			// �ٽ� �־���
			clientQueue.setSendQueue(sendQueues);

			clientQueue.setRecvQueue(recvQueues);
			LeaveCriticalSection(&cs);
		}
	}
	return 0;
}

// Ŭ���̾�Ʈ�� ����� ����� Send���� ���� ������
unsigned WINAPI MakeMsgThread(void *arg) {

	while (1) {
		Sleep(1);
		// �ޱ� ť�� ��ȯ�޴´�
		EnterCriticalSection(&cs);
		queue<INFO_CLIENT> recvQueues = clientQueue.getRecvQueue();
		LeaveCriticalSection(&cs);
		string alpha1 = "abcdefghijklmnopqrstuvwxyz";
		string alpha2 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		string password = "1234";

		if (recvQueues.size() != 0) {
			Sleep(1);
			queue<INFO_CLIENT> sendQueues;
			while (!recvQueues.empty()) {
				INFO_CLIENT info = recvQueues.front();
				recvQueues.pop();
				EnterCriticalSection(&cs);
				int cStatus =
					clientQueue.getClientMap().find(info.Sock)->second.clientStatus;
				string idName =
					clientQueue.getClientMap().find(info.Sock)->second.id;
				LeaveCriticalSection(&cs);
				if (cStatus == STATUS_LOGOUT) {
					// cout << "STATUS_LOGOUT " << endl;
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
					// cout << "STATUS_WAITING " << endl;

					int directionNum = (rand() % 10);
					if ((directionNum % 2) == 0) { // 0 2 4 6 8 ������

						int randNum1 = (rand() % 2);
						int randNum2 = (rand() % 4);
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
						int randNum2 = (rand() % 4);
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
					// cout << "STATUS_CHATTIG " << endl;
					int directionNum = (rand() % 20);
					string msg = "";
					if (directionNum < 19) {
						int randNum1 = (rand() % 2);

						for (int i = 0; i <= (rand() % 60) + 1; i++) {
							int randNum2 = (rand() % 26);
							if (randNum1 == 0) {
								msg += alpha1.at(randNum2);
							}
							else {
								msg += alpha2.at(randNum2);
							}
						}
					}
					else { // 20���� �ѹ� ����
						msg = "\\out";
					}
					info.direction = -1;
					info.message = msg;
				}
				sendQueues.push(info);
			}

			// �ٽ� �־���
			EnterCriticalSection(&cs);
			// cout << "MakeMsgThread end " << sendQueues.size() << endl;
			clientQueue.setRecvQueue(recvQueues);
			clientQueue.setSendQueue(sendQueues);
			LeaveCriticalSection(&cs);
		}
	}
	return 0;
}
// ������ ����� ������
unsigned WINAPI RecvMsgThread(LPVOID hComPort) {

	SOCKET sock;
	DWORD bytesTrans;
	LPPER_IO_DATA ioInfo;

	while (1) {
		GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD)&sock,
			(LPOVERLAPPED*)&ioInfo, INFINITE);

		if (READ_MORE == ioInfo->serverMode || READ == ioInfo->serverMode) {
			// ������ �б� ����
			PacketReading(ioInfo, bytesTrans);

			if ((ioInfo->recvByte < ioInfo->totByte)
				|| (ioInfo->recvByte < 4 && ioInfo->totByte == 0)) { // ���� ��Ŷ ���� || ��� �� ������ -> ���޾ƾ���

				RecvMore(sock, ioInfo); // ��Ŷ ���ޱ�
			}
			else {
				int status;
				char *msg = DataCopy(ioInfo, &status);

				// Client�� ���� ���� ���� �ʼ�
				// �������� �ذ����� ����
				if (status == STATUS_LOGOUT || status == STATUS_WAITING
					|| status == STATUS_CHATTIG) {

					if (clientQueue.getClientMap().find(sock)->second.clientStatus
						!= status) { // ���� ����� �ܼ� clear
						//	system("cls");
					}
					EnterCriticalSection(&cs);
					unordered_map<SOCKET, INFO_CLIENT> clientMap =
						clientQueue.getClientMap();
					clientMap.find(sock)->second.clientStatus = status; // clear���� client���º��� ���ش�
					clientQueue.setClientMap(clientMap);

					cout << "sock " << sock << " status " << status << endl;
					LeaveCriticalSection(&cs);
					// cout << msg << endl;
				}
				else if (status == STATUS_WHISPER) { // �ӼӸ� �����϶��� Ŭ���̾�Ʈ ���º�ȭ ����
					// cout << msg << endl;
				}
				CharPool* charPool = CharPool::getInstance();
				charPool->Free(msg);
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
	InitializeCriticalSectionAndSpinCount(&cs, 2000);
	// Completion Port ����
	HANDLE hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

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
		string alpha1 = "abcdefghijklmnopqrstuvwxyz";
		string alpha2 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		int j = rand() % 26;
		if (i / 26 == 0) {
			info.id = alpha1.at(j);
		}
		else {
			info.id = alpha2.at(j);
		}

		// Recv���� ����
		recvQueues.push(info);
		// userMap ä���
		userMap[clientSocket] = info;
		cout << "sock num " << clientSocket << endl;
		// �� socket�� Recv ��������
		Recv(clientSocket);
		Sleep(50);
	}
	// ��ü clientUser
	clientQueue.setClientMap(userMap);
	// Recvť�� ä����
	clientQueue.setRecvQueue(recvQueues);
	// Sendť�� ����·�
	clientQueue.setSendQueue(sendQueues);

	cout << "���� ���� Ŭ���̾�Ʈ" << clientCnt << endl;

	HANDLE sendThread, recvThread, makeThread;

	// ���� ������ ����
	// ������� RecvMsg�� hComPort CP ������Ʈ�� �Ҵ��Ѵ�
	// RecvMsg���� Recv�� �Ϸ�Ǹ� ������ �κ��� �ִ�
	recvThread = (HANDLE)_beginthreadex(NULL, 0, RecvMsgThread,
		(LPVOID)hComPort, 0,
		NULL);

	
	// ������ ������� ������ ����
	// ������� RecvMsg�� hComPort CP ������Ʈ�� �Ҵ��Ѵ�
	// RecvMsg���� Recv�� �Ϸ�Ǹ� ������ �κ��� �ִ�
	makeThread = (HANDLE)_beginthreadex(NULL, 0, MakeMsgThread,
		NULL, 0,
		NULL);
	
	// �۽� ������ ����
	// Thread�ȿ��� clientSocket���� Send���ٰŴϱ� ���ڷ� �Ѱ��ش�
	// CP�� Send�� ���� �ȵǾ����� GetQueuedCompletionStatus���� Send �Ϸ�ó�� �ʿ����
	sendThread = (HANDLE)_beginthreadex(NULL, 0, SendMsgThread,
		NULL, 0,
		NULL);

	WaitForSingleObject(recvThread, INFINITE);
	WaitForSingleObject(makeThread, INFINITE);
	WaitForSingleObject(sendThread, INFINITE);
	// �Ӱ迵�� Object ��ȯ
	DeleteCriticalSection(&cs);

	unordered_map<SOCKET, INFO_CLIENT>::iterator iter;

	for (iter = clientQueue.getClientMap().begin();
		iter != clientQueue.getClientMap().end(); ++iter) {
		closesocket(iter->first);
	}
	WSACleanup();
	return 0;
}
