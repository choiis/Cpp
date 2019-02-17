
#ifndef FILESERVICE_H_
#define FILESERVICE_H_

#include <winsock2.h>
#include <memory>
#include <algorithm>
#include <iostream>
#include <direct.h>
#include <list>
#include <string>
#include <unordered_map>
#include "common.h"

namespace FileService {

class FileService {
private:
	// udp ���� �ʱ�ȭ�� �ʿ�
	WSADATA wsaData;
	// udp ����
	SOCKET udpSocket;
	// udp Send ����
	SOCKET udpSendSocket;
public:
	// ������
	FileService();
	// �Ҹ���
	virtual ~FileService();
	// UDP��������
	// �޼���ۿ��� �� ����Ʈ������ ����ȭ 
	void SendToRoomFile(FILE* fp, const string dir, shared_ptr<ROOM_DATA> second, const unordered_map<SOCKET, string>& liveSocket);
	// ä�ù濡���� ���� ����� ���̽�
	string  RecvFile(SOCKET sock);
};

}/* namespace Service */

#endif /* FILESERVICE_H_ */