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
#include <map>
#include <list>
#include "common.h"

using namespace std;

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

// ������ ��й�ȣ�� ���� �ڷᱸ��
map<string, USER_DATA> idMap;
// ������ ������ ���� �ڷ� ����
map<SOCKET, LPPER_HANDLE_DATA> userMap;
// ������ �� ���� ����
map<string, list<SOCKET> > roomMap;

// �Ӱ迵���� �ʿ��� ��ü
// Ŀ�θ�� �ƴ϶� ���������� ����ȭ ����� ��
// �� ���μ������� ����ȭ �̹Ƿ� ũ��Ƽ�ü��� ���
CRITICAL_SECTION cs;

// �Ѹ��� �޼��� ����
void SendToOneMsg(char *msg, SOCKET mySock, int status) {

	LPPER_IO_DATA ioInfo = new PER_IO_DATA;

	memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
	P_PACKET_DATA packet = new PACKET_DATA;

	strncpy(packet->message, msg, BUF_SIZE);
	packet->clientStatus = status;
	ioInfo->wsaBuf.buf = (char*) packet;
	ioInfo->wsaBuf.len = sizeof(PACKET_DATA);
	ioInfo->serverMode = WRITE; // GetQueuedCompletionStatus ���� �бⰡ Send�� ���� �ְ�

	WSASend(mySock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);

}

// ���� ���� ����鿡�� �޼��� ����
void SendToRoomMsg(char *msg, list<SOCKET> &lists, int status) {

	LPPER_IO_DATA ioInfo = new PER_IO_DATA;
	memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
	P_PACKET_DATA packet = new PACKET_DATA;

	strncpy(packet->message, msg, BUF_SIZE);
	packet->clientStatus = status;
	ioInfo->wsaBuf.buf = (char*) packet;
	ioInfo->wsaBuf.len = sizeof(PACKET_DATA);
	ioInfo->serverMode = WRITE;  // GetQueuedCompletionStatus ���� �бⰡ Send�� ���� �ְ�

	list<SOCKET>::iterator iter;
	EnterCriticalSection(&cs);
	for (iter = lists.begin(); iter != lists.end(); iter++) {
		WSASend((*iter), &(ioInfo->wsaBuf), 1,
		NULL, 0, &(ioInfo->overlapped), NULL);
	}
	LeaveCriticalSection(&cs);

}

// Recv �����Լ�
void Recv(SOCKET sock, DWORD recvBytes, DWORD flags) {
	LPPER_IO_DATA ioInfo = new PER_IO_DATA;
	memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
	ioInfo->wsaBuf.len = sizeof(PACKET_DATA);
	ioInfo->wsaBuf.buf = ioInfo->buffer;
	ioInfo->serverMode = READ; // GetQueuedCompletionStatus ���� �бⰡ Recv�� ���� �ְ�

	// ��� Recv
	WSARecv(sock, &(ioInfo->wsaBuf), 1, &recvBytes, &flags,
			&(ioInfo->overlapped),
			NULL);
}

// �ʱ� �α���
// �������� �߰�
void InitUser(P_PACKET_DATA packet, SOCKET sock) {

	char msg[BUF_SIZE];
	char name[NAME_SIZE];

	strncpy(name, idMap.find(packet->message)->second.nickname, NAME_SIZE);

	LPPER_HANDLE_DATA handleInfo = new PER_HANDLE_DATA;
	cout << "START user : " << name << endl;
	cout << "sock : " << sock << endl;
	// ������ ���� ���� �ʱ�ȭ
	// ioInfo = new PER_IO_DATA;
	handleInfo->status = STATUS_WAITING;

	strncpy(handleInfo->userId, packet->message, NAME_SIZE);
	strncpy(handleInfo->userName, name, NAME_SIZE);
	strncpy(handleInfo->roomName, "", NAME_SIZE);
	EnterCriticalSection(&cs);
	// �������� insert
	userMap.insert(pair<SOCKET, LPPER_HANDLE_DATA>(sock, handleInfo));

	// id�� key���� ���� �̸��� �ƴ϶� ���̵�!!
	idMap.find(packet->message)->second.logMode = NOW_LOGIN;

	cout << "���� ���� �ο� �� : " << userMap.size() << endl;

	LeaveCriticalSection(&cs);

	strncpy(msg, "������ ȯ���մϴ�!\n", BUF_SIZE);
	strcat(msg, waitRoomMessage);
	SendToOneMsg(msg, sock, STATUS_WAITING);

}

