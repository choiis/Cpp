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

using namespace std;

// ���� �����Ͻ� ���� ó�� Ŭ����
Service::BusinessService *handle;

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
			handle->ClientExit(sock);

			// delete ioInfo; // ����info ����
			handle->free(ioInfo);
		} else if (READ == ioInfo->serverMode
				|| READ_MORE == ioInfo->serverMode) { // Recv �������

				// ������ �б� ����
			handle->PacketReading(ioInfo, bytesTrans);
			if ((ioInfo->recvByte < ioInfo->totByte)
					|| (ioInfo->recvByte < 4 && ioInfo->totByte == 0)) { // ���� ��Ŷ ���� || ��� �� ������ -> ���޾ƾ���

				LPPER_IO_DATA ioInfo = handle->malloc();
				handle->BusinessService::getIocpService()->RecvMore(sock,
						ioInfo); // ��Ŷ ���ޱ�
			} else { // �� ���� �� ���� ����
				int clientStatus;
				int direction;
				char *msg = handle->DataCopy(ioInfo, &clientStatus, &direction);

				if (handle->getUserMap().find(sock)
						== handle->getUserMap().end()) { // ���ǰ� ���� => �α��� ���� �б�
						// �α��� ���� ���� ó��
					handle->StatusLogout(sock, clientStatus, direction, msg);
					// Recv�� ����Ѵ�
					LPPER_IO_DATA ioInfo = handle->malloc();
					handle->BusinessService::getIocpService()->Recv(sock,
							ioInfo); // ��Ŷ ���ޱ�
					// ���ǰ� ���� => �α��� ���� �б� ��
				} else { // ���ǰ� ������ => ���� �Ǵ� ä�ù� ����

					int status = handle->getUserMap().find(sock)->second->status;

					if (status == STATUS_WAITING) { // ���� ���̽�
						// ���� ó�� �Լ�
						handle->StatusWait(sock, status, direction, msg);
					} else if (status == STATUS_CHATTIG) { // ä�� �� ���̽�
						// ä�ù� ó�� �Լ�
						handle->StatusChat(sock, status, direction, msg);
					}

					// Recv�� ����Ѵ�
					LPPER_IO_DATA ioInfo = handle->malloc();
					handle->BusinessService::getIocpService()->Recv(sock,
							ioInfo); // ��Ŷ ���ޱ�
				}

			}
		} else if (WRITE == ioInfo->serverMode) { // Send �������
			cout << "message send :" << ioInfo->recvByte << endl;
			ioInfo->recvByte++; // �����Ϸ� ī��Ʈ +1
			if (ioInfo->totByte == ioInfo->recvByte) { // ���ο��� �� ������ ����
				delete ioInfo->wsaBuf.buf; // packet��ġ�� ����
				// delete ioInfo; // ����info ����
				handle->free(ioInfo);
			}
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

	handle = new Service::BusinessService();

	cout << "Server ready listen" << endl;
	cout << "port number : " << argv[1] << endl;

	while (true) {
		SOCKET hClientSock;
		SOCKADDR_IN clntAdr;
		int addrLen = sizeof(clntAdr);
		cout << "accept wait" << endl;
		hClientSock = accept(hServSock, (SOCKADDR*) &clntAdr, &addrLen);

		cout << "Connected client IP " << inet_ntoa(clntAdr.sin_addr) << endl;

		// Completion Port �� accept�� ���� ����
		CreateIoCompletionPort((HANDLE) hClientSock, hComPort,
				(DWORD) hClientSock, 0);
		LPPER_IO_DATA ioInfo1 = handle->malloc();
		handle->BusinessService::getIocpService()->Recv(hClientSock, ioInfo1);

		// �ʱ� ���� �޼��� Send
		string str = "������ ȯ���մϴ�!\n";
		str += loginBeforeMessage;
		LPPER_IO_DATA ioInfo2 = handle->malloc();
		handle->BusinessService::getIocpService()->SendToOneMsg(str.c_str(),
				hClientSock, STATUS_LOGOUT, ioInfo2);
	}

	delete handle;

	return 0;
}
