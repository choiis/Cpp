//============================================================================
// Name        : Server.cpp
// Author      :
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================
#include <iostream>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <process.h>
#include <winsock2.h>
#include <unordered_set>
#include "BusinessService.h"
#include "common.h"
#include "CharPool.h"

using namespace std;

// ���� �����Ͻ� ���� ó�� Ŭ����
Service::BusinessService *businessService;

// Recv�� Work���� ����
queue<JOB_DATA> jobQueue;
// ���Ӳ����� socket�� Send���� ����
unordered_set<SOCKET> liveSocket;

CRITICAL_SECTION liveSocketCs;

CRITICAL_SECTION queueCs;

// DB log insert�� ����ϴ� ������
unsigned WINAPI SQLThread(void *arg) {

	while (true) {
		businessService->SQLwork();
	}	
}

// Server ��ǻ��  CPU ������ŭ ������ �����ɰ�
// ���� ������ �� �Լ����� ó��
unsigned WINAPI WorkThread(void *arg) {

	while (true) {
		JOB_DATA jobData;
		jobData.job = 0;
		EnterCriticalSection(&queueCs);

		if (jobQueue.size() != 0) {
			jobData = jobQueue.front();
			jobData.job = 1;
			jobQueue.pop();
		}

		LeaveCriticalSection(&queueCs);

		if (jobData.job == 1) {

			EnterCriticalSection(&liveSocketCs);
			unordered_set<SOCKET>::const_iterator itr = liveSocket.find(jobData.socket);
			LeaveCriticalSection(&liveSocketCs);
			if (itr == liveSocket.end()) {
				continue;
			}
			// DataCopy���� ���� ioInfo ��� free
			if (!businessService->SessionCheck(jobData.socket)) { // ���ǰ� ���� => �α��� ���� �б�
				// �α��� ���� ���� ó��
				businessService->StatusLogout(jobData.socket, jobData.direction, jobData.msg.c_str());
				// ���ǰ� ���� => �α��� ���� �б� ��
			}
			else { // ���ǰ� ������ => ���� �Ǵ� ä�ù� ����

				int status = businessService->BusinessService::GetStatus(
					jobData.socket);

				if (status == STATUS_WAITING) { // ���� ���̽�
					// ���� ó�� �Լ�
					businessService->StatusWait(jobData.socket, status, jobData.direction,
						jobData.msg.c_str());
				}
				else if (status == STATUS_CHATTIG) { // ä�� �� ���̽�
					// ä�ù� ó�� �Լ�

					businessService->StatusChat(jobData.socket, status, jobData.direction,
						jobData.msg.c_str());
				}
			}
		}
	}

	return 0;
}

// Server ��ǻ��  CPU ������ŭ ������ �����ɰ�
// ���� ������ �� �Լ����� ó��
unsigned WINAPI RecvThread(LPVOID pCompPort) {

	// Completion port��ü
	HANDLE hComPort = (HANDLE) pCompPort;
	SOCKET sock;
	short bytesTrans;
	LPPER_IO_DATA ioInfo;

	while (true) {

		bool success = GetQueuedCompletionStatus(hComPort, (LPDWORD)&bytesTrans,
				(LPDWORD) &sock, (LPOVERLAPPED*) &ioInfo, INFINITE);

		if (bytesTrans == 0 && !success) { // ���� ���� �ܼ� ���� ����
			// �ܼ� �������� ó��
			EnterCriticalSection(&liveSocketCs);
			liveSocket.erase(sock);
			LeaveCriticalSection(&liveSocketCs);
			businessService->ClientExit(sock);
			MPool* mp = MPool::getInstance();
			mp->Free(ioInfo);
		} else if (READ == ioInfo->serverMode
			|| READ_MORE == ioInfo->serverMode) { // Recv �� �⺻ ����
			// ������ �б� ����
			short remainByte = min(bytesTrans, BUF_SIZE); // �ʱ� Remain Byte
			bool recvMore = false;

			while (1) {
				remainByte = businessService->PacketReading(ioInfo, remainByte);
				// �� ���� �� ���� ����
				// DataCopy������ ��� �޸� ���� ��ȯ
				if (remainByte >= 0) {
					JOB_DATA jobData;
					jobData.msg = businessService->DataCopy(ioInfo, &jobData.nowStatus,
						&jobData.direction);
					jobData.socket = sock;
					EnterCriticalSection(&queueCs);
					jobQueue.push(jobData);
					LeaveCriticalSection(&queueCs);
					// JobQueue Insert
				}
				
				if (remainByte == 0) {
					MPool* mp = MPool::getInstance();
					mp->Free(ioInfo);
					break;
				}
				else if (remainByte < 0) { // ���� ��Ŷ ���� || ��� �� ������ -> ���޾ƾ���
					businessService->BusinessService::getIocpService()->RecvMore(
						sock, ioInfo); // ��Ŷ ���ޱ� & �⺻ ioInfo ����
					recvMore = true;
					break;
				}
			}
			if (!recvMore) {
				businessService->BusinessService::getIocpService()->Recv(
					sock); // ��Ŷ ���ޱ�
			}
			
		} else if (WRITE == ioInfo->serverMode) { // Send �������

			CharPool* charPool = CharPool::getInstance();

			charPool->Free(ioInfo->wsaBuf.buf);
			MPool* mp = MPool::getInstance();

			mp->Free(ioInfo);
		} else {

			MPool* mp = MPool::getInstance();
			mp->Free(ioInfo);
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

	InitializeCriticalSectionAndSpinCount(&queueCs, 2000);

	InitializeCriticalSectionAndSpinCount(&liveSocketCs, 2000);
	
	businessService = new Service::BusinessService();
	
	// Thread Pool Client���� ��Ŷ �޴� ����
	for (int i = 0; i < (process * 3) / 8; i++) {
		// ������� HandleThread�� hComPort CP ������Ʈ�� �Ҵ��Ѵ�
		_beginthreadex(NULL, 0, RecvThread, (LPVOID)hComPort, 0, NULL);
	 }

	// Thread Pool Client�鿡�� ������ ����
	for (int i = 0; i < (process * 3) / 2; i++) {
		_beginthreadex(NULL, 0, WorkThread, NULL, 0, NULL);
	}

	// Thread Pool �α� ���� SQL ���࿡ ����
	for (int i = 0; i < process / 2; i++) {
		_beginthreadex(NULL, 0, SQLThread, NULL, 0, NULL);
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

	cout << "Server ready listen" << endl;
	cout << "port number : " << port << endl;

	while (true) {
		SOCKET hClientSock;
		SOCKADDR_IN clntAdr;
		int addrLen = sizeof(clntAdr);
		hClientSock = accept(hServSock, (SOCKADDR*) &clntAdr, &addrLen);

		EnterCriticalSection(&liveSocketCs);
		liveSocket.insert(hClientSock);
		LeaveCriticalSection(&liveSocketCs);
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
	DeleteCriticalSection(&queueCs);

	DeleteCriticalSection(&liveSocketCs);
	
	delete businessService;

	return 0;
}