// ���� �������� ����
void ClientExit(SOCKET sock) {

	if (closesocket(sock) != SOCKET_ERROR) {
		cout << "���� ���� " << endl;

		if (userMap.find(sock) != userMap.end()) {

			// �������� �������� => �������� �� ���� ���� �ʿ�
			char roomName[NAME_SIZE];
			char name[NAME_SIZE];
			char id[NAME_SIZE];

			strncpy(roomName, userMap.find(sock)->second->roomName, NAME_SIZE);
			strncpy(name, userMap.find(sock)->second->userName, NAME_SIZE);
			strncpy(id, userMap.find(sock)->second->userId, NAME_SIZE);
			// �α��� ���� ���󺹱�
			idMap.find(id)->second.logMode = NOW_LOGOUT;

			// ���̸� �ӽ� ����
			if (strcmp(roomName, "") != 0) { // �濡 �������� ���
				// ������ ��� ���� out

				if (roomMap.find(roomName) != roomMap.end()) { // null üũ �켱
					(roomMap.find(roomName)->second).remove(sock);

					if ((roomMap.find(roomName)->second).size() == 0) { // ���ο� 0���̸� �� ����
						EnterCriticalSection(&cs);
						roomMap.erase(roomName);
						LeaveCriticalSection(&cs);
					} else {
						char sendMsg[BUF_SIZE];
						// �̸����� ���� NAME_SIZE����
						strncpy(sendMsg, name, NAME_SIZE);
						strcat(sendMsg, " ���� �������ϴ�!");

						if (roomMap.find(userMap.find(sock)->second->roomName)
								!= roomMap.end()) { // null �˻�
							SendToRoomMsg(sendMsg,
									roomMap.find(
											userMap.find(sock)->second->roomName)->second,
									STATUS_CHATTIG);
						}
					}
				}
			}
			EnterCriticalSection(&cs);
			userMap.erase(sock); // ���� ���� ���� ����
			LeaveCriticalSection(&cs);
			cout << "���� ���� �ο� �� : " << userMap.size() << endl;
		}

	}

}

