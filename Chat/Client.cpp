//============================================================================
// Name        : Client.cpp
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

#define BUF_SIZE 512
#define NAME_SIZE 20

// Ŭ���̾�Ʈ ���� ����
#define STATUS_LOGOUT 1
#define STATUS_WAITING 2
#define STATUS_CHATTIG 3

// �������� ���� ����
#define ROOM_MAKE 1
#define ROOM_ENTER 2
#define ROOM_OUT 3

// ���� Ŭ���̾�Ʈ ���� => ���� => ���� �α��� �������� �ٲ��
int clientStatus = 2;

char name[NAME_SIZE] = "";
char msg[BUF_SIZE];

using namespace std;

typedef struct { // socket info
	SOCKET hClntSock;
	SOCKADDR_IN clntAdr;
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
	int clientMode;
} PER_IO_DATA, *LPPER_IO_DATA;

// �۽��� ����� ������
unsigned WINAPI SendMsg(void *arg) {
	SOCKET hSock = *((SOCKET*) arg);
	char msg[BUF_SIZE];

	while (1) {
		gets(msg);
		if (strcmp(msg, "\n") == 0) { // �Է� ���ϸ� ���� ����
			continue;
		} else if (clientStatus == STATUS_WAITING // ����
				&& (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n")
						|| !strcmp(msg, "out\n"))) {
			// ä�� ���� �޽���
			send(hSock, "out", sizeof("out"), 0);
			closesocket(hSock);
			exit(1);
		}

		PER_IO_DATA dataBuf;
		WSAOVERLAPPED overlapped;

		WSAEVENT event = WSACreateEvent();
		memset(&overlapped, 0, sizeof(OVERLAPPED));
		overlapped.hEvent = event;

		P_PACKET_DATA packet;
		packet = new PACKET_DATA;
		if (clientStatus == STATUS_WAITING) { // ���� �� ��
			if (strcmp(msg, "2") == 0) {
				puts("�� �̸��� �Է��� �ּ���");
				gets(msg);
				packet->direction = ROOM_MAKE;
			} else if (strcmp(msg, "3") == 0) {
				puts("������ �� �̸��� �Է��� �ּ���");
				gets(msg);
				packet->direction = ROOM_ENTER;
			}
		} else if (clientStatus == STATUS_CHATTIG) { // ä������ ��

		}
		strcpy(packet->message, msg);

		dataBuf.wsaBuf.buf = (char*) packet;
		dataBuf.wsaBuf.len = sizeof(PACKET_DATA);

		WSASend(hSock, &(dataBuf.wsaBuf), 1, NULL, 0, &overlapped, NULL);
	}
	return 0;
}

// ������ ����� ������
unsigned WINAPI RecvMsg(LPVOID hComPort) {

	SOCKET sock;
	char nameMsg[BUF_SIZE];
	DWORD bytesTrans;
	LPPER_HANDLE_DATA handleInfo;
	LPPER_IO_DATA ioInfo;

	while (1) {
		GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD) &handleInfo,
				(LPOVERLAPPED*) &ioInfo, INFINITE);

		P_PACKET_DATA packet = new PACKET_DATA;
		memcpy(packet, ioInfo->buffer, sizeof(PACKET_DATA));

		// Client�� ���� ���� ���� �ʼ�
		// �������� �ذ����� ����
		if (packet->clientStatus == STATUS_WAITING) {
			clientStatus = packet->clientStatus;
			strcpy(nameMsg, packet->message);
			cout << nameMsg << endl;
		} else if (packet->clientStatus == STATUS_CHATTIG) {
			clientStatus = packet->clientStatus;
			strcpy(nameMsg, packet->message);
			cout << nameMsg << endl;
		}

		sock = handleInfo->hClntSock;
		if (bytesTrans == 0) {
			return -1;
		}

		int recvBytes, flags = 0;
		ioInfo = new PER_IO_DATA;
		ioInfo->wsaBuf.len = sizeof(PACKET_DATA);
		ioInfo->wsaBuf.buf = ioInfo->buffer;
		WSARecv(sock, &(ioInfo->wsaBuf), 1, (LPDWORD) &recvBytes,
				(LPDWORD) &flags, &(ioInfo->overlapped),
				NULL);

	}
	return 0;
}
int main(int argc, char* argv[0]) {

	WSADATA wsaData;
	SOCKET hSocket;
	SOCKADDR_IN servAddr;
	LPPER_HANDLE_DATA handleInfo;
	HANDLE sendThread, recvThread;

	// Socket lib�� �ʱ�ȭ
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("WSAStartup() error!");
		exit(1);
	}
	// Overlapped IO���� ������ �����
	// TCP ����Ұ�
	hSocket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0,
	WSA_FLAG_OVERLAPPED);

	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = PF_INET;
	servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	while (1) {

		cout << "��Ʈ��ȣ�� �Է��� �ּ��� :";
		string port;
		cin >> port;
		servAddr.sin_port = htons(atoi(port.c_str()));

		if (connect(hSocket, (SOCKADDR*) &servAddr,
				sizeof(servAddr))==SOCKET_ERROR) {
			printf("connect() error!");
		} else {
			break;
		}
	}

	cout << "�̸��� �Է��� �ּ��� :";
	string user;
	cin >> user;
	sprintf(name, "%s", user.c_str());

	// Completion Port ����
	HANDLE hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	PER_IO_DATA dataBuf;
	WSAOVERLAPPED overlapped;

	WSAEVENT event = WSACreateEvent();
	handleInfo = new PER_HANDLE_DATA;
	handleInfo->hClntSock = hSocket;
	int addrLen = sizeof(servAddr);
	memcpy(&(handleInfo->clntAdr), &servAddr, addrLen);
	memset(&overlapped, 0, sizeof(OVERLAPPED));

	// Completion Port �� ���� ����
	CreateIoCompletionPort((HANDLE) hSocket, hComPort, (DWORD) handleInfo, 0);

	overlapped.hEvent = event;
	P_PACKET_DATA packet;
	packet = new PACKET_DATA;
	strcpy(packet->message, name);

	dataBuf.wsaBuf.buf = (char*) packet;
	dataBuf.wsaBuf.len = sizeof(PACKET_DATA);

	// �̸����� ���� ����
	WSASend(hSocket, &(dataBuf.wsaBuf), 1, NULL, 0, &overlapped, NULL);

	delete packet;
	// ���� ������ ����
	recvThread = (HANDLE) _beginthreadex(NULL, 0, RecvMsg, (LPVOID) hComPort, 0,
	NULL);
	// �۽� ������ ����
	sendThread = (HANDLE) _beginthreadex(NULL, 0, SendMsg, (void*) &hSocket, 0,
	NULL);

	WaitForSingleObject(sendThread, INFINITE);
	WaitForSingleObject(recvThread, INFINITE);
	closesocket(hSocket);
	WSACleanup();
	return 0;
}
