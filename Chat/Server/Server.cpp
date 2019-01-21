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

#include "BusinessService.h"
#include "common.h"
#include "CharPool.h"

using namespace std;

// ���� �����Ͻ� ���� ó�� Ŭ����
Service::BusinessService *businessService;

// Server ��ǻ��  CPU ������ŭ ������ �����ɰ�
// ���� ������ �� �Լ����� ó��
unsigned WINAPI HandleThread(LPVOID pCompPort) {

	// Completion port��ü
	HANDLE hComPort = (HANDLE) pCompPort;
	SOCKET sock;
	DWORD bytesTrans;
	LPPER_IO_DATA ioInfo;

	while (true) {

		bool success = GetQueuedCompletionStatus(hComPort, &bytesTrans,
				(LPDWORD) &sock, (LPOVERLAPPED*) &ioInfo, INFINITE);

		if (bytesTrans == 0 && !success) { // ���� ���� �ܼ� ���� ����
			// �ܼ� �������� ó��
			businessService->ClientExit(sock);

			MPool* mp = MPool::getInstance();
			mp->free(ioInfo);
		} else if (READ == ioInfo->serverMode
				|| READ_MORE == ioInfo->serverMode) { // Recv �������

				// ������ �б� ����
			businessService->PacketReading(ioInfo, bytesTrans);
			if ((ioInfo->recvByte < ioInfo->totByte)
					|| (ioInfo->recvByte < 4 && ioInfo->totByte == 0)) { // ���� ��Ŷ ���� || ��� �� ������ -> ���޾ƾ���

				businessService->BusinessService::getIocpService()->RecvMore(
						sock, ioInfo); // ��Ŷ ���ޱ� & �⺻ ioInfo ����
			} else { // �� ���� �� ���� ����
				int clientStatus = -1;
				int direction = -1;
				char *msg = businessService->DataCopy(ioInfo, &clientStatus,
						&direction);
				// DataCopy���� ���� ioInfo ��� free

				if (businessService->getUserMap().find(sock)
						== businessService->getUserMap().end()) { // ���ǰ� ���� => �α��� ���� �б�
						// �α��� ���� ���� ó��
					businessService->StatusLogout(sock, clientStatus, direction,
							msg);
					// Recv�� ����Ѵ�

					businessService->BusinessService::getIocpService()->Recv(
							sock); // ��Ŷ ���ޱ�
					// ���ǰ� ���� => �α��� ���� �б� ��
				} else { // ���ǰ� ������ => ���� �Ǵ� ä�ù� ����

					int status =
							businessService->getUserMap().find(sock)->second.status;

					if (status == STATUS_WAITING) { // ���� ���̽�
						// ���� ó�� �Լ�
						businessService->StatusWait(sock, status, direction,
								msg);
					} else if (status == STATUS_CHATTIG) { // ä�� �� ���̽�
						// ä�ù� ó�� �Լ�
						businessService->StatusChat(sock, status, direction,
								msg);
					}

					// Recv�� ����Ѵ�
					businessService->BusinessService::getIocpService()->Recv(
							sock); // ��Ŷ ���ޱ�
				}

			}
		} else if (WRITE == ioInfo->serverMode) { // Send �������
			// cout << "message send" << endl;
			CharPool* charPool = CharPool::getInstance();
			charPool->free(ioInfo->wsaBuf.buf);

			MPool* mp = MPool::getInstance();
			mp->free(ioInfo);
		} else {
			MPool* mp = MPool::getInstance();
			mp->free(ioInfo);
		}
	}
	return 0;
}

int main(int argc, char* argv[]) {

	WSADATA wsaData;
	HANDLE hComPort;
	SYSTEM_INFO sysInfo;

	SOCKET hServSock;
	SOCKADDR_IN servAdr;

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

	string port;
	cout << "��Ʈ��ȣ�� �Է��� �ּ���" << endl;
	cin >> port;

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = PF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY���� ��� IP���� ������ �͵� ��û �����Ѵٴ� ��
	servAdr.sin_port = htons(atoi(port.c_str()));

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

	businessService = new Service::BusinessService();

	cout << "Server ready listen" << endl;
	cout << "port number : " << port << endl;

	while (true) {
		SOCKET hClientSock;
		SOCKADDR_IN clntAdr;
		int addrLen = sizeof(clntAdr);
		cout << "accept wait" << endl;
		hClientSock = accept(hServSock, (SOCKADDR*) &clntAdr, &addrLen);

		// cout << "Connected client IP " << inet_ntoa(clntAdr.sin_addr) << endl;

		// Completion Port �� accept�� ���� ����
		CreateIoCompletionPort((HANDLE) hClientSock, hComPort,
				(DWORD) hClientSock, 0);

		businessService->BusinessService::getIocpService()->Recv(hClientSock);

		// �ʱ� ���� �޼��� Send
		string str = "������ ȯ���մϴ�!\n";
		str += loginBeforeMessage;

		businessService->BusinessService::getIocpService()->SendToOneMsg(
				str.c_str(), hClientSock, STATUS_LOGOUT);
	}

	delete businessService;

	return 0;
}