// �α��� ���� ����ó��
// ���ǰ� ���� �� ����
void StatusLogout(SOCKET sock, P_PACKET_DATA packet) {

	if (packet->clientStatus == STATUS_LOGOUT) {

		// memcpy �κ� ����
		char msg[BUF_SIZE];

		if (packet->direction == USER_MAKE) { // 1�� ��������
			cout << "USER MAKE" << endl;
			// ��ȿ�� ���� �ʿ�
			EnterCriticalSection(&cs);
			if (idMap.find(packet->message) == idMap.end()) { // ID �ߺ�üũ

				USER_DATA userData;
				userData.password = string(packet->message2);
				strncpy(userData.nickname, packet->message3, NAME_SIZE);
				userData.logMode = NOW_LOGOUT;

				idMap.insert(
						pair<string, USER_DATA>(string(packet->message),
								userData));

				strncpy(msg, packet->message, BUF_SIZE);
				strcat(msg, " ���� ���� �Ϸ�!\n");
				strcat(msg, loginBeforeMessage);
				SendToOneMsg(msg, sock, STATUS_LOGOUT);
			} else { // ID�ߺ�����
				strncpy(msg, "�ߺ��� ���̵� �ֽ��ϴ�!\n", BUF_SIZE);
				strcat(msg, loginBeforeMessage);
				SendToOneMsg(msg, sock, STATUS_LOGOUT);
			}
			LeaveCriticalSection(&cs);
		} else if (packet->direction == USER_ENTER) { // 2�� �α��� �õ�
			cout << "USER ENTER" << endl;
			if (idMap.find(packet->message) == idMap.end()) { // ���� ����
				strncpy(msg, "���� �����Դϴ� ���̵� Ȯ���ϼ���!\n", BUF_SIZE);
				strcat(msg, loginBeforeMessage);
				SendToOneMsg(msg, sock, STATUS_LOGOUT);
			} else if (idMap.find(packet->message)->second.password.compare(
					packet->message2) == 0) { // ��й�ȣ ��ġ

				if (idMap.find(packet->message)->second.logMode == NOW_LOGIN) { // �ߺ��α��� ��ȿ�� �˻�
					strncpy(msg, "�ߺ� �α����� �ȵ˴ϴ�!\n", BUF_SIZE);
					strcat(msg, loginBeforeMessage);
					SendToOneMsg(msg, sock, STATUS_LOGOUT);
				} else { // �ߺ��α��� X
					InitUser(packet, sock); // �������� ����
				}

			} else { // ��й�ȣ Ʋ��
				strncpy(msg, "��й�ȣ Ʋ��!\n", BUF_SIZE);
				strcat(msg, loginBeforeMessage);
				SendToOneMsg(msg, sock, STATUS_LOGOUT);
			}

		} else if (packet->direction == USER_LIST) { // ������ ���� ��û
			cout << "USER LIST" << endl;
			char userList[BUF_SIZE];
			strncpy(userList, "���̵� ���� ����Ʈ", BUF_SIZE);
			if (idMap.size() == 0) {
				strcat(userList, " ����");
			} else {
				map<string, USER_DATA>::iterator iter;
				for (iter = idMap.begin(); iter != idMap.end(); ++iter) {
					strcat(userList, "\n");
					strcat(userList, (iter->first).c_str());
				}
			}

			SendToOneMsg(userList, sock, STATUS_LOGOUT);
		} else { // �׿� ��ɾ� �Է�
			char sendMsg[BUF_SIZE];

			strncpy(sendMsg, errorMessage, BUF_SIZE);
			strcat(sendMsg, loginBeforeMessage);
			// ������ �����̹Ƿ� STATUS_WAITING ���·� �����Ѵ�
			SendToOneMsg(sendMsg, sock, STATUS_LOGOUT);
		}
	}

}

