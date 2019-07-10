/*
* BusinessService.cpp
*
*  Created on: 2019. 2. 13.
*      Author: choiis1207
*/

#include "FileService.h"

namespace FileService {

	FileService::FileService() {
		udpSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

		udpSendSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

		struct sockaddr_in local_addr;

		memset(&local_addr, 0, sizeof(local_addr));
		local_addr.sin_family = AF_INET;
		local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		local_addr.sin_port = htons(atoi(UDP_PORT)); // UDP port

		if (bind(udpSocket, (SOCKADDR *)&local_addr, sizeof(local_addr)) == SOCKET_ERROR) {
			cout << "bind() error!" << endl;
		}
	}


	FileService::~FileService() {
		closesocket(udpSocket);

		closesocket(udpSendSocket);
	}

	// UDP��������
	// �޼���ۿ��� �� ����Ʈ������ ����ȭ 
	void FileService::SendToRoomFile(FILE* fp, const string& dir, shared_ptr<ROOM_DATA> second, const map<SOCKET, string>& liveSocket) {
		
		list<SOCKET> userList = second->userList;

		for_each(userList.begin(), userList.end(), [&](SOCKET socket) {
		
			SOCKADDR_IN sendAddr;
			memset(&sendAddr, 0, sizeof(sendAddr));
			sendAddr.sin_family = AF_INET;
			string ip = liveSocket.find(socket)->second;
			sendAddr.sin_addr.s_addr = inet_addr(ip.c_str()); // ���� ��� IP ��ȣ
			sendAddr.sin_port = htons(atoi(UDP_PORT_SEND)); // UDP SEND ��Ʈ 
			int addrSize = sizeof(sendAddr);
			if (connect(udpSendSocket, (SOCKADDR *)&sendAddr, sizeof(sendAddr)) == SOCKET_ERROR) {
				cout << "bind error" << endl;
				return;
			}

			char fileName[BUF_SIZE];
			strncpy(fileName, dir.c_str(), BUF_SIZE);

			cout << fileName << " ������ Ŭ���̾�Ʈ " << ip << " �� �����մϴ�" << endl;
			char buf[FILE_BUF_SIZE];
			// �����̸� ����	
			int sendBytes = sendto(udpSendSocket, fileName, strlen(fileName), 0, (SOCKADDR *)&sendAddr, addrSize);
			// ��������
			while (true) {
				sendBytes = fread(buf, sizeof(char), FILE_BUF_SIZE, fp);
				sendto(udpSendSocket, buf, sendBytes, 0, (SOCKADDR *)&sendAddr, addrSize);
				if (sendBytes == EOF || sendBytes == 0) {
					break;
				}
				else {
					Sleep(1);
				}
			}
		});

		cout << "��ü ���޿Ϸ�" << endl;
		
	}

	// ä�ù濡���� ���� ����� ���̽�
	string FileService::RecvFile(SOCKET sock) {

		SOCKADDR_IN client_addr;

		memset(&client_addr, 0, sizeof(client_addr));
		client_addr.sin_family = AF_INET;
		client_addr.sin_addr.s_addr = htons(INADDR_ANY); // ���� ��� IP ��ȣ
		client_addr.sin_port = htons(atoi(UDP_PORT)); // UDP ��Ʈ 

		int len_addr = sizeof(client_addr);

		FILE * fp;

		char fileName[BUF_SIZE];// �ι�°�� ���� �̸��� �޴´�
		int readBytes = recvfrom(this->udpSocket, fileName, BUF_SIZE, 0, (SOCKADDR*)&client_addr, &len_addr);
		fileName[readBytes] = '\0';
		// char fileDir[BUF_SIZE] = "C:/Users/choiis1207/Downloads/"; // �ٿ�ε� ������ ��� ����
		char fileDir[BUF_SIZE] = "Downloads"; // �ٿ�ε� ���� ��λ���
		_mkdir(fileDir);
		strcat(fileDir, "/");
		strcat(fileDir, fileName);
		// ���ο��� ����
		fp = fopen(fileDir, "wb");

		char buf[FILE_BUF_SIZE];

		while (true) {
			readBytes = recvfrom(this->udpSocket, buf, FILE_BUF_SIZE, 0, (SOCKADDR*)&client_addr, &len_addr);
			//cout << "readBytes " << readBytes << endl;
			if (readBytes == 0 || readBytes == EOF) {
				break;
			}
			fwrite(buf, sizeof(char), readBytes, fp);
		}
		fclose(fp);
		// ���ο��� �ݱ�
		return string(fileDir);
	}
}