//============================================================================
// Name        : Server.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <map>
#include <list>

using namespace std;

#define BUF_SIZE 512
#define NAME_SIZE 20

#define START 1
#define READ 2
#define WRITE 3

// Ŭ���̾�Ʈ ���� ���� => �������� �����Ұ�
#define STATUS_LOGOUT 1
#define STATUS_WAITING 2
#define STATUS_CHATTIG 3

// ������ �� ���� ����
#define ROOM_MAKE 1
#define ROOM_ENTER 2
#define ROOM_OUT 3

// ������ ������ ���� ����
// client ���Ͽ� �����ϴ� ��������
typedef struct { // socket info
	char userName[NAME_SIZE];
	char roomName[NAME_SIZE];
	int status;
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

// ���� ��� ��Ŷ ������
typedef struct {
	int clientStatus;
	char message[BUF_SIZE];
	int direction;
} PACKET_DATA, *P_PACKET_DATA;

// �񵿱� ��ſ� �ʿ��� ����ü
typedef struct { // buffer info
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	char buffer[sizeof(PACKET_DATA)];
	int serverMode;
} PER_IO_DATA, *LPPER_IO_DATA;

// ������ ������ ���� �ڷ� ����
map<SOCKET, LPPER_HANDLE_DATA> userMap;
// ������ �� ���� ����
map<string, list<SOCKET> > roomMap;

// �Ӱ迵���� �ʿ��� ������Ʈ
// WaitForSingleObject�� mutex�� non-singled�� ���� => ����
// ReleaseMutex��  mutex�� singled�� ���� => ��
HANDLE mutex;

// ���ο��� �޼��� ����
void SendToMeMsg(LPPER_IO_DATA ioInfo, char *msg, SOCKET mySock, int status) {

	// cout << "SendToMeMsg " << msg << endl;

	ioInfo = new PER_IO_DATA;

	memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
	P_PACKET_DATA packet = new PACKET_DATA;

	strcpy(packet->message, msg);
	packet->clientStatus = status;
	ioInfo->wsaBuf.buf = (char*) packet;
	ioInfo->wsaBuf.len = sizeof(PACKET_DATA);
	ioInfo->serverMode = WRITE;

	WSASend(mySock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);

	delete ioInfo;
	// delete packet;
}

// ���� ���� ����鿡�� �޼��� ����
void SendToRoomMsg(LPPER_IO_DATA ioInfo, char *msg, list<SOCKET> lists,
		int status) {

	// cout << "SendToRoomMsg " << msg << endl;

	ioInfo = new PER_IO_DATA;
	memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
	P_PACKET_DATA packet = new PACKET_DATA;

	strcpy(packet->message, msg);
	packet->clientStatus = status;
	ioInfo->wsaBuf.buf = (char*) packet;
	ioInfo->wsaBuf.len = sizeof(PACKET_DATA);
	ioInfo->serverMode = WRITE;

	WaitForSingleObject(mutex, INFINITE);
	list<SOCKET>::iterator iter;
	for (iter = lists.begin(); iter != lists.end(); iter++) {
		WSASend((*iter), &(ioInfo->wsaBuf), 1,
		NULL, 0, &(ioInfo->overlapped), NULL);
	}
	ReleaseMutex(mutex);

	delete ioInfo;
	// delete packet;
}

// ��ü ����鿡�� �޼��� ����
// ������ ��� ���� ���� ����
void SendToAllMsg(LPPER_IO_DATA ioInfo, char *msg, SOCKET mySock, int status) {

	cout << "SendToAllMsg " << msg << endl;

	ioInfo = new PER_IO_DATA;
	memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
	P_PACKET_DATA packet = new PACKET_DATA;

	strcpy(packet->message, msg);
	packet->clientStatus = status;
	ioInfo->wsaBuf.buf = (char*) packet;
	ioInfo->wsaBuf.len = sizeof(PACKET_DATA);
	ioInfo->serverMode = WRITE;

	WaitForSingleObject(mutex, INFINITE);
	map<SOCKET, LPPER_HANDLE_DATA>::iterator iter;
	for (iter = userMap.begin(); iter != userMap.end(); iter++) {
		if (iter->first == mySock) { // ���� ����
			continue;
		} else {
			WSASend(iter->first, &(ioInfo->wsaBuf), 1, NULL, 0,
					&(ioInfo->overlapped), NULL);
		}
	}
	ReleaseMutex(mutex);

	// delete packet;
	delete ioInfo;
}

// �ʱ� ����
void InitUser(P_PACKET_DATA packet, LPPER_IO_DATA ioInfo, SOCKET sock) {

	char msg[BUF_SIZE];
	char name[NAME_SIZE];
	DWORD flags = 0;
	packet = new PACKET_DATA;

	memcpy(packet, ioInfo->buffer, sizeof(PACKET_DATA));
	strcpy(name, packet->message);
	LPPER_HANDLE_DATA handleInfo = new PER_HANDLE_DATA;
	cout << "START user : " << name << endl;
	cout << "sock : " << sock << endl;
	// ������ ���� ���� �ʱ�ȭ
	ioInfo = new PER_IO_DATA;
	handleInfo->status = STATUS_WAITING;
	strcpy(handleInfo->userName, name);
	strcpy(handleInfo->roomName, "");
	WaitForSingleObject(mutex, INFINITE);
	userMap.insert(pair<SOCKET, LPPER_HANDLE_DATA>(sock, handleInfo));
	cout << "���� ���� �ο� �� : " << userMap.size() << endl;

	// map<SOCKET, LPPER_HANDLE_DATA>::iterator iter;
	// for (iter = userMap.begin(); iter != userMap.end(); ++iter) {
	// 	cout << "������ �̸� : " << (*iter).second->userName << (*iter).first
	//	 		<< endl;
	// }

	ReleaseMutex(mutex);

	delete ioInfo;

	strcpy(msg, "������ ȯ���մϴ�!\n");
	strcat(msg, "1.�� ���� ����, 2.�� ����� 3.�� �����ϱ�");
	SendToMeMsg(ioInfo, msg, sock, STATUS_WAITING);

	ioInfo = new PER_IO_DATA;

	memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
	ioInfo->wsaBuf.len = sizeof(PACKET_DATA);
	ioInfo->wsaBuf.buf = ioInfo->buffer;
	ioInfo->serverMode = READ;

	WSARecv(sock, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped),
	NULL);

}

// Server ��ǻ��  CPU ������ŭ ������ �����ɰ�
unsigned WINAPI HandleThread(LPVOID pCompPort) {

	HANDLE hComPort = (HANDLE) pCompPort;
	SOCKET sock;
	DWORD bytesTrans;
	// LPPER_HANDLE_DATA handleInfo;
	LPPER_IO_DATA ioInfo;

	P_PACKET_DATA packet;

	while (true) {

		GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD) &sock,
				(LPOVERLAPPED*) &ioInfo, INFINITE);

		if (START == ioInfo->serverMode) {

			InitUser(packet, ioInfo, sock);

		} else if (READ == ioInfo->serverMode) {
			cout << "message received" << endl;

			// IO �Ϸ��� ���� �κ�
			char name[BUF_SIZE];
			char msg[BUF_SIZE];
			DWORD flags = 0;
			int direction = 0;

			// ��Ŷ�� �ް� �̸��� �޼����� �ʱ� ����
			packet = new PACKET_DATA;
			memcpy(packet, ioInfo->buffer, sizeof(PACKET_DATA));

			strcpy(name, userMap.find(sock)->second->userName);
			strcpy(msg, packet->message);

			if (bytesTrans == 0 || strcmp(ioInfo->buffer, "out") == 0) { // ���� ����
				char roomName[NAME_SIZE];

				strcpy(roomName, userMap.find(sock)->second->roomName);
				// ���̸� �ӽ� ����
				if (strcmp(roomName, "") != 0) { // �濡 �������� ���
					// ������ ��� ���� out
					(roomMap.find(roomName)->second).remove(sock);

					if ((roomMap.find(roomName)->second).size() == 0) { // ���ο� 0���̸� �� ����
						roomMap.erase(roomName);
					} else {
						char sendMsg[NAME_SIZE + BUF_SIZE + 4];
						strcpy(sendMsg, name);
						strcat(sendMsg, " ���� �������ϴ�!");

						SendToRoomMsg(ioInfo, sendMsg,
								roomMap.find(
										userMap.find(sock)->second->roomName)->second,
								STATUS_CHATTIG);
					}
				}

				WaitForSingleObject(mutex, INFINITE);
				userMap.erase(sock); // ���� ���� ���� ����
				cout << "���� ���� �ο� �� : " << userMap.size() << endl;
				ReleaseMutex(mutex);
				closesocket(sock);

				// delete ioInfo;
				continue;
			}

			int nowStatus = (userMap.find(sock))->second->status;
			direction = packet->direction;

			if (nowStatus == STATUS_WAITING) { // ���� ���̽�
				if (direction == ROOM_MAKE) { // ���ο� �� ���鶧
					// cout << "ROOM_MAKE" << endl;

					// ��ȿ�� ���� ����
					if (roomMap.count(msg) != 0) { // ���̸� �ߺ�
						strcpy(msg, "�̹� �ִ� �� �̸��Դϴ�!\n");
						strcat(msg, "1.�� ���� ����, 2.�� ����� 3.�� �����ϱ�");
						SendToMeMsg(ioInfo, msg, sock, STATUS_WAITING);
						// �ߺ� ���̽��� �� ���� �� ����
					} else { // ���̸� �ߺ� �ƴ� ���� ����

						WaitForSingleObject(mutex, INFINITE);
						// ���ο� �� ���� ����
						list<SOCKET> chatList;
						chatList.push_back(sock);
						roomMap.insert(
								pair<string, list<SOCKET> >(msg, chatList));

						// User�� ���� ���� �ٲ۴�
						strcpy((userMap.find(sock))->second->roomName, msg);
						(userMap.find(sock))->second->status = STATUS_CHATTIG;

						ReleaseMutex(mutex);
						strcat(msg, " ���� �����Ǿ����ϴ�. �����÷��� out�� �Է��ϼ���");
						cout << "���� ������ �� ���� : " << roomMap.size() << endl;
						SendToMeMsg(ioInfo, msg, sock, STATUS_CHATTIG);
					}

				} else if (direction == ROOM_ENTER) { // �� ���� ��û
					// cout << "ROOM_ENTER" << endl;

					// ��ȿ�� ���� ����
					if (roomMap.count(msg) == 0) { // �� ��ã��
						strcpy(msg, "���� �� �Դϴ�!\n");
						strcat(msg, "1.�� ���� ����, 2.�� ����� 3.�� �����ϱ�");
						SendToMeMsg(ioInfo, msg, sock, STATUS_WAITING);
					} else {
						WaitForSingleObject(mutex, INFINITE);

						roomMap.find(msg)->second.push_back(sock);
						strcpy(userMap.find(sock)->second->roomName, msg);
						userMap.find(sock)->second->status = STATUS_CHATTIG;

						ReleaseMutex(mutex);

						char sendMsg[NAME_SIZE + BUF_SIZE + 4];
						strcpy(sendMsg, name);
						strcat(sendMsg, " ���� �����ϼ̾����ϴ� �����÷��� out�� �Է��ϼ���");
						SendToRoomMsg(ioInfo, sendMsg,
								roomMap.find(msg)->second,
								STATUS_CHATTIG);
					}

				} else if (strcmp(msg, "1") == 0) { // �� ���� ��û��

					WaitForSingleObject(mutex, INFINITE);
					map<string, list<SOCKET> >::iterator iter;
					char str[BUF_SIZE];
					if (roomMap.size() == 0) {
						strcpy(str, "������� ���� �����ϴ�\n");
					} else {
						strcpy(str, "�� ���� ����Ʈ\n");
						// �������� ���ڿ��� �����
						for (iter = roomMap.begin(); iter != roomMap.end();
								iter++) {
							char num[2];
							strcat(str, (iter->first).c_str());
							strcat(str, " : ");
							sprintf(num, "%d", (iter->second).size());
							strcat(str, num);
							strcat(str, " �� ");
							strcat(str, "\n");
						}
					}
					ReleaseMutex(mutex);
					SendToMeMsg(ioInfo, str, sock, STATUS_WAITING);
				}
			} else if (nowStatus == STATUS_CHATTIG) { // ä�� �� ���̽�

				if (bytesTrans == 0 || strcmp(msg, "out") == 0
						|| strcmp(msg, "Q") == 0 || strcmp(msg, "q") == 0) { // ä�ù� ����
						// cout << "ä�ù� ������ " << endl;

					char sendMsg[NAME_SIZE + BUF_SIZE + 4];
					strcpy(sendMsg, name);
					strcat(sendMsg, " ���� �������ϴ�!");

					SendToRoomMsg(ioInfo, sendMsg,
							roomMap.find(userMap.find(sock)->second->roomName)->second,
							STATUS_CHATTIG);
					char roomName[NAME_SIZE];
					// �Ӱ迵�� �����ص� �ɰ� ����
					WaitForSingleObject(mutex, INFINITE);

					strcpy(roomName, userMap.find(sock)->second->roomName);
					// ���̸� �ӽ� ����
					strcpy(userMap.find(sock)->second->roomName, "");
					userMap.find(sock)->second->status = STATUS_WAITING;

					ReleaseMutex(mutex);
					// �Ӱ迵�� �����ص� �ɰ� ����

					strcpy(msg, "1.�� ���� ����, 2.�� ����� 3.�� �����ϱ�");
					SendToMeMsg(ioInfo, msg, sock, STATUS_WAITING);
					cout << "������ ��� name : " << name << endl;
					cout << "������ �� name : " << roomName << endl;

					// ������ ��� ���� out
					(roomMap.find(roomName)->second).remove(sock);

					if ((roomMap.find(roomName)->second).size() == 0) { // ���ο� 0���̸� �� ����
						roomMap.erase(roomName);
					}
				} else { // ä�ù濡�� ä����
					cout << "ä�ù濡 �ִ��� " << endl;
					char sendMsg[NAME_SIZE + BUF_SIZE + 4];
					strcpy(sendMsg, name);
					strcat(sendMsg, " : ");
					strcat(sendMsg, msg);
					SendToRoomMsg(ioInfo, sendMsg,
							roomMap.find(userMap.find(sock)->second->roomName)->second,
							STATUS_CHATTIG);
				}

			}

			delete ioInfo;

			ioInfo = new PER_IO_DATA;
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = sizeof(PACKET_DATA);
			ioInfo->wsaBuf.buf = ioInfo->buffer;
			ioInfo->serverMode = READ;

			// ��� Recv
			WSARecv(sock, &(ioInfo->wsaBuf), 1, NULL, &flags,
					&(ioInfo->overlapped), NULL);
		} else {
			cout << "message send" << endl;

		}
	}
	return 0;
}