// ���ǿ����� ���� ó��
// ���ǰ� ����
void StatusWait(SOCKET sock, P_PACKET_DATA packet) {

	char name[BUF_SIZE];
	char msg[BUF_SIZE];
	int direction = packet->direction;
	// ���ǿ��� �̸� ���� ����
	strncpy(name, userMap.find(sock)->second->userName, BUF_SIZE);
	strncpy(msg, packet->message, BUF_SIZE);
	if (direction == ROOM_MAKE) { // ���ο� �� ���鶧
		cout << "ROOM MAKE" << endl;
		// ��ȿ�� ���� ����
		if (roomMap.count(msg) != 0) { // ���̸� �ߺ�
			strncpy(msg, "�̹� �ִ� �� �̸��Դϴ�!\n", BUF_SIZE);
			strcat(msg, waitRoomMessage);
			SendToOneMsg(msg, sock, STATUS_WAITING);
			// �ߺ� ���̽��� �� ���� �� ����
		} else { // ���̸� �ߺ� �ƴ� ���� ����

			// ���ο� �� ���� ����
			list<SOCKET> chatList;
			chatList.push_back(sock);
			EnterCriticalSection(&cs);
			roomMap.insert(pair<string, list<SOCKET> >(msg, chatList));

			// User�� ���� ���� �ٲ۴�
			strncpy((userMap.find(sock))->second->roomName, msg, NAME_SIZE);
			(userMap.find(sock))->second->status =
			STATUS_CHATTIG;

			LeaveCriticalSection(&cs);
			strcat(msg, " ���� �����Ǿ����ϴ�.");
			strcat(msg, chatRoomMessage);
			cout << "���� ������ �� ���� : " << roomMap.size() << endl;
			SendToOneMsg(msg, sock, STATUS_CHATTIG);
		}

	} else if (direction == ROOM_ENTER) { // �� ���� ��û
		cout << "ROOM ENTER" << endl;

		// ��ȿ�� ���� ����
		if (roomMap.count(msg) == 0) { // �� ��ã��
			strncpy(msg, "���� �� �Դϴ�!\n", sizeof("���� �� �Դϴ�!\n"));
			strcat(msg, waitRoomMessage);
			SendToOneMsg(msg, sock, STATUS_WAITING);
		} else {
			EnterCriticalSection(&cs);

			if (roomMap.find(msg) != roomMap.end()) {
				roomMap.find(msg)->second.push_back(sock);
			}

			strncpy(userMap.find(sock)->second->roomName, msg, NAME_SIZE);
			userMap.find(sock)->second->status = STATUS_CHATTIG;

			LeaveCriticalSection(&cs);

			char sendMsg[BUF_SIZE];
			strncpy(sendMsg, name, BUF_SIZE);
			strcat(sendMsg, " ���� �����ϼ̽��ϴ�. ");
			strcat(sendMsg, chatRoomMessage);

			if (roomMap.find(msg) != roomMap.end()) { // ������ �˻�

				SendToRoomMsg(sendMsg, roomMap.find(msg)->second,
				STATUS_CHATTIG);

			}
		}

	} else if (strcmp(msg, "1") == 0) { // �� ���� ��û��

		map<string, list<SOCKET> >::iterator iter;
		char str[BUF_SIZE];
		if (roomMap.size() == 0) {
			strncpy(str, "������� ���� �����ϴ�", BUF_SIZE);
		} else {
			strncpy(str, "�� ���� ����Ʈ", BUF_SIZE);
			EnterCriticalSection(&cs);
			// �������� ���ڿ��� �����
			for (iter = roomMap.begin(); iter != roomMap.end(); iter++) {
				char num[4];
				strcat(str, "\n");
				strcat(str, iter->first.c_str());
				strcat(str, " : ");
				sprintf(num, "%d", (iter->second).size());
				strcat(str, num);
				strcat(str, "��");
			}
			LeaveCriticalSection(&cs);
		}

		SendToOneMsg(str, sock, STATUS_WAITING);
	} else if (strcmp(msg, "4") == 0) { // ���� ���� ��û��
		char str[BUF_SIZE];

		map<SOCKET, LPPER_HANDLE_DATA>::iterator iter;
		strncpy(str, "���� ���� ����Ʈ", BUF_SIZE);
		// ���� ������ ���ڿ��� �����
		EnterCriticalSection(&cs);
		for (iter = userMap.begin(); iter != userMap.end(); iter++) {
			strcat(str, "\n");
			strcat(str, (iter->second)->userName);
			strcat(str, " : ");
			if ((iter->second)->status == STATUS_WAITING) {
				strcat(str, "���� ");
			} else {
				strcat(str, (iter->second)->roomName);
			}
		}
		LeaveCriticalSection(&cs);
		SendToOneMsg(str, sock, STATUS_WAITING);
	} else if (direction == WHISPER) { // �ӼӸ�
		cout << "WHISPER " << endl;
		strncpy(name, packet->message2, NAME_SIZE);

		map<SOCKET, LPPER_HANDLE_DATA>::iterator iter;
		char sendMsg[BUF_SIZE];

		if (strcmp(name, userMap.find(sock)->second->userName) == 0) { // ���ο��� ����
			strncpy(sendMsg, "�ڽſ��� ������ ������ �����ϴ�\n", BUF_SIZE);
			strcat(sendMsg, waitRoomMessage);
			SendToOneMsg(sendMsg, sock, STATUS_WAITING);
		} else {
			bool find = false;
			EnterCriticalSection(&cs);
			for (iter = userMap.begin(); iter != userMap.end(); iter++) {
				if (strcmp(iter->second->userName, name) == 0) {
					find = true;

					strncpy(sendMsg, userMap.find(sock)->second->userName,
					BUF_SIZE);
					strcat(sendMsg, " �Կ��� �� �ӼӸ� : ");
					strcat(sendMsg, msg);
					SendToOneMsg(sendMsg, iter->first, STATUS_WHISPER);
					break;
				}
			}
			LeaveCriticalSection(&cs);
			// �ӼӸ� ����� ��ã��
			if (!find) {
				strncpy(sendMsg, name, BUF_SIZE);
				strcat(sendMsg, " ���� ã�� �� �����ϴ�");
				SendToOneMsg(sendMsg, sock, STATUS_WAITING);
			}
		}

	} else if (strcmp(msg, "6") == 0) { // �α׾ƿ�
		EnterCriticalSection(&cs);

		string userId = idMap.find(userMap.find(sock)->second->userId)->first;
		userMap.erase(sock); // ���� ���� ���� ����
		idMap.find(userId)->second.logMode = NOW_LOGOUT;
		LeaveCriticalSection(&cs);
		char sendMsg[BUF_SIZE];

		strncpy(sendMsg, loginBeforeMessage, BUF_SIZE);
		SendToOneMsg(sendMsg, sock, STATUS_LOGOUT);
	} else { // �׿� ��ɾ� �Է�
		char sendMsg[BUF_SIZE];

		strncpy(sendMsg, errorMessage, BUF_SIZE);
		strcat(sendMsg, waitRoomMessage);
		// ������ �����̹Ƿ� STATUS_WAITING ���·� �����Ѵ�
		SendToOneMsg(sendMsg, sock, STATUS_WAITING);
	}
}

