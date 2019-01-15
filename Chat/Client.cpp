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
#include "common.h"

// ���� Ŭ���̾�Ʈ ���� => ���� => ���� �α��� �������� �ٲ��
int clientStatus;

using namespace std;

typedef struct { // socket info
	SOCKET hClntSock;
	SOCKADDR_IN clntAdr;
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

// Recv ��� �����Լ�
void RecvMore(SOCKET sock, DWORD recvBytes, DWORD flags, LPPER_IO_DATA ioInfo) {

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
void Recv(SOCKET sock, DWORD recvBytes, DWORD flags) {
	LPPER_IO_DATA ioInfo = new PER_IO_DATA;
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

// fgets�� \n���� �����Լ� ���� �����÷ο����
void Gets(char *message, int size) {
	fgets(message, size, stdin);
	char *p;
	if ((p = strchr(message, '\n')) != NULL) {
		*p = '\0';
	}
}

// �۽��� ����� ������
unsigned WINAPI SendMsgThread(void *arg) {
	// �Ѿ�� clientSocket�� �޾���
	SOCKET clientSock = *((SOCKET*) arg);
	char msg[BUF_SIZE];

	while (1) {
		Gets(msg, BUF_SIZE);

		PER_IO_DATA dataBuf;
		WSAOVERLAPPED overlapped;

		memset(&overlapped, 0, sizeof(OVERLAPPED));

		P_PACKET_DATA packet;
		packet = new PACKET_DATA;
		if (clientStatus == STATUS_LOGOUT) { // �α��� ����
			if (strcmp(msg, "1") == 0) {	// ���� ���� ��

				while (true) {
					cout << "������ �Է��� �ּ���" << endl;
					Gets(msg, BUF_SIZE);

					if (strcmp(msg, "") != 0) {
						break;
					}
				}

				char password1[NAME_SIZE];
				char password2[NAME_SIZE];

				char nickname[NAME_SIZE];
				while (true) {
					cout << "��й�ȣ�� �Է��� �ּ���" << endl;
					Gets(password1, NAME_SIZE);
					if (strcmp(password1, "") == 0) {
						continue;
					} else {
						break;
					}
				}

				while (true) {
					cout << "��й�ȣ�� �ѹ��� �Է��� �ּ���" << endl;
					Gets(password2, NAME_SIZE);

					if (strcmp(password2, "") == 0) {
						continue;
					}

					if (strcmp(password1, password2) != 0) { // ��й�ȣ �ٸ�
						cout << "��й�ȣ Ȯ�� ����!" << endl;
					} else {
						break;
					}
				}

				while (true) {
					cout << "�г����� �Է��� �ּ���" << endl;
					Gets(nickname, NAME_SIZE);

					if (strcmp(nickname, "") != 0) {
						break;
					}
				}
				strcat(msg, "\\");
				strcat(msg, password1);  // ��й�ȣ
				strcat(msg, "\\");
				strcat(msg, nickname); // �г���
				packet->direction = USER_MAKE;
				packet->clientStatus = STATUS_LOGOUT;

			} else if (strcmp(msg, "2") == 0) { // �α��� �õ�
				cout << "������ �Է��� �ּ���" << endl;
				Gets(msg, BUF_SIZE);

				char password[NAME_SIZE];

				cout << "��й�ȣ�� �Է��� �ּ���" << endl;
				Gets(password, NAME_SIZE);

				strcat(msg, "\\");
				strcat(msg, password);  // ��й�ȣ

				packet->direction = USER_ENTER;
				packet->clientStatus = STATUS_LOGOUT;
			} else if (strcmp(msg, "3") == 0) {
				packet->direction = USER_LIST;
				packet->clientStatus = STATUS_LOGOUT;
			} else if (strcmp(msg, "4") == 0) { // Ŭ���̾�Ʈ ��������
				exit(1);
			} else if (strcmp(msg, "5") == 0) { // �ܼ������
				system("cls");
				cout << loginBeforeMessage << endl;
				continue;
			} else if (strcmp(msg, "") == 0) { // �Է¾��ϸ� Send����
				continue;
			}

		} else if (clientStatus == STATUS_WAITING) { // ���� �� ��
			if (strcmp(msg, "2") == 0) {	// �� ���� ��
				cout << "�� �̸��� �Է��� �ּ���" << endl;
				Gets(msg, BUF_SIZE);

				packet->direction = ROOM_MAKE;
			} else if (strcmp(msg, "3") == 0) {	// �� ������ ��
				cout << "������ �� �̸��� �Է��� �ּ���" << endl;
				Gets(msg, BUF_SIZE);
				packet->direction = ROOM_ENTER;
			} else if (strcmp(msg, "5") == 0) { // �ӼӸ�

				char toName[NAME_SIZE];
				char Msg[BUF_SIZE];
				cout << "�ӼӸ� ����� �Է��� �ּ���" << endl;
				Gets(toName, NAME_SIZE);
				strncpy(msg, toName, NAME_SIZE);
				cout << "������ ���� �Է��� �ּ���" << endl;

				Gets(Msg, BUF_SIZE);
				strcat(msg, "\\");
				strcat(msg, Msg);  // ���
				packet->direction = WHISPER;
			} else if (strcmp(msg, "7") == 0) { // �ܼ������
				system("cls");
				cout << waitRoomMessage << endl;
				continue;
			} else if (strcmp(msg, "") == 0) { // �Է¾��ϸ� Send����
				continue;
			}
		} else if (clientStatus == STATUS_CHATTIG) { // ä������ ��
			if (strcmp(msg, "") == 0) { // �Է¾��ϸ� Send����
				continue;
			} else if (strcmp(msg, "\\clear") == 0) { // �ܼ�â clear
				system("cls");
				cout << chatRoomMessage << endl;
				continue;
			}
		}

		if (strcmp(msg, "") == 0) { // �Է¾��ϸ� Send����
			continue;
		}
		int len = strlen(msg) + 1;
		char *p;
		p = new char[len + (3 * sizeof(int))];
		memcpy(p, &len, 4); // dataSize;
		memcpy(((char*) p) + 4, &clientStatus, 4); // status;
		memcpy(((char*) p) + 8, &packet->direction, 4); // direction;

		packet->message = new char[len];

		strncpy(packet->message, msg, len - 1);
		packet->message[strlen(msg)] = '\0';
		memcpy(((char*) p) + 12, msg, len); // status;

		dataBuf.wsaBuf.buf = (char*) p;
		dataBuf.wsaBuf.len = len + (3 * sizeof(int));
		dataBuf.serverMode = WRITE;

		WSASend(clientSock, &(dataBuf.wsaBuf), 1, NULL, 0, &overlapped, NULL);
	}
	return 0;
}

// ������ ����� ������
unsigned WINAPI RecvMsgThread(LPVOID hComPort) {

	SOCKET sock;
	DWORD bytesTrans;
	LPPER_IO_DATA ioInfo;

	while (1) {
		GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD) &sock,
				(LPOVERLAPPED*) &ioInfo, INFINITE);

		// IO �Ϸ��� ���� �κ�
		if (READ == ioInfo->serverMode) {
			memcpy(&(ioInfo->bodySize), ioInfo->buffer, 4);
			ioInfo->recvBuffer = new char[ioInfo->bodySize + 12]; // BodySize��ŭ ���� �Ҵ�
			memcpy(((char*) ioInfo->recvBuffer), ioInfo->buffer, bytesTrans);
			ioInfo->recvByte = 0;
			ioInfo->totByte = ioInfo->bodySize + 12;
		} else if (READ_MORE == ioInfo->serverMode) {

			if (ioInfo->recvByte > 12 && ioInfo->recvBuffer != NULL) {
				memcpy(((char*) ioInfo->recvBuffer) + ioInfo->recvByte,
						ioInfo->buffer, bytesTrans);
			}
		}
		if (READ_MORE == ioInfo->serverMode || READ == ioInfo->serverMode) {
			ioInfo->recvByte += bytesTrans;
			if (ioInfo->recvByte < ioInfo->totByte) { // ���� ��Ŷ ���� -> ���޾ƾ���
				DWORD recvBytes = 0;
				DWORD flags = 0;
				RecvMore(sock, recvBytes, flags, ioInfo); // ��Ŷ ���ޱ�
			} else {
				memcpy(&(ioInfo->serverMode), ((char*) ioInfo->recvBuffer) + 4,
						4);
				char *msg;
				msg = new char[ioInfo->bodySize];

				memcpy(msg, ((char*) ioInfo->recvBuffer) + 12,
						ioInfo->bodySize);
				// Client�� ���� ���� ���� �ʼ�
				// �������� �ذ����� ����
				if (ioInfo->serverMode == STATUS_LOGOUT
						|| ioInfo->serverMode == STATUS_WAITING
						|| ioInfo->serverMode == STATUS_CHATTIG) {
					if (clientStatus != ioInfo->serverMode) { // ���� ����� �ܼ� clear
						system("cls");
					}
					clientStatus = ioInfo->serverMode; // clear���� client���º��� ���ش�
					cout << msg << endl;
				} else if (ioInfo->serverMode == STATUS_WHISPER) { // �ӼӸ� �����϶��� Ŭ���̾�Ʈ ���º�ȭ ����
					cout << msg << endl;
				}

				DWORD recvBytes = 0;
				DWORD flags = 0;
				Recv(sock, recvBytes, flags);
			}
		}

	}
	return 0;
}
int main(int argc, char* argv[0]) {

	WSADATA wsaData;
	SOCKET hSocket;
	SOCKADDR_IN servAddr;

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
	// Completion Port ����
	HANDLE hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	// Completion Port �� ���� ����
	CreateIoCompletionPort((HANDLE) hSocket, hComPort, (DWORD) hSocket, 0);

	// ���� ������ ����
	// ������� RecvMsg�� hComPort CP ������Ʈ�� �Ҵ��Ѵ�
	// RecvMsg���� Recv�� �Ϸ�Ǹ� ������ �κ��� �ִ�
	recvThread = (HANDLE) _beginthreadex(NULL, 0, RecvMsgThread,
			(LPVOID) hComPort, 0,
			NULL);

	// �۽� ������ ����
	// Thread�ȿ��� clientSocket���� Send���ٰŴϱ� ���ڷ� �Ѱ��ش�
	// CP�� Send�� ���� �ȵǾ����� GetQueuedCompletionStatus���� Send �Ϸ�ó�� �ʿ����
	sendThread = (HANDLE) _beginthreadex(NULL, 0, SendMsgThread,
			(void*) &hSocket, 0,
			NULL);

	Recv(hSocket, 0, 0);
	WaitForSingleObject(sendThread, INFINITE);
	WaitForSingleObject(recvThread, INFINITE);
	closesocket(hSocket);
	WSACleanup();
	return 0;
}
