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
#include <direct.h>
#include "BusinessService.h"

using namespace std;

// ���� �����Ͻ� ���� ó�� Ŭ����
BusinessService::BusinessService *businessService;

// Recv�� Work���� ����
queue<JOB_DATA> jobQueue;

CRITICAL_SECTION queueCs;

// DB log insert�� ����ϴ� ������
unsigned WINAPI SQLThread(void *arg) {

	while (true) {
		businessService->SQLwork();
	}	

	return 0;
}

// Send broadcast ����ϴ� ������
unsigned WINAPI SendThread(void *arg) {

	while (true) {
		businessService->Sendwork();
	}
	return 0;
}


// Server ��ǻ��  CPU ������ŭ ������ �����ɰ�
// ���� ������ �� �Լ����� ó��
unsigned WINAPI WorkThread(void *arg) {

	while (true) {

		if (!jobQueue.empty()) {
			EnterCriticalSection(&queueCs);
			// ť ��ä ����
			queue<JOB_DATA> copyJobQueue = jobQueue;
			queue<JOB_DATA> emptyQueue; // �� ť
			swap(jobQueue, emptyQueue); // �� ť�� �ٲ�ġ��
			LeaveCriticalSection(&queueCs);

			while (!copyJobQueue.empty()) { // ���� ��Ŷ ������ �Ѳ����� ó��
				JOB_DATA jobData = copyJobQueue.front();
				copyJobQueue.pop();

				if (businessService->IsSocketDead(jobData.socket)) {
					continue;
				}
				// DataCopy���� ���� ioInfo ��� free
				if (!businessService->SessionCheck(jobData.socket)) { // ���ǰ� ���� => �α��� ���� �б�
					// �α��� ���� ���� ó��
					businessService->StatusLogout(jobData.socket, jobData.direction, jobData.msg.c_str());
					// ���ǰ� ���� => �α��� ���� �б� ��
				}
				else { // ���ǰ� ������ => ���� �Ǵ� ä�ù� ����

					int status = businessService->GetStatus(
						jobData.socket);
					
					if (status == STATUS_WAITING && jobData.direction != -1) { // ���� ���̽�
						// ���� ó�� �Լ�
						businessService->StatusWait(jobData.socket, status, jobData.direction,
							jobData.msg.c_str());
					}
					else if (status == STATUS_CHATTIG) { // ä�� �� ���̽�
						// ä�ù� ó�� �Լ�

						if (jobData.direction == FILE_SEND) { // ���� ���� ���̽�
							businessService->StatusFile(jobData.socket);
						}
						else { // �Ϲ� ä���϶�
							businessService->StatusChat(jobData.socket, status, jobData.direction,
								jobData.msg.c_str());
						}
						
					}
				}
			}
		}
		else {  // ���۾��� �� �ٸ� �����尡 ���� �� �ְ�
			Sleep(1);
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
			
			businessService->ClientExit(sock);
			MPool* mp = MPool::getInstance();
			mp->Free(ioInfo);
		} else if (READ == ioInfo->serverMode
			|| READ_MORE == ioInfo->serverMode) { // Recv �� �⺻ ����
			// ������ �б� ����
			short remainByte = min(bytesTrans, BUF_SIZE); // �ʱ� Remain Byte
			bool recvMore = false;

			// jobQueue�� �����͸� �ѹ��� ������� �ڷᱸ��
			queue<JOB_DATA> packetQueue;

			while (true) {
				remainByte = businessService->PacketReading(ioInfo, remainByte);
				// �� ���� �� ���� ����
				// DataCopy������ ��� �޸� ���� ��ȯ
				if (remainByte >= 0) {
					JOB_DATA jobData;
					jobData.msg = businessService->DataCopy(ioInfo, &jobData.nowStatus,
						&jobData.direction);
					jobData.socket = sock;
					packetQueue.push(jobData);
					// packetQueue ä��
				}
				
				if (remainByte == 0) {
					MPool* mp = MPool::getInstance();
					mp->Free(ioInfo);
					break;
				}
				else if (remainByte < 0) { // ���� ��Ŷ ���� || ��� �� ������ -> ���޾ƾ���
					businessService->getIocpService()->RecvMore(
						sock, ioInfo); // ��Ŷ ���ޱ� & �⺻ ioInfo ����
					recvMore = true;
					break;
				}
			}

			EnterCriticalSection(&queueCs); // jobQueue LockȽ���� ���δ�
			while (!packetQueue.empty()) { // packetQueue -> jobQueue 
				JOB_DATA jobData = packetQueue.front();
				packetQueue.pop();
				jobQueue.push(jobData);
			}
			LeaveCriticalSection(&queueCs);
			// jobQueue�� �ѹ��� Insert
			
			if (!recvMore) { // recvMore�� �ƴϸ� �ش� socket�� �ޱ� ������ ����Ѵ�
				businessService->getIocpService()->Recv(
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

	SetConsoleTextAttribute(
		GetStdHandle(STD_OUTPUT_HANDLE), 10);

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "WSAStartup() error!" << endl;
		exit(1);
	}

	// Completion Port ����
	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	GetSystemInfo(&sysInfo);

	// Overlapped IO���� ������ �����
	// TCP ����Ұ�
	hServSock = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0,
		WSA_FLAG_OVERLAPPED);

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = PF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY���� ��� IP���� ������ �͵� ��û �����Ѵٴ� ��
	servAdr.sin_port = htons(atoi(SERVER_PORT));

	if (bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR) {
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
	cout << "port number : " << SERVER_PORT << endl;

	int process = sysInfo.dwNumberOfProcessors;
	cout << "Server CPU num : " << process << endl;

	MPool* mp = MPool::getInstance(); // �޸�Ǯ �ʱ�ȭ ����
	CharPool* charPool = CharPool::getInstance(); // �޸�Ǯ �ʱ�ȭ ����

	InitializeCriticalSectionAndSpinCount(&queueCs, 2000);

	businessService = new BusinessService::BusinessService();
	
	// Thread Pool Client���� ��Ŷ �޴� ����
	for (int i = 0; i < process; i++) {
		// ������� HandleThread�� hComPort CP ������Ʈ�� �Ҵ��Ѵ�
		_beginthreadex(NULL, 0, RecvThread, (LPVOID)hComPort, 0, NULL);
	}

	// Thread Pool �����Ͻ� ���� ���
	for (int i = 0; i < 2 * process; i++) {
		_beginthreadex(NULL, 0, WorkThread, NULL, 0, NULL);
	}

	// Thread Pool �α� ���� SQL ���࿡ ����
	for (int i = 0; i < (process * 2) / 3; i++) {
		_beginthreadex(NULL, 0, SQLThread, NULL, 0, NULL);
	}

	// Thread Pool BroadCast ����
	for (int i = 0; i < (process * 4) / 3; i++) {
		_beginthreadex(NULL, 0, SendThread, NULL, 0, NULL);
	}

	while (true) {
		SOCKET hClientSock;
		SOCKADDR_IN clntAdr;
		int addrLen = sizeof(clntAdr);
		hClientSock = accept(hServSock, (SOCKADDR*) &clntAdr, &addrLen);
		
		businessService->InsertLiveSocket(hClientSock, clntAdr);
	
		// Completion Port �� accept�� ���� ����
		CreateIoCompletionPort((HANDLE) hClientSock, hComPort,
				(DWORD) hClientSock, 0);

		businessService->getIocpService()->Recv(hClientSock);

		// �ʱ� ���� �޼��� Send
		string str = "������ ȯ���մϴ�!\n";
		str += loginBeforeMessage;

		businessService->getIocpService()->SendToOneMsg(
				str.c_str(), hClientSock, STATUS_LOGOUT);
	}
	DeleteCriticalSection(&queueCs);
	
	delete businessService;

	return 0;
}