// ä�ù濡���� ���� ó��
// ���ǰ� ����
void StatusChat(SOCKET sock, P_PACKET_DATA packet) {

	char name[NAME_SIZE];
	char msg[BUF_SIZE];

	// ���ǿ��� �̸� ���� ����
	strncpy(name, userMap.find(sock)->second->userName, NAME_SIZE);
	strncpy(msg, packet->message, BUF_SIZE);

	if (strcmp(msg, "\\out") == 0) { // ä�ù� ����

		char sendMsg[BUF_SIZE];
		char roomName[NAME_SIZE];
		strncpy(sendMsg, name, BUF_SIZE);
		strcat(sendMsg, " ���� �������ϴ�!");

		if (roomMap.find(userMap.find(sock)->second->roomName)
				!= roomMap.end()) { // null �˻�

			SendToRoomMsg(sendMsg,
					roomMap.find(userMap.find(sock)->second->roomName)->second,
					STATUS_CHATTIG);

			strncpy(roomName, userMap.find(sock)->second->roomName,
			NAME_SIZE);
			// ���̸� �ӽ� ����
			strncpy(userMap.find(sock)->second->roomName, "", NAME_SIZE);
			userMap.find(sock)->second->status = STATUS_WAITING;
		}

		strncpy(msg, waitRoomMessage, BUF_SIZE);
		SendToOneMsg(msg, sock, STATUS_WAITING);
		cout << "������ ��� name : " << name << endl;
		cout << "������ �� name : " << roomName << endl;

		// ������ ��� ���� out
		EnterCriticalSection(&cs);
		(roomMap.find(roomName)->second).remove(sock);

		if ((roomMap.find(roomName)->second).size() == 0) { // ���ο� 0���̸� �� ����
			roomMap.erase(roomName);
		}
		LeaveCriticalSection(&cs);
	} else { // ä�ù濡�� ä����

		char sendMsg[BUF_SIZE];
		strncpy(sendMsg, name, BUF_SIZE);
		strcat(sendMsg, " : ");
		strcat(sendMsg, msg);

		if (roomMap.find(userMap.find(sock)->second->roomName)
				!= roomMap.end()) { // null �˻�

			SendToRoomMsg(sendMsg,
					roomMap.find(userMap.find(sock)->second->roomName)->second,
					STATUS_CHATTIG);
		}
	}

}

