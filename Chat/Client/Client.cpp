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
	} else {
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

	int len = min((int) strlen(msg) + 1, BUF_SIZE - 12); // �ִ� ������ �ִ� ���� 500Byte
	CharPool* charPool = CharPool::getInstance();
	char* packet = charPool->Malloc(); // 512 Byte���� �б� ����

	copy((char*) &len, (char*) &len + 4, packet); // dataSize
	copy((char*) &status, (char*) &status + 4, packet + 4);  // status
	copy((char*) &direction, (char*) &direction + 4, packet + 8);  // direction
	copy(msg, msg + len, packet + 12);  // msg

	ioInfo->wsaBuf.buf = (char*) packet;
	ioInfo->wsaBuf.len = len + (3 * sizeof(int));
	ioInfo->serverMode = WRITE;

	WSASend(clientSock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped),
	NULL);

}

// �۽��� ����� ������
unsigned WINAPI SendMsgThread(void *arg) {
	// �Ѿ�� clientSocket�� �޾���
	SOCKET clientSock = *((SOCKET*) arg);

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
					} else if (password1.length() >= 4
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
					} else {
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
			} else if (msg.compare("2") == 0) { // �α��� �õ�
				cout << "������ �Է��� �ּ���" << endl;
				getline(cin, msg);

				string password;

				cout << "��й�ȣ�� �Է��� �ּ���" << endl;
				getline(cin, password);

				msg.append("\\");
				msg.append(password); // ��й�ȣ

				direction = USER_ENTER;
				status = STATUS_LOGOUT;
			} else if (msg.compare("3") == 0) {
				direction = USER_LIST;
				status = STATUS_LOGOUT;
			} else if (msg.compare("4") == 0) { // Ŭ���̾�Ʈ ��������
				exit(1);
			} else if (msg.compare("5") == 0) { // �ܼ������
				system("cls");
				cout << loginBeforeMessage << endl;
				continue;
			}
		} else if (clientStatus == STATUS_WAITING) { // ���� �� ��
			if (msg.compare("1") == 0) {	// �� ���� ��û
				direction = ROOM_INFO;
			} else if (msg.compare("2") == 0) {	// �� ���� ��
				cout << "�� �̸��� �Է��� �ּ���" << endl;
				getline(cin, msg);

				direction = ROOM_MAKE;
			} else if (msg.compare("3") == 0) {	// �� ������ ��
				cout << "������ �� �̸��� �Է��� �ּ���" << endl;
				getline(cin, msg);
				direction = ROOM_ENTER;
			} else if (msg.compare("4") == 0) {	// ���� ���� ��û
				direction = ROOM_USER_INFO;
			} else if (msg.compare("5") == 0) { // �ӼӸ�

				string Msg;
				cout << "�ӼӸ� ����� �Է��� �ּ���" << endl;
				getline(cin, msg);
				cout << "������ ���� �Է��� �ּ���" << endl;
				getline(cin, Msg);
				msg.append("\\");
				msg.append(Msg); // ���
				direction = WHISPER;
			} else if (msg.compare("7") == 0) { // �ܼ������
				system("cls");
				cout << waitRoomMessage << endl;
				continue;
			}
		} else if (clientStatus == STATUS_CHATTIG) { // ä������ ��
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
void PacketReading(LPPER_IO_DATA ioInfo, DWORD bytesTrans) {
	// IO �Ϸ��� ���� �κ�
	if (READ == ioInfo->serverMode) {
		if (bytesTrans < 4) { // ����� �� �� �о�� ��Ȳ
			copy(((char*) &(ioInfo->bodySize)) + ioInfo->recvByte,
					((char*) &(ioInfo->bodySize)) + ioInfo->recvByte
							+ bytesTrans, ioInfo->buffer);
			ioInfo->recvByte += bytesTrans; // ���ݱ��� ���� ����Ʈ �� ����
		} else {
			copy(ioInfo->buffer, ioInfo->buffer + 4,
					(char*) &(ioInfo->bodySize));
			if (ioInfo->bodySize >= 0 && ioInfo->bodySize <= BUF_SIZE - 12) { // Size ����
				CharPool* charPool = CharPool::getInstance();
				ioInfo->recvBuffer = charPool->Malloc(); // 512 Byte���� �б� ����
				copy(ioInfo->buffer, ioInfo->buffer + bytesTrans,
						((char*) ioInfo->recvBuffer));
				ioInfo->recvByte += bytesTrans; // ���ݱ��� ���� ����Ʈ �� ����
			} else { // �߸��� bodySize; => ���б� �ʿ�
				ioInfo->recvByte = 0;
				ioInfo->totByte = 0;
				ioInfo->bodySize = 0;
			}
		}
	} else { // �� �б�
		if (ioInfo->recvByte >= 4) { // ��� �� �о���
			copy(ioInfo->buffer, ioInfo->buffer + bytesTrans,
					((char*) ioInfo->recvBuffer) + ioInfo->recvByte);
			ioInfo->recvByte += bytesTrans; // ���ݱ��� ���� ������ �� ����
		} else { // ����� �� ���о��� ���
			int recv = min(4 - ioInfo->recvByte, bytesTrans);
			copy(ioInfo->buffer, ioInfo->buffer + recv,
					((char*) &(ioInfo->bodySize)) + ioInfo->recvByte);
			ioInfo->bodySize = min(ioInfo->bodySize, (DWORD) BUF_SIZE - 12);
			ioInfo->recvByte += bytesTrans; // ���ݱ��� ���� ������ �� ����
			if (ioInfo->recvByte >= 4) {
				CharPool* charPool = CharPool::getInstance();
				ioInfo->recvBuffer = charPool->Malloc(); // 512 Byte���� �б� ����
				copy(((char*) ioInfo->buffer) + recv,
						((char*) ioInfo->buffer) + recv + bytesTrans - recv,
						((char*) ioInfo->recvBuffer) + 4);
			}
		}
	}

	if (ioInfo->totByte == 0 && ioInfo->recvByte >= 4) { // ����� �� �о�� ��Ż ����Ʈ ���� �� �� �ִ�
		ioInfo->totByte = min(ioInfo->bodySize + 12, (DWORD) BUF_SIZE);
	}
}

// Ŭ���̾�Ʈ���� ���� ������ ������ ����ü ����
char* DataCopy(LPPER_IO_DATA ioInfo, int *status) {
	copy(((char*) ioInfo->recvBuffer) + 4, ((char*) ioInfo->recvBuffer) + 8,
			(char*) status);
	CharPool* charPool = CharPool::getInstance();
	char* msg = charPool->Malloc(); // 512 Byte���� ī�� ����

	copy(((char*) ioInfo->recvBuffer) + 12,
			((char*) ioInfo->recvBuffer) + 12
					+ min(ioInfo->bodySize, (DWORD) BUF_SIZE), msg); //������ġ

	// �� ���� �޾����� �Ҵ� ����
	charPool->Free(ioInfo->recvBuffer);

	MPool* mp = MPool::getInstance();
	mp->Free(ioInfo);

	return msg;
}

// ������ ����� ������
unsigned WINAPI RecvMsgThread(LPVOID hComPort) {

	SOCKET sock;
	DWORD bytesTrans;
	LPPER_IO_DATA ioInfo;

	while (1) {
		bool success = GetQueuedCompletionStatus(hComPort, &bytesTrans,
				(LPDWORD) &sock, (LPOVERLAPPED*) &ioInfo, INFINITE);

		if (bytesTrans == 0 && !success) { // ���� ���� �ܼ� ���� ����
			// ���� �ܼ� �������� ó��
			cout << "���� ����" << endl;
			closesocket(sock);
			MPool* mp = MPool::getInstance();
			mp->Free(ioInfo);
		} else if (READ_MORE == ioInfo->serverMode
				|| READ == ioInfo->serverMode) {

			// ������ �б� ����
			PacketReading(ioInfo, bytesTrans);

			if (((ioInfo->recvByte < ioInfo->totByte)
					|| (ioInfo->recvByte < 4 && ioInfo->totByte == 0))
					&& ioInfo->recvByte <= BUF_SIZE) { // ���� ��Ŷ ���� || ��� �� ������ -> ���޾ƾ���

				RecvMore(sock, ioInfo); // ��Ŷ ���ޱ�
			} else {
				int status;
				char *msg = DataCopy(ioInfo, &status);

				// Client�� ���� ���� ���� �ʼ�
				// �������� �ذ����� ����
				if (status == STATUS_LOGOUT || status == STATUS_WAITING
						|| status == STATUS_CHATTIG) {
					if (clientStatus != status) { // ���� ����� �ܼ� clear
						system("cls");
					}

					clientStatus = status; // clear���� client���º��� ���ش�
					cout << msg << endl;
				} else if (status == STATUS_WHISPER) { // �ӼӸ� �����϶��� Ŭ���̾�Ʈ ���º�ȭ ����
					cout << msg << endl;
				}
				CharPool* charPool = CharPool::getInstance();
				charPool->Free(msg);
				Recv(sock);
			}

		} else if (WRITE == ioInfo->serverMode) {
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

	Recv(hSocket);
	WaitForSingleObject(sendThread, INFINITE);
	WaitForSingleObject(recvThread, INFINITE);
	closesocket(hSocket);
	WSACleanup();
	return 0;
}
