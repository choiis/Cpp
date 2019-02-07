/*
 * BusinessService.cpp
 *
 *  Created on: 2019. 1. 17.
 *      Author: choiis1207
 */

#include "BusinessService.h"

namespace Service {

BusinessService::BusinessService() {
	// �Ӱ迵�� Object ����
	InitializeCriticalSectionAndSpinCount(&idCs, 2000);

	InitializeCriticalSectionAndSpinCount(&userCs, 2000);

	InitializeCriticalSectionAndSpinCount(&roomCs, 2000);

	InitializeCriticalSectionAndSpinCount(&sqlCs, 2000);

	InitializeCriticalSectionAndSpinCount(&sendCs, 2000);

	iocpService = new IocpService::IocpService();
	
	dao = new Dao();
}

BusinessService::~BusinessService() {

	delete dao;

	delete iocpService;
	// �Ӱ迵�� Object ��ȯ
	DeleteCriticalSection(&idCs);

	DeleteCriticalSection(&userCs);

	DeleteCriticalSection(&roomCs);

	DeleteCriticalSection(&sqlCs);

	DeleteCriticalSection(&sendCs);
}

void BusinessService::SQLwork() {

	SQL_DATA sqlData;
	
	EnterCriticalSection(&sqlCs);
	
	if (sqlQueue.size() != 0) {
		sqlData = sqlQueue.front();
		sqlQueue.pop();
	
		switch (sqlData.direction)
		{
		case UPDATE_USER: // ID���� update
			dao->UpdateUser(sqlData.vo);
			break;
		case INSERT_LOGIN: // �α��� ���� insert
			dao->InsertLogin(sqlData.vo);
			break;
		case INSERT_DIRECTION: // ���� �α� insert
			dao->InsertDirection(sqlData.vo);
			break;
		case INSERT_CHATTING: // ä�� �α� insert
			dao->InsertChatting(sqlData.vo);
			break; 
		default:
			break;
		}
	}
	LeaveCriticalSection(&sqlCs);
}


void BusinessService::Sendwork() {
	
	Send_DATA sendData;

	if (sendQueue.size() != 0) {
		EnterCriticalSection(&sendCs);
		sendData = sendQueue.front();
		sendQueue.pop();
		LeaveCriticalSection(&sendCs);
		switch (sendData.direction)
		{
		case SEND_ME: // Send to One
			iocpService->SendToOneMsg(sendData.msg.c_str(), sendData.mySocket, sendData.status);
			break;
		case SEND_ROOM: // Send to Room
			// LockCount�� ���� �� => �� ����Ʈ�� ������� ��
			{
				EnterCriticalSection(&roomCs);
				auto iter = roomMap.find(sendData.roomName);
				shared_ptr<ROOM_DATA> second;
				if (iter != roomMap.end()) {
					second = iter->second;
				}
				LeaveCriticalSection(&roomCs);

				if (second->listCs.LockCount == -1) {
					EnterCriticalSection(&second->listCs);
					iocpService->SendToRoomMsg(sendData.msg.c_str(), second->userList, sendData.status);
					LeaveCriticalSection(&second->listCs);
				}
				break;
			}
		default:
			break;
		}
	}
	else {
		Sleep(1);
	}
	
}

// InsertSendQueue ����ȭ
void BusinessService::InsertSendQueue(int direction, string msg, string roomName, SOCKET socket, int status){

	EnterCriticalSection(&sendCs);
	// SendQueue�� Insert
	Send_DATA sendData;
	if (direction == SEND_ME) {
		sendData.direction = direction;
		sendData.msg = msg;
		sendData.mySocket = socket;
		sendData.status = status;
	}
	else { //  SEND_ROOM
		sendData.direction = direction;
		sendData.msg = msg;
		sendData.roomName = roomName;
		sendData.status = status;
	}
	
	sendQueue.push(sendData);
	LeaveCriticalSection(&sendCs);
}

// �ʱ� �α���
// �������� �߰�
void BusinessService::InitUser(const char *id, SOCKET sock, const char* nickName) {

	string msg;

	PER_HANDLE_DATA userInfo;
	// ������ ���� ���� �ʱ�ȭ
	userInfo.status = STATUS_WAITING;

	strncpy(userInfo.userId, id, NAME_SIZE);
	strncpy(userInfo.roomName, "", NAME_SIZE);
	strncpy(userInfo.userName, nickName, NAME_SIZE);

	EnterCriticalSection(&userCs);
	// �������� insert
	this->userMap[sock] = userInfo;
	cout << "START user : " << nickName << endl;
	cout << "���� ���� �ο� �� : " << userMap.size() << endl;
	LeaveCriticalSection(&userCs);

	Vo vo; // DB SQL���� �ʿ��� Data
	vo.setUserId(userInfo.userId);
	vo.setNickName(userInfo.userName);

	EnterCriticalSection(&sqlCs);
	
	SQL_DATA sqlData1, sqlData2;
	sqlData1.vo = vo;
	sqlData1.direction = UPDATE_USER;
	sqlQueue.push(sqlData1); // �ֱ� �α��α�� ������Ʈ
	sqlData2.vo = vo;
	sqlData2.direction = INSERT_LOGIN;
	sqlQueue.push(sqlData2); // �α��� DB�� ���

	LeaveCriticalSection(&sqlCs);

	msg = "������ ȯ���մϴ�!\n";
	msg.append(waitRoomMessage);

	InsertSendQueue(SEND_ME, msg, "", sock, STATUS_WAITING);
}

// ���� �������� ����
void BusinessService::ClientExit(SOCKET sock) {

	if (closesocket(sock) != SOCKET_ERROR) {
		cout << "���� ���� " << endl;

		if (userMap.find(sock) != userMap.end()) {

			// �������� �������� => �������� �� ���� ���� �ʿ�
			char roomName[NAME_SIZE];
			char name[NAME_SIZE];
			char id[NAME_SIZE];

			strncpy(roomName, userMap.find(sock)->second.roomName,
			NAME_SIZE);
			strncpy(name, userMap.find(sock)->second.userName, NAME_SIZE);
			strncpy(id, userMap.find(sock)->second.userId, NAME_SIZE);
			// �α��� Set���� out
			EnterCriticalSection(&idCs);
			idSet.erase(id);
			LeaveCriticalSection(&idCs);

			// ���̸� �ӽ� ����
			if (userMap.find(sock)->second.status == STATUS_CHATTIG) { // �濡 �������� ���
				// ������ ��� ���� out

				EnterCriticalSection(&roomCs);
				
				if (roomMap.find(roomName) != roomMap.end()) { // null üũ �켱

					EnterCriticalSection(
							&roomMap.find(roomName)->second->listCs);
					roomMap.find(roomName)->second->userList.remove(sock);

					LeaveCriticalSection(
						&roomMap.find(roomName)->second->listCs);

					if ((roomMap.find(roomName)->second)->userList.size() == 0) { // ���ο� 0���̸� �� ����
						DeleteCriticalSection(
							&roomMap.find(roomName)->second->listCs);
						// �� ���� ���� CS�� Delete ģ��
						
						roomMap.erase(roomName);
						LeaveCriticalSection(&roomCs);
					} else {
						string sendMsg = name;
						// �̸����� ���� NAME_SIZE����
						sendMsg += " ���� �������ϴ�!";

						EnterCriticalSection(&userCs);
						string roomN = userMap.find(sock)->second.roomName;
						LeaveCriticalSection(&userCs);
						int roomMapCnt = roomMap.count(roomN);

						if (roomMapCnt != 0) { // null �˻�
							// SendQueue�� Insert
							InsertSendQueue(SEND_ROOM, sendMsg, userMap.find(sock)->second.roomName, 0, STATUS_CHATTIG);
						}

						LeaveCriticalSection(&roomCs);
					}
				}
				else {
					LeaveCriticalSection(&roomCs);
				}
			}
			EnterCriticalSection(&userCs);
			userMap.erase(sock); // ���� ���� ���� ����
			cout << "���� ���� �ο� �� : " << userMap.size() << endl;
			LeaveCriticalSection(&userCs);
			
		}

	}
}

// �α��� ���� ����ó��
// ���ǰ� ���� �� ����
void BusinessService::StatusLogout(SOCKET sock, int direction, const char *message) {

	string msg = "";

	if (direction == USER_MAKE) { // 1�� ��������

		char *sArr[3] = { NULL, };
		char message2[BUF_SIZE];
		strncpy(message2, message, BUF_SIZE);
		char *ptr = strtok(message2, "\\"); // ���� ���ڿ��� �������� ���ڿ��� �ڸ�
		int i = 0;
		while (ptr != NULL)            // �ڸ� ���ڿ��� ������ ���� ������ �ݺ�
		{
			sArr[i] = ptr;           // ���ڿ��� �ڸ� �� �޸� �ּҸ� ���ڿ� ������ �迭�� ����
			i++;                       // �ε��� ����
			ptr = strtok(NULL, "\\");   // ���� ���ڿ��� �߶� �����͸� ��ȯ
		}
		// ��ȿ�� ���� �ʿ�
		if (sArr[0] != NULL && sArr[1] != NULL && sArr[2] != NULL) {
			
			Vo vo;
			vo.setUserId(sArr[0]);
			vo = dao->selectUser(vo);

			if (strcmp(vo.getUserId(), "") == 0) { // ID �ߺ�üũ => ���� ����

				vo.setUserId(sArr[0]);
				vo.setPassword(sArr[1]);
				vo.setNickName(sArr[2]);

				dao->InsertUser(vo); 

				msg.append(sArr[0]);
				msg.append("���� ���� �Ϸ�!\n");
				msg.append(loginBeforeMessage);

				InsertSendQueue(SEND_ME, msg, "", sock, STATUS_LOGOUT);

			} else { // ID�ߺ�����

				msg.append("�ߺ��� ���̵� �ֽ��ϴ�!\n");
				msg.append(loginBeforeMessage);

				InsertSendQueue(SEND_ME, msg, "", sock, STATUS_LOGOUT);
			}
		}

	} else if (direction == USER_ENTER) { // 2�� �α��� �õ�

		char *sArr[2] = { NULL, };
		char message2[BUF_SIZE];
		strncpy(message2, message, BUF_SIZE);
		char *ptr = strtok(message2, "\\"); // ���� ���ڿ��� �������� ���ڿ��� �ڸ�
		int i = 0;
		while (ptr != NULL)            // �ڸ� ���ڿ��� ������ ���� ������ �ݺ�
		{
			sArr[i] = ptr;           // ���ڿ��� �ڸ� �� �޸� �ּҸ� ���ڿ� ������ �迭�� ����
			i++;                       // �ε��� ����
			ptr = strtok(NULL, "\\");   // ���� ���ڿ��� �߶� �����͸� ��ȯ
		}
		if (sArr[0] != NULL && sArr[1] != NULL) {
			
			Vo vo;
			vo.setUserId(sArr[0]);
			vo = dao->selectUser(vo);

			if (strcmp(vo.getUserId() , "") == 0) { // ���� ����

				msg.append("���� �����Դϴ� ���̵� Ȯ���ϼ���!\n");
				msg.append(loginBeforeMessage);

				// SendQueue�� Insert
				InsertSendQueue(SEND_ME, msg, "", sock, STATUS_LOGOUT);

			} else if (	strcmp(vo.getPassword(), sArr[1]) == 0	) { // ��й�ȣ ��ġ
				EnterCriticalSection(&idCs);
				unordered_set<string>::const_iterator it = idSet.find(sArr[0]);
				if (it != idSet.end()) { // �ߺ��α��� ��ȿ�� �˻�
					LeaveCriticalSection(&idCs); // Case Lock ����
					msg.append("�ߺ� �α����� �ȵ˴ϴ�!\n");
					msg.append(loginBeforeMessage);

					// SendQueue�� Insert
					InsertSendQueue(SEND_ME, msg, "", sock, STATUS_LOGOUT);

				} else { // �ߺ��α��� X
					idSet.insert(sArr[0]);
					LeaveCriticalSection(&idCs); //Init�κ� �ι����� ����
					InitUser(sArr[0], sock , vo.getNickName()); // �������� ����
				}

			} else { // ��й�ȣ Ʋ��

				msg.append("��й�ȣ Ʋ��!\n");
				msg.append(loginBeforeMessage);

				InsertSendQueue(SEND_ME, msg, "", sock, STATUS_LOGOUT);

			}
		}

	} else { // �׿� ��ɾ� �Է�
		string sendMsg = errorMessage;
		sendMsg += waitRoomMessage;
		InsertSendQueue(SEND_ME, sendMsg, "", sock, STATUS_LOGOUT);
	}

}

// ���ǿ����� ���� ó��
// ���ǰ� ����
void BusinessService::StatusWait(SOCKET sock, int status, int direction,
	const char *message) {

	string name = userMap.find(sock)->second.userName;
	string msg = string(message);
	// ���ǿ��� �̸� ���� ����

	if (direction == ROOM_MAKE) { // ���ο� �� ���鶧

		// ��ȿ�� ���� ����
		EnterCriticalSection(&roomCs);
		int cnt = roomMap.count(msg);

		if (cnt != 0) { // ���̸� �ߺ�
			LeaveCriticalSection(&roomCs);
			msg += "�̹� �ִ� �� �̸��Դϴ�!\n";
			msg += waitRoomMessage;
			InsertSendQueue(SEND_ME, msg, "", sock, STATUS_WAITING);
			// �ߺ� ���̽��� �� ���� �� ����
		} else { // ���̸� �ߺ� �ƴ� ���� ����
				 // ���ο� �� ���� ����
			shared_ptr<ROOM_DATA> roomData = make_shared<ROOM_DATA>();
			list<SOCKET> chatList;
			chatList.push_back(sock);
			InitializeCriticalSection(&roomData->listCs);
			// �� ����Ʈ�� CS��ü Init
			roomData->userList = chatList;
			roomMap[msg] = roomData;

			LeaveCriticalSection(&roomCs);

			// User�� ���� ���� �ٲ۴�
			EnterCriticalSection(&userCs);
			strncpy((userMap.find(sock))->second.roomName, msg.c_str(),
			NAME_SIZE);
			(userMap.find(sock))->second.status =
			STATUS_CHATTIG;
			LeaveCriticalSection(&userCs);

			msg += " ���� �����Ǿ����ϴ�.";
			msg += chatRoomMessage;
			// cout << "���� ������ �� ���� : " << roomMap.size() << endl;
			InsertSendQueue(SEND_ME, msg, "", sock, STATUS_CHATTIG);

		}

	} else if (direction == ROOM_ENTER) { // �� ���� ��û

		// ��ȿ�� ���� ����
		EnterCriticalSection(&roomCs);

		if (roomMap.find(msg) == roomMap.end()) { // �� ��ã��
			LeaveCriticalSection(&roomCs);
			msg = "���� �� �Դϴ�!\n";
			msg += waitRoomMessage;

			InsertSendQueue(SEND_ME, msg, "", sock, STATUS_WAITING);

		} else {
			// LeaveCriticalSection(&roomCs);

			EnterCriticalSection(&userCs);
			strncpy(userMap.find(sock)->second.roomName, msg.c_str(),
			NAME_SIZE);
			userMap.find(sock)->second.status = STATUS_CHATTIG;
			LeaveCriticalSection(&userCs);

			string sendMsg = name;
			sendMsg += " ���� �����ϼ̽��ϴ�. ";
			sendMsg += chatRoomMessage;
			if (roomMap.find(msg) != roomMap.end()) {
				EnterCriticalSection(&roomMap.find(msg)->second->listCs); // ���� Lock
				//���� �����ϱ� ������ insert
				roomMap.find(msg)->second->userList.push_back(sock);
				LeaveCriticalSection(&roomMap.find(msg)->second->listCs); // ���� Lock
			}
			
			LeaveCriticalSection(&roomCs); 
			InsertSendQueue(SEND_ROOM, sendMsg, msg, 0, STATUS_CHATTIG);
		}

	} else if (direction == ROOM_INFO) { // �� ���� ��û��

		string str;
		if (roomMap.size() == 0) {
			str = "������� ���� �����ϴ�";
		} else {
			str += "�� ���� ����Ʈ";
			EnterCriticalSection(&roomCs);
			unordered_map<string, shared_ptr<ROOM_DATA>> roomCopyMap = roomMap;
			LeaveCriticalSection(&roomCs);
			// ����ȭ ���� ���� ��������
			unordered_map<string, shared_ptr<ROOM_DATA>>::const_iterator iter;

			// �������� ���ڿ��� �����
			for (iter = roomCopyMap.begin(); iter != roomCopyMap.end();
					iter++) {

				str += "\n";
				str += iter->first.c_str();
				str += ":";
				str += to_string((iter->second)->userList.size());
				str += "��";
			}

		}

		InsertSendQueue(SEND_ME, str, "", sock, STATUS_WAITING);

	} else if (direction == ROOM_USER_INFO) { // ���� ���� ��û��
		string str = "���� ���� ����Ʈ";

		EnterCriticalSection(&userCs);
		unordered_map<SOCKET, PER_HANDLE_DATA> userCopyMap = userMap;
		LeaveCriticalSection(&userCs);
		// ����ȭ ���� ���� ��������
		unordered_map<SOCKET, PER_HANDLE_DATA>::const_iterator iter;
		for (iter = userCopyMap.begin(); iter != userCopyMap.end(); iter++) {
			if (str.length() >= 1024) {
				break;
			}
			str += "\n";
			str += (iter->second).userName;
			str += ":";
			if ((iter->second).status == STATUS_WAITING) {
				str += "����";
			} else {
				str += (iter->second).roomName;
			}
		}
		InsertSendQueue(SEND_ME, str, "", sock, STATUS_WAITING);

	} else if (direction == WHISPER) { // �ӼӸ�

		char *sArr[2] = { NULL, };
		char message2[BUF_SIZE];
		strncpy(message2, message, BUF_SIZE);
		char *ptr = strtok(message2, "\\"); // ���� ���ڿ��� �������� ���ڿ��� �ڸ�
		int i = 0;
		while (ptr != NULL)            // �ڸ� ���ڿ��� ������ ���� ������ �ݺ�
		{
			sArr[i] = ptr;           // ���ڿ��� �ڸ� �� �޸� �ּҸ� ���ڿ� ������ �迭�� ����
			i++;                       // �ε��� ����
			ptr = strtok(NULL, "\\");   // ���� ���ڿ��� �߶� �����͸� ��ȯ
		}
		if (sArr[0] != NULL && sArr[1] != NULL) {
			name = string(sArr[0]);
			msg = string(sArr[1]);

			string sendMsg;

			if (name.compare(userMap.find(sock)->second.userName) == 0) { // ���ο��� ����
				sendMsg = "�ڽſ��� ������ ������ �����ϴ�\n";
				sendMsg += waitRoomMessage;

				InsertSendQueue(SEND_ME, sendMsg, "", sock, STATUS_WAITING);

			} else {
				bool find = false;
				unordered_map<SOCKET, PER_HANDLE_DATA>::iterator iter;

				EnterCriticalSection(&userCs);
				for (iter = userMap.begin(); iter != userMap.end(); iter++) {
					if (name.compare(iter->second.userName) == 0) {
						find = true;
						sendMsg = userMap.find(sock)->second.userName;
						sendMsg += " �Կ��� �� �ӼӸ� : ";
						sendMsg += msg;

						InsertSendQueue(SEND_ME, sendMsg, "", iter->first, STATUS_WAITING);

						break;
					}
				}
				LeaveCriticalSection(&userCs);

				// �ӼӸ� ����� ��ã��
				if (!find) {
					sendMsg = name;
					sendMsg += " ���� ã�� �� �����ϴ�";

					InsertSendQueue(SEND_ME, sendMsg, "", sock, STATUS_WAITING);
				}
			}
		}

	} else if (msg.compare("6") == 0) { // �α׾ƿ�

		EnterCriticalSection(&idCs);
		idSet.erase(userMap.find(sock)->second.userId); // �α��� �¿��� ����
		LeaveCriticalSection(&idCs);

		EnterCriticalSection(&userCs);
		userMap.erase(sock); // ���� ���� ���� ����
		cout << "���� ���� �ο� �� : " << userMap.size() << endl;
		LeaveCriticalSection(&userCs);

		string sendMsg = loginBeforeMessage;
		InsertSendQueue(SEND_ME, sendMsg, "", sock, STATUS_LOGOUT);
	} else { // �׿� ��ɾ� �Է�
		string sendMsg = errorMessage;
		sendMsg += waitRoomMessage;
		// ������ �����̹Ƿ� STATUS_WAITING ���·� �����Ѵ�

		InsertSendQueue(SEND_ME, sendMsg, "", sock, STATUS_WAITING);
	}

	Vo vo; // DB SQL���� �ʿ��� Data
	vo.setNickName(name.c_str());
	vo.setMsg(message);
	vo.setDirection(direction);
	vo.setStatus(status);

	EnterCriticalSection(&sqlCs);
	SQL_DATA sqlData;
	sqlData.vo = vo;
	sqlData.direction = INSERT_DIRECTION;
	sqlQueue.push(sqlData); // ���� ��� insert
	LeaveCriticalSection(&sqlCs);
}

// ä�ù濡���� ���� ó��
// ���ǰ� ����
void BusinessService::StatusChat(SOCKET sock, int status, int direction,
	const char *message) {

	string name;
	string msg;

	// ���ǿ��� �̸� ���� ����
	name = userMap.find(sock)->second.userName;
	msg = string(message);

	if (msg.compare("\\out") == 0) { // ä�ù� ����

		string sendMsg;
		string roomName;
		sendMsg = name;
		sendMsg += " ���� �������ϴ�!";

		// EnterCriticalSection(&roomCs);
		// ���̸� �ӽ� ����
		roomName = string(userMap.find(sock)->second.roomName);

		EnterCriticalSection(&roomMap.find(roomName)->second->listCs);

		roomMap.find(roomName)->second->userList.remove(sock); // ������ ��� ���� out

		LeaveCriticalSection(&roomMap.find(roomName)->second->listCs);
		
		// LeaveCriticalSection(&roomCs);
		// SendQueue�� Insert

		InsertSendQueue(SEND_ROOM, sendMsg, userMap.find(sock)->second.roomName, 0, STATUS_CHATTIG);

		msg = waitRoomMessage;
		InsertSendQueue(SEND_ME, msg, "", sock, STATUS_WAITING);

		EnterCriticalSection(&userCs);
		strncpy(userMap.find(sock)->second.roomName, "", NAME_SIZE); // ���̸� �ʱ�ȭ
		userMap.find(sock)->second.status = STATUS_WAITING; // ���� ����
		LeaveCriticalSection(&userCs);
		
		EnterCriticalSection(&roomCs);
		if ((roomMap.find(roomName)->second)->userList.size() == 0) { // ���ο� 0���̸� �� ����
			DeleteCriticalSection(&roomMap.find(roomName)->second->listCs);
			roomMap.erase(roomName);
		}
		LeaveCriticalSection(&roomCs);
	} else { // ä�ù濡�� ä����

		string sendMsg;
		sendMsg = name;
		sendMsg += " : ";
		sendMsg += msg;

		char roomName[NAME_SIZE];

		EnterCriticalSection(&userCs);
		unordered_map<string, shared_ptr<ROOM_DATA>>::iterator it = roomMap.find(
				userMap.find(sock)->second.roomName);
		strncpy(roomName, userMap.find(sock)->second.roomName, NAME_SIZE);
		LeaveCriticalSection(&userCs);
	
		Vo vo; // DB SQL���� �ʿ��� Data
		vo.setNickName(name.c_str());
		vo.setRoomName(roomName);
		vo.setMsg(msg.c_str());
		vo.setDirection(0);
		vo.setStatus(0);

		EnterCriticalSection(&sqlCs);

		SQL_DATA sqlData;
		sqlData.vo = vo;
		sqlData.direction = INSERT_CHATTING;
		sqlQueue.push(sqlData); // ��ȭ ��� ������Ʈ

		LeaveCriticalSection(&sqlCs);

		if (it != roomMap.end()) { // null �˻�
			InsertSendQueue(SEND_ROOM, sendMsg, userMap.find(sock)->second.roomName, 0, STATUS_CHATTIG);
		}
	}

}

// Ŭ���̾�Ʈ���� ���� ������ ������ ����ü ����
// ��� �޸� ���� ��ȯ
string BusinessService::DataCopy(LPPER_IO_DATA ioInfo, int *status,
	int *direction) {
	
	copy(((char*)ioInfo->recvBuffer) + 2, ((char*)ioInfo->recvBuffer) + 6,
		(char*)status);
	copy(((char*)ioInfo->recvBuffer) + 6, ((char*)ioInfo->recvBuffer) + 10,
		(char*)direction);

	CharPool* charPool = CharPool::getInstance();
	char* msg = charPool->Malloc(); // 512 Byte���� ī�� ����
	copy(((char*)ioInfo->recvBuffer) + 10,
		((char*)ioInfo->recvBuffer) + 10
		+ min(ioInfo->bodySize, (DWORD)BUF_SIZE), msg);

	// �� ���� �޾����� �Ҵ� ����
	charPool->Free(ioInfo->recvBuffer);
	string str = string(msg);
	charPool->Free(msg);
	
	return str;
}

// ��Ŷ ������ �б�
short BusinessService::PacketReading(LPPER_IO_DATA ioInfo, short bytesTrans) {
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
		else if(bytesTrans == 1) { // ��� ����
			copy(ioInfo->buffer, ioInfo->buffer + bytesTrans,
				(char*) &(ioInfo->bodySize) ); // �������ִ� Byte���� ī��
			ioInfo->recvByte = bytesTrans;
			ioInfo->totByte = 0;
			return -1;
		}
		else {
			return 0;
		}
	} else { // �� �б� (BodySize©�����)
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

// �α��� ���� üũ �� ��ȯ
bool BusinessService::SessionCheck(SOCKET sock) {
	// userMap������ ����CS
	EnterCriticalSection(&this->userCs);
	int cnt = userMap.count(sock);
	LeaveCriticalSection(&this->userCs);

	if (cnt > 0) {
		return true;
	} else {
		return false;
	}
}

// Ŭ���̾�Ʈ�� �������� ��ȯ
int BusinessService::GetStatus(SOCKET sock) {
	EnterCriticalSection(&this->userCs);
	int status = userMap.find(sock)->second.status;
	LeaveCriticalSection(&this->userCs);
	return status;
}

IocpService::IocpService* BusinessService::getIocpService() {
	return this->iocpService;
}

} /* namespace Service */