int main(int argc, char* argv[]) {

	WSADATA wsaData;
	HANDLE hComPort;
	SYSTEM_INFO sysInfo;
	LPPER_IO_DATA ioInfo;
	LPPER_HANDLE_DATA handleInfo;

	SOCKET hServSock;
	SOCKADDR_IN servAdr;
	int recvBytes, i, flags = 0;
	if (argc != 2) {
		cout << "Usage : " << argv[0] << endl;
		exit(1);
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("WSAStartup() error!");
		exit(1);
	}

	// Completion Port ����
	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	GetSystemInfo(&sysInfo);

	// CPU ���� ��ŭ ������ ����
	int process = sysInfo.dwNumberOfProcessors;
	cout << "Server CPU num : " << process << endl;
	for (i = 0; i < process; i++) {
		_beginthreadex(NULL, 0, HandleThread, (LPVOID) hComPort, 0, NULL);
	}
	// Overlapped IO���� ������ �����
	// TCP ����Ұ�
	hServSock = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0,
	WSA_FLAG_OVERLAPPED);

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = PF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(atoi(argv[1]));

	if (bind(hServSock, (SOCKADDR*) &servAdr, sizeof(servAdr)) == SOCKET_ERROR) {
		printf("bind() error!");
		exit(1);
	}

	if (listen(hServSock, 5) == SOCKET_ERROR) {
		printf("listen() error!");
		exit(1);
	}

	// �Ӱ迵�� Object ����
	mutex = CreateMutex(NULL, FALSE, NULL);
	cout << "Server ready listen" << endl;
	cout << "port number : " << argv[1] << endl;

	while (true) {
		SOCKET hClientSock;
		SOCKADDR_IN clntAdr;
		int addrLen = sizeof(clntAdr);
		cout << "accept wait" << endl;
		hClientSock = accept(hServSock, (SOCKADDR*) &clntAdr, &addrLen);

		handleInfo = new PER_HANDLE_DATA;

		cout << "Connected client IP " << inet_ntoa(clntAdr.sin_addr) << endl;

		// Completion Port �� ���� ����
		CreateIoCompletionPort((HANDLE) hClientSock, hComPort,
				(DWORD) hClientSock, 0);

		ioInfo = new PER_IO_DATA;
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));

		ioInfo->wsaBuf.len = sizeof(PACKET_DATA);
		ioInfo->wsaBuf.buf = ioInfo->buffer;
		ioInfo->serverMode = START;

		WSARecv(hClientSock, &(ioInfo->wsaBuf), 1, (LPDWORD) &recvBytes,
				(LPDWORD) &flags, &(ioInfo->overlapped),
				NULL);

		delete handleInfo;
	}

	// �Ӱ迵�� Object ��ȯ
	CloseHandle(mutex);
	return 0;
}