// Server ��ǻ��  CPU ������ŭ ������ �����ɰ�
// ���� ������ �� �Լ����� ó��
unsigned WINAPI HandleThread(LPVOID pCompPort) {

	// Completion port��ü
	HANDLE hComPort = (HANDLE) pCompPort;
	SOCKET sock;
	DWORD bytesTrans;
	// LPPER_HANDLE_DATA handleInfo;
	LPPER_IO_DATA ioInfo;
	P_PACKET_DATA packet;

	while (true) {

		GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD) &sock,
				(LPOVERLAPPED*) &ioInfo, INFINITE);

		if (READ == ioInfo->serverMode) { // Recv �������
			cout << "message received" << endl;
			cout << "bytesTrans : " << bytesTrans << endl;
			// IO �Ϸ��� ���� �κ�
			DWORD recvBytes = 0;
			DWORD flags = 0;
			// ��Ŷ�� �ް� �̸��� �޼����� �ʱ� ����
			packet = new PACKET_DATA;
			memcpy(packet, ioInfo->buffer, sizeof(PACKET_DATA));

			if (bytesTrans == 0) { // ���� ���� �ܼ� ���� ����
				// �ܼ� �������� ó��
				ClientExit(sock);

			} else if (userMap.find(sock) == userMap.end()) { // ���ǰ� ���� => �α��� ���� �б�
					// �α��� ���� ���� ó��
				StatusLogout(sock, packet);

				// Recv�� ����Ѵ�
				Recv(sock, recvBytes, flags);

				// ���ǰ� ���� => �α��� ���� �б� ��
			} else { // ���ǰ� ������ => ���� �Ǵ� ä�ù� ����

				int status = userMap.find(sock)->second->status;

				if (status == STATUS_WAITING) { // ���� ���̽�
					// ���� ó�� �Լ�
					StatusWait(sock, packet);
				} else if (status == STATUS_CHATTIG) { // ä�� �� ���̽�
					// ä�ù� ó�� �Լ�
					StatusChat(sock, packet);
				}

				// Recv�� ����Ѵ�
				Recv(sock, recvBytes, flags);
			}
		} else { // Send �������
			cout << "message send" << endl;
		}
	}
	return 0;
}

int main(int argc, char* argv[]) {

	WSADATA wsaData;
	HANDLE hComPort;
	SYSTEM_INFO sysInfo;
	LPPER_HANDLE_DATA handleInfo;

	SOCKET hServSock;
	SOCKADDR_IN servAdr;
	DWORD recvBytes = 0;
	DWORD flags = 0;
	if (argc != 2) {
		cout << "Usage : " << argv[0] << endl;
		exit(1);
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "WSAStartup() error!" << endl;
		exit(1);
	}

	// Completion Port ����
	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	GetSystemInfo(&sysInfo);

	int process = sysInfo.dwNumberOfProcessors;
	cout << "Server CPU num : " << process << endl;

	// CPU ���� ��ŭ ������ ����
	// Thread Pool �����带 �ʿ��� ��ŭ ����� ���� �ı� ���ϰ� ���
	for (int i = 0; i < process; i++) {
		// ������� HandleThread�� hComPort CP ������Ʈ�� �Ҵ��Ѵ�
		_beginthreadex(NULL, 0, HandleThread, (LPVOID) hComPort, 0, NULL);
		// ù��°�� security���� NULL �����
		// �ټ���° 0�� ������ ��ٷ� ���� �ǹ�
		// ������°�� ������ ���̵�
	}

	// Overlapped IO���� ������ �����
	// TCP ����Ұ�
	hServSock = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0,
	WSA_FLAG_OVERLAPPED);

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = PF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY���� ��� IP���� ������ �͵� ��û �����Ѵٴ� ��
	servAdr.sin_port = htons(atoi(argv[1]));

	if (bind(hServSock, (SOCKADDR*) &servAdr, sizeof(servAdr)) == SOCKET_ERROR) {
		cout << "bind() error!" << endl;
		exit(1);
	}

	if (listen(hServSock, 5) == SOCKET_ERROR) {
		// listen�� �ι�° ���ڴ� ���� ��� ť�� ũ��
		// accept�۾� �� ���������� ��� �� ����
		cout << "listen() error!" << endl;
		exit(1);
	}

	// �Ӱ迵�� Object ����
	InitializeCriticalSection(&cs);

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

		// Completion Port �� accept�� ���� ����
		CreateIoCompletionPort((HANDLE) hClientSock, hComPort,
				(DWORD) hClientSock, 0);

		Recv(hClientSock, recvBytes, flags);

		// �ʱ� ���� �޼��� Send
		char msg[BUF_SIZE];
		strncpy(msg, "������ ȯ���մϴ�!\n", BUF_SIZE);
		strcat(msg, loginBeforeMessage);
		SendToOneMsg(msg, hClientSock, STATUS_LOGOUT);

		delete handleInfo;
	}

	// �Ӱ迵�� Object ��ȯ
	DeleteCriticalSection(&cs);
	return 0;
}
