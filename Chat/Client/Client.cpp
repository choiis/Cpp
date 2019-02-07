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
#include <windows.h>
#include <process.h>
#include <regex>
#include "common.h"
#include "MPool.h"
#include "CharPool.h"

// ���� Ŭ���̾�Ʈ ���� => ���� => ���� �α��� �������� �ٲ��
int clientStatus;

using namespace std;

typedef struct { // socket info
	SOCKET hClntSock;
	SOCKADDR_IN clntAdr;
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

// ����+���� ���ڿ� �Ǻ�
bool IsAlphaNumber(string str) {
	regex alpha("[a-z|A-Z|0-9]+");
	if (regex_match(str, alpha)) {
		return true;
	}
	else {
		return false;
	}
}

// Recv ��� �����Լ�
void RecvMore(SOCKET sock, LPPER_IO_DATA ioInfo) {
	DWORD recvBytes = 0;
	DWORD flags = 0;
	memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
	ioInfo->wsaBuf.len = BUF_SIZE;
	memset(ioInfo->buffer, 0, BUF_SIZE);
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
	ioInfo->wsaBuf.len = BUF_SIZE;
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
void SendMsg(SOCKET clientSock, const char* msg, int status, int direction) {
	MPool* mp = MPool::getInstance();
	LPPER_IO_DATA ioInfo = mp->Malloc();

	memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));

	unsigned short len = min((unsigned short)strlen(msg) + 11, BUF_SIZE); // �ִ� ������ �ִ� ���� 502Byte
	CharPool* charPool = CharPool::getInstance();
	char* packet = charPool->Malloc(); // 512 Byte���� �б� ����

	copy((char*)&len, (char*)&len + 2, packet); // dataSize
	copy((char*)&status, (char*)&status + 4, packet + 2);  // status
	copy((char*)&direction, (char*)&direction + 4, packet + 6);  // direction
	copy(msg, msg + len, packet + 10);  // msg
	ioInfo->wsaBuf.buf = (char*)packet;
	ioInfo->wsaBuf.len = len;
	ioInfo->serverMode = WRITE;

	WSASend(clientSock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped),
		NULL);

}


// �۽��� ����� ������
unsigned WINAPI SendMsgThread(void *arg) {
	// �Ѿ�� clientSocket�� �޾���
	SOCKET clientSock = *((SOCKET*)arg);

	while (1) {
		string msg;
		getline(cin, msg);

		int direction = -1;
		int status = -1;

		if (clientStatus == STATUS_LOGOUT) { // �α��� ����
			if (msg.compare("1") == 0) {	// ���� ���� ��

				while (true) {
					cout << "������ �Է��� �ּ��� (��������10�ڸ�)" << endl;
					getline(cin, msg);

					if (msg.compare("") != 0 && msg.length() <= 10
						&& IsAlphaNumber(msg)) {
						break;
					}
				}

				string password1;
				string password2;

				string nickname;
				while (true) {
					cout << "��й�ȣ�� �Է��� �ּ��� (��������4�ڸ��̻� 10�ڸ�����)" << endl;
					getline(cin, password1);
					if (password1.compare("") == 0) {
						continue;
					}
					else if (password1.length() >= 4
						&& password1.length() <= 10
						&& IsAlphaNumber(password1)) {
						break;
					}
				}

				while (true) {
					cout << "��й�ȣ�� �ѹ��� �Է��� �ּ���" << endl;
					getline(cin, password2);

					if (password2.compare("") == 0) {
						continue;
					}

					if (password1.compare(password2) != 0) { // ��й�ȣ �ٸ�
						cout << "��й�ȣ Ȯ�� ����!" << endl;
					}
					else {
						break;
					}
				}

				while (true) {
					cout << "�г����� �Է��� �ּ��� (20����Ʈ ����)" << endl;
					getline(cin, nickname);

					if (nickname.compare("") != 0 && nickname.length() <= 20) {
						break;
					}
				}
				msg.append("\\");
				msg.append(password1); // ��й�ȣ
				msg.append("\\");
				msg.append(nickname); // �г���
				direction = USER_MAKE;
				status = STATUS_LOGOUT;
			}
			else if (msg.compare("2") == 0) { // �α��� �õ�
				cout << "������ �Է��� �ּ���" << endl;
				getline(cin, msg);

				string password;

				cout << "��й�ȣ�� �Է��� �ּ���" << endl;
				getline(cin, password);

				msg.append("\\");
				msg.append(password); // ��й�ȣ

				direction = USER_ENTER;
				status = STATUS_LOGOUT;
			}
			else if (msg.compare("3") == 0) { // Ŭ���̾�Ʈ ��������
				exit(1);
			}
			else if (msg.compare("4") == 0) { // �ܼ������
				system("cls");
				cout << loginBeforeMessage << endl;
				continue;
			}
		}
		else if (clientStatus == STATUS_WAITING) { // ���� �� ��
			if (msg.compare("1") == 0) {	// �� ���� ��û
				direction = ROOM_INFO;
			}
			else if (msg.compare("2") == 0) {	// �� ���� ��
				cout << "�� �̸��� �Է��� �ּ���" << endl;
				getline(cin, msg);

				direction = ROOM_MAKE;
			}
			else if (msg.compare("3") == 0) {	// �� ������ ��
				cout << "������ �� �̸��� �Է��� �ּ���" << endl;
				getline(cin, msg);
				direction = ROOM_ENTER;
			}
			else if (msg.compare("4") == 0) { // ���� ���� ��û
				direction = ROOM_USER_INFO;
			}
			else if (msg.compare("5") == 0) { // �ӼӸ�

				string Msg;
				cout << "�ӼӸ� ����� �Է��� �ּ���" << endl;
				getline(cin, msg);
				cout << "������ ���� �Է��� �ּ���" << endl;
				getline(cin, Msg);
				msg.append("\\");
				msg.append(Msg); // ���
				direction = WHISPER;
			}
			else if (msg.compare("6") == 0) { // �α׾ƿ�
				direction = LOG_OUT;
			}
			else if (msg.compare("7") == 0) { // �ܼ������
				system("cls");
				cout << waitRoomMessage << endl;
				continue;
			}
		}
		else if (clientStatus == STATUS_CHATTIG) { // ä������ ��
			if (msg.compare("\\clear") == 0) { // �ܼ�â clear
				system("cls");
				cout << chatRoomMessage << endl;
				continue;
			}
		}

		if (msg.compare("") == 0) { // �Է¾��ϸ� Send����
			continue;
		}

		SendMsg(clientSock, msg.c_str(), status, direction);
	}
	return 0;
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

// ������ ����� ������
unsigned WINAPI RecvMsgThread(LPVOID hComPort) {

	SOCKET sock;
	short bytesTrans;
	LPPER_IO_DATA ioInfo;

	while (1) {
		bool success = GetQueuedCompletionStatus(hComPort, (LPDWORD)&bytesTrans,
			(LPDWORD)&sock, (LPOVERLAPPED*)&ioInfo, INFINITE);

		if (bytesTrans == 0 && !success) { // ���� ���� �ܼ� ���� ����
			// ���� �ܼ� �������� ó��
			cout << "���� ����" << endl;
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
						if (clientStatus != status) { // ���� ����� �ܼ� clear
							system("cls");
							switch (status) {
							case STATUS_LOGOUT:
								SetConsoleTextAttribute(
									GetStdHandle(STD_OUTPUT_HANDLE), 10);
								break;
							case STATUS_WAITING:
								SetConsoleTextAttribute(
									GetStdHandle(STD_OUTPUT_HANDLE), 9);
								break;
							case STATUS_CHATTIG:
								SetConsoleTextAttribute(
									GetStdHandle(STD_OUTPUT_HANDLE), 14);
								break;
							}

						}

						clientStatus = status; // clear���� client���º��� ���ش�
						cout << msg << endl;
					}
					else if (status == STATUS_WHISPER) { // �ӼӸ� �����϶��� Ŭ���̾�Ʈ ���º�ȭ ����
						cout << msg << endl;
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
		else {
			MPool* mp = MPool::getInstance();
			mp->Free(ioInfo);
		}
	}
	return 0;
}
int main() {

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

	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
	while (1) {

		cout << "��Ʈ��ȣ�� �Է��� �ּ��� :";
		string port;
		cin >> port;

		servAddr.sin_port = htons(atoi(port.c_str()));

		if (connect(hSocket, (SOCKADDR*)&servAddr,
			sizeof(servAddr)) == SOCKET_ERROR) {
			printf("connect() error!");
		}
		else {
			break;
		}
	}
	// Completion Port ����
	HANDLE hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	// Completion Port �� ���� ����
	CreateIoCompletionPort((HANDLE)hSocket, hComPort, (DWORD)hSocket, 0);

	// ���� ������ ����
	// ������� RecvMsg�� hComPort CP ������Ʈ�� �Ҵ��Ѵ�
	// RecvMsg���� Recv�� �Ϸ�Ǹ� ������ �κ��� �ִ�
	recvThread = (HANDLE)_beginthreadex(NULL, 0, RecvMsgThread,
		(LPVOID)hComPort, 0,
		NULL);

	// �۽� ������ ����
	// Thread�ȿ��� clientSocket���� Send���ٰŴϱ� ���ڷ� �Ѱ��ش�
	// CP�� Send�� ���� �ȵǾ����� GetQueuedCompletionStatus���� Send �Ϸ�ó�� �ʿ����
	sendThread = (HANDLE)_beginthreadex(NULL, 0, SendMsgThread,
		(void*)&hSocket, 0,
		NULL);

	Recv(hSocket);
	WaitForSingleObject(sendThread, INFINITE);
	WaitForSingleObject(recvThread, INFINITE);
	closesocket(hSocket);
	WSACleanup();
	return 0;
}