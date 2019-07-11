/*
* BusinessService.cpp
*
*  Created on: 2019. 1. 17.
*      Author: choiis1207
*/

#include "BusinessService.h"

namespace BusinessService {

	BusinessService::BusinessService() {

		// �Ӱ迵�� Object ����
		InitializeCriticalSectionAndSpinCount(&idCs, 2000);

		InitializeCriticalSectionAndSpinCount(&userCs, 2000);

		InitializeCriticalSectionAndSpinCount(&roomCs, 2000);

		iocpService = new IocpService::IocpService();

		fileService = new FileService::FileService();

		dao = new Dao();
	}

	BusinessService::~BusinessService() {

		delete dao;

		delete iocpService;

		delete fileService;

		// �Ӱ迵�� Object ��ȯ
		DeleteCriticalSection(&idCs);

		DeleteCriticalSection(&userCs);

		DeleteCriticalSection(&roomCs);

	}

	void BusinessService::SQLwork() {

		if (!sqlQueue.empty()) { // SQL insert �ѹ���
			queue<SQL_DATA> copySqlQueue;

			{
				lock_guard<mutex> guard(sqlCs);  // Lock�ּ�ȭ
				copySqlQueue = sqlQueue;
				queue<SQL_DATA> emptyQueue; // �� ť
				swap(sqlQueue, emptyQueue); // �� ť�� �ٲ�ġ��
			}

			while (!copySqlQueue.empty()) { // ���� ��Ŷ ������ �Ѳ����� ó��
				SQL_DATA sqlData = copySqlQueue.front();
				copySqlQueue.pop();

				switch (sqlData.direction)
				{
				case SqlWork::UPDATE_USER: // ID���� update
					dao->UpdateUser(sqlData.vo);
					break;
				case SqlWork::INSERT_LOGIN: // �α��� ���� insert
					dao->InsertLogin(sqlData.vo);
					break;
				case SqlWork::INSERT_DIRECTION: // ���� �α� insert
					dao->InsertDirection(sqlData.vo);
					break;
				case SqlWork::INSERT_CHATTING: // ä�� �α� insert
					dao->InsertChatting(sqlData.vo);
					break;
				default:
					break;
				}
			}
		}
		else {
			Sleep(1);
		}

	}


	void BusinessService::Sendwork() {

		Send_DATA sendData;
		if (!sendQueue.empty()) {

			queue<Send_DATA> copySendQueue;
			{
				lock_guard<mutex> guard(sendCs);  // Lock�ּ�ȭ
				copySendQueue = sendQueue;
				queue<Send_DATA> emptyQueue; // �� ť
				swap(sendQueue, emptyQueue); // �� ť�� �ٲ�ġ��
			}

			while (!copySendQueue.empty()) { // ���� ��Ŷ ������ �Ѳ����� ó��
				Send_DATA sendData = copySendQueue.front();
				copySendQueue.pop();
				
				switch (sendData.direction)
				{
				case SendTo::SEND_ME: // Send to One
					iocpService->SendToOneMsg(sendData.msg.c_str(), sendData.mySocket, sendData.status);
					break;
				case SendTo::SEND_ROOM: // Send to Room
					// LockCount�� ���� �� => �� ����Ʈ�� ������� ��
				{
					EnterCriticalSection(&roomCs);
					auto iter = roomMap.find(sendData.roomName);
					shared_ptr<ROOM_DATA> second = nullptr;
					if (iter != roomMap.end()) {
						second = iter->second;
					}
					LeaveCriticalSection(&roomCs);

					if (second != nullptr) {
						lock_guard<recursive_mutex> guard(second->listCs);
						iocpService->SendToRoomMsg(sendData.msg.c_str(), second->userList, sendData.status);
					}
					break;
				}
				case SendTo::SEND_FILE:
				{
					 string dir = sendData.msg;
					 FILE* fp = fopen(dir.c_str(), "rb");
					if (fp == NULL) {
						cout << "���� ���� ����" << endl;
						return;
					}

					int idx;
					while ((idx = dir.find("/")) != -1) { // ���ϸ� ����
						dir.erase(0, idx + 1);
					}

					EnterCriticalSection(&roomCs);
					auto iter = roomMap.find(sendData.roomName);
					shared_ptr<ROOM_DATA> second = nullptr;
					if (iter != roomMap.end()) {
						second = iter->second;
					}
					LeaveCriticalSection(&roomCs);

					// �� ���� CS
					if (second != nullptr) {
						lock_guard<recursive_mutex> guard(second->listCs);
						fileService->SendToRoomFile(fp, dir, second, liveSocket);
					}
					fclose(fp);
					break;
				}
				default:
					break;
				}
			}
		}
		else {
		
			Sleep(1);
		}

	}

	// InsertSendQueue ����ȭ
	void BusinessService::InsertSendQueue(SendTo direction, const string& msg, const string& roomName, SOCKET socket, ClientStatus status) {

		// SendQueue�� Insert
		Send_DATA sendData;
		if (direction == SendTo::SEND_ME) {
			sendData.direction = direction;
			sendData.msg = msg;
			sendData.mySocket = socket;
			sendData.status = status;
		}
		else { //  SendTo::SEND_ROOM
			sendData.direction = direction;
			sendData.msg = msg;
			sendData.roomName = roomName;
			sendData.status = status;
		}
		{
			lock_guard<mutex> guard(sendCs);
			sendQueue.push(sendData);
		}
	
	}

	// �ʱ� �α���
	// �������� �߰�
	void BusinessService::InitUser(const char *id, SOCKET sock, const char* nickName) {

		string msg;

		PER_HANDLE_DATA userInfo;
		// ������ ���� ���� �ʱ�ȭ
		userInfo.status = ClientStatus::STATUS_WAITING;

		strncpy(userInfo.userId, id, NAME_SIZE);
		strncpy(userInfo.roomName, "", NAME_SIZE);
		strncpy(userInfo.userName, nickName, NAME_SIZE);

		EnterCriticalSection(&userCs);
		// �������� insert
		this->userMap[sock] = userInfo;
		cout << "START user : " << nickName << endl;
		cout << "���� ���� �ο� �� : " << userMap.size() << endl;
		LeaveCriticalSection(&userCs);

		LogVo vo; // DB SQL���� �ʿ��� Data
		vo.setUserId(userInfo.userId);
		vo.setNickName(userInfo.userName);
		{
			lock_guard<mutex> guard(sqlCs);

			SQL_DATA sqlData1, sqlData2;
			sqlData1.vo = vo;
			sqlData1.direction = SqlWork::UPDATE_USER;
			sqlQueue.push(sqlData1); // �ֱ� �α��α�� ������Ʈ
			sqlData2.vo = vo;
			sqlData2.direction = SqlWork::INSERT_LOGIN;
			sqlQueue.push(sqlData2); // �α��� DB�� ���
		}

		msg = nickName;
		msg.append("�� ������ ȯ���մϴ�!\n");
		msg.append(waitRoomMessage);
		InsertSendQueue(SendTo::SEND_ME, msg, "", sock, ClientStatus::STATUS_WAITING);
	}

	// ���� �������� ����
	void BusinessService::ClientExit(SOCKET sock) {

		if (closesocket(sock) != SOCKET_ERROR) {
			cout << "���� ���� " << endl;

			// �ܼ� �������� ó��
			{
				lock_guard<mutex> guard(liveSocketCs);
				liveSocket.erase(sock);
			}

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
				if (userMap.find(sock)->second.status == ClientStatus::STATUS_CHATTIG) { // �濡 �������� ���
					string sendMsg;
					string roomName;
					sendMsg = name;
					sendMsg += " ���� �������ϴ�!";

					// ���̸� �ӽ� ����
					roomName = string(userMap.find(sock)->second.roomName);

					// ���� ����ÿ��� Room List ���� Lock��
					{
						lock_guard<recursive_mutex> guard(roomMap.find(roomName)->second->listCs);
						// �������� ��� BoardCast
						iocpService->SendToRoomMsg(sendMsg.c_str(), roomMap.find(roomName)->second->userList, ClientStatus::STATUS_CHATTIG);

						roomMap.find(roomName)->second->userList.remove(sock); // ������ ��� ���� out
						// Room List ���� Lock��
					}
					
					EnterCriticalSection(&userCs); // �α��ε� ��������� ���� Lock
					strncpy(userMap.find(sock)->second.roomName, "", NAME_SIZE); // ���̸� �ʱ�ȭ
					userMap.find(sock)->second.status = ClientStatus::STATUS_WAITING; // ���� ����
					LeaveCriticalSection(&userCs);

					// room Lock�� �� ���� ������ ����
					EnterCriticalSection(&roomCs);
					if (roomMap.find(roomName) != roomMap.end()) {
						if ((roomMap.find(roomName)->second)->userList.size() == 0) { // ���ο� 0���̸� �� ����
							roomMap.erase(roomName);
						}
					}
					LeaveCriticalSection(&roomCs);
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
	void BusinessService::StatusLogout(SOCKET sock, Direction direction, const char *message) {

		string msg = "";

		if (direction == Direction::USER_MAKE) { // 1�� ��������

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

				UserVo vo;
				vo.setUserId(sArr[0]);
				vo = move(dao->selectUser(vo));

				if (strcmp(vo.getUserId(), "") == 0) { // ID �ߺ�üũ => ���� ����

					vo.setUserId(sArr[0]);
					vo.setPassword(sArr[1]);
					vo.setNickName(sArr[2]);

					dao->InsertUser(vo);

					msg.append(sArr[0]);
					msg.append("���� ���� �Ϸ�!\n");
					msg.append(loginBeforeMessage);

					InsertSendQueue(SendTo::SEND_ME, msg, "", sock, ClientStatus::STATUS_LOGOUT);

				}
				else { // ID�ߺ�����

					msg.append("�ߺ��� ���̵� �ֽ��ϴ�!\n");
					msg.append(loginBeforeMessage);

					InsertSendQueue(SendTo::SEND_ME, msg, "", sock, ClientStatus::STATUS_LOGOUT);
				}
			}

		}
		else if (direction == Direction::USER_ENTER) { // 2�� �α��� �õ�

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

				UserVo vo;
				vo.setUserId(sArr[0]);
				vo = move(dao->selectUser(vo));

				if (strcmp(vo.getUserId(), "") == 0) { // ���� ����

					msg.append("���� �����Դϴ� ���̵� Ȯ���ϼ���!\n");
					msg.append(loginBeforeMessage);

					// SendQueue�� Insert
					InsertSendQueue(SendTo::SEND_ME, msg, "", sock, ClientStatus::STATUS_LOGOUT);

				}
				else if (strcmp(vo.getPassword(), sArr[1]) == 0) { // ��й�ȣ ��ġ
					EnterCriticalSection(&idCs);
					unordered_set<string>::const_iterator it = idSet.find(sArr[0]);
					if (it != idSet.end()) { // �ߺ��α��� ��ȿ�� �˻�
						LeaveCriticalSection(&idCs); // Case Lock ����
						msg.append("�ߺ� �α����� �ȵ˴ϴ�!\n");
						msg.append(loginBeforeMessage);

						// SendQueue�� Insert
						InsertSendQueue(SendTo::SEND_ME, msg, "", sock, ClientStatus::STATUS_LOGOUT);

					}
					else { // �ߺ��α��� X
						idSet.insert(sArr[0]);
						LeaveCriticalSection(&idCs); //Init�κ� �ι����� ����
						InitUser(sArr[0], sock, vo.getNickName()); // �������� ����
					}

				}
				else { // ��й�ȣ Ʋ��
					msg.append("��й�ȣ Ʋ��!\n");
					msg.append(loginBeforeMessage);

					InsertSendQueue(SendTo::SEND_ME, msg, "", sock, ClientStatus::STATUS_LOGOUT);
				}
			}

		}
		else { // �׿� ��ɾ� �Է�
			string sendMsg = errorMessage;
			sendMsg += loginBeforeMessage;
			InsertSendQueue(SendTo::SEND_ME, sendMsg, "", sock, ClientStatus::STATUS_LOGOUT);
		}

	}

	// ���ǿ����� ���� ó��
	// ���ǰ� ����
	void BusinessService::StatusWait(SOCKET sock, ClientStatus status, Direction direction, const char *message) {

		string name = userMap.find(sock)->second.userName;
		string id = userMap.find(sock)->second.userId;
		string msg = string(message);
		// ���ǿ��� �̸� ���� ����
		if (direction == Direction::ROOM_MAKE) { // ���ο� �� ���鶧

			// ��ȿ�� ���� ����
			EnterCriticalSection(&roomCs);
			int cnt = roomMap.count(msg);

			if (cnt != 0) { // ���̸� �ߺ�
				LeaveCriticalSection(&roomCs);
				msg += "�̹� �ִ� �� �̸��Դϴ�!\n";
				msg += waitRoomMessage;
				InsertSendQueue(SendTo::SEND_ME, msg, "", sock, ClientStatus::STATUS_WAITING);
				// �ߺ� ���̽��� �� ���� �� ����
			}
			else { // ���̸� �ߺ� �ƴ� ���� ����
				// ���ο� �� ���� ����
				shared_ptr<ROOM_DATA> roomData = make_shared<ROOM_DATA>();
				list<SOCKET> chatList;
				chatList.push_back(sock);
				// �� ����Ʈ�� CS��ü Init
				roomData->userList = chatList;
				roomMap[msg] = roomData;

				LeaveCriticalSection(&roomCs);

				// User�� ���� ���� �ٲ۴�
				EnterCriticalSection(&userCs);
				strncpy((userMap.find(sock))->second.roomName, msg.c_str(),
					NAME_SIZE);
				(userMap.find(sock))->second.status =
					ClientStatus::STATUS_CHATTIG;
				LeaveCriticalSection(&userCs);

				msg += " ���� �����Ǿ����ϴ�.";
				msg += chatRoomMessage;
				// cout << "���� ������ �� ���� : " << roomMap.size() << endl;
				InsertSendQueue(SendTo::SEND_ME, msg, "", sock, ClientStatus::STATUS_CHATTIG);

			}

		}
		else if (direction == Direction::ROOM_ENTER) { // �� ���� ��û

			// ��ȿ�� ���� ����
			EnterCriticalSection(&roomCs);

			if (roomMap.find(msg) == roomMap.end()) { // �� ��ã��
				LeaveCriticalSection(&roomCs);
				msg = "���� �� �Դϴ�!\n";
				msg += waitRoomMessage;

				InsertSendQueue(SendTo::SEND_ME, msg, "", sock, ClientStatus::STATUS_WAITING);

			}
			else {

				auto iter = roomMap.find(msg);
				shared_ptr<ROOM_DATA> second = nullptr;
				if (iter != roomMap.end()) {
					second = iter->second;
				}
				LeaveCriticalSection(&roomCs); // ������ ����� ��ü �� Lock ����

				if (second != nullptr) { // ���� ������� �� ���� ������Ʈ + list insert
					EnterCriticalSection(&userCs);
					strncpy(userMap.find(sock)->second.roomName, msg.c_str(),
						NAME_SIZE); // �α��� ���� ���� ����
					userMap.find(sock)->second.status = ClientStatus::STATUS_CHATTIG;
					LeaveCriticalSection(&userCs);
					{
						lock_guard<recursive_mutex> guard(roomMap.find(msg)->second->listCs);
						//���� �����ϱ� ������ insert
						roomMap.find(msg)->second->userList.push_back(sock);
					}
					 // ���� Lock

					string sendMsg = name;
					sendMsg += " ���� �����ϼ̽��ϴ�. ";
					sendMsg += chatRoomMessage;

					InsertSendQueue(SendTo::SEND_ROOM, sendMsg, msg, 0, ClientStatus::STATUS_CHATTIG);
				}
			}
		}
		else if (direction == Direction::ROOM_INFO) { // �� ���� ��û��

			string str;
			if (roomMap.size() == 0) {
				str = "������� ���� �����ϴ�";
			}
			else {
				str += "�� ���� ����Ʈ";
				EnterCriticalSection(&roomCs);
				map<string, shared_ptr<ROOM_DATA>> roomCopyMap = roomMap;
				LeaveCriticalSection(&roomCs);
				// roomMap ��� ��� ���� �ʵ��� ��������
				map<string, shared_ptr<ROOM_DATA>>::const_iterator iter;

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

			InsertSendQueue(SendTo::SEND_ME, str, "", sock, ClientStatus::STATUS_WAITING);

		}
		else if (direction == Direction::ROOM_USER_INFO) { // ���� ���� ��û��
			string str = "���� ���� ����Ʈ";

			EnterCriticalSection(&userCs);
			unordered_map<SOCKET, PER_HANDLE_DATA> userCopyMap = userMap;
			LeaveCriticalSection(&userCs);
			// userMap ��� ��� ���� �ʵ��� ��������
			unordered_map<SOCKET, PER_HANDLE_DATA>::const_iterator iter;
			for (iter = userCopyMap.begin(); iter != userCopyMap.end(); iter++) {
				if (str.length() >= 4000) {
					break;
				}
				str += "\n";
				str += (iter->second).userName;
				str += ":";
				if ((iter->second).status == ClientStatus::STATUS_WAITING) {
					str += "����";
				}
				else {
					str += (iter->second).roomName;
				}
			}
			InsertSendQueue(SendTo::SEND_ME, str, "", sock, ClientStatus::STATUS_WAITING);

		}
		else if (direction == Direction::FRIEND_INFO) { // ģ�� ���� ��û

			RelationVo vo;
			vo.setUserId(id.c_str());
			vector<RelationVo> vec = move(dao->selectFriends(vo));

			string sendMsg = "ģ�� ���� ����Ʈ";
			if (vec.size() == 0) {
				sendMsg.append("\n��ϵ� ģ���� �����ϴ�");
				InsertSendQueue(SendTo::SEND_ME, sendMsg, "", sock, ClientStatus::STATUS_WAITING);
			}
			else {
				EnterCriticalSection(&userCs);
				unordered_map<SOCKET, PER_HANDLE_DATA> userCopyMap = userMap; // �α��ε� ģ������ Ȯ������ ����
				LeaveCriticalSection(&userCs);

				// Select�ؼ� ������ ������ ģ������
				for (int i = 0; i < vec.size(); i++) {

					unordered_map<SOCKET, PER_HANDLE_DATA>::const_iterator iter;
					bool exist = false;
					// userMap�� Ž���Ͽ� ģ�� ��ġ ����
					for (iter = userCopyMap.begin(); iter != userCopyMap.end(); iter++) {
						if (strcmp(vec[i].getNickName(), iter->second.userName) == 0) {
							sendMsg += "\n";
							sendMsg += vec[i].getNickName();
							sendMsg += ":";
							if (strcmp(iter->second.roomName, "") == 0) {
								sendMsg += "����";
							}
							else {
								sendMsg += iter->second.roomName;
							}
							exist = true;
							break;
						}
					}
					// userMap�� ������ ���� ���� ����
					if (!exist) {
						sendMsg += "\n";
						sendMsg += vec[i].getNickName();
						sendMsg += ":";
						sendMsg += "���Ӿ���";
					}

				}
				InsertSendQueue(SendTo::SEND_ME, sendMsg, "", sock, ClientStatus::STATUS_WAITING);
			}

		}
		else if (direction == Direction::FRIEND_ADD) { // ģ�� �߰�
			AddFriend(sock, msg, id, ClientStatus::STATUS_WAITING);
		}
		else if (direction == Direction::FRIEND_GO) { // ģ���� �ִ� ������
			RelationVo vo;
			vo.setUserId(id.c_str());
			vo.setRelationto(msg.c_str());
			// ��û ģ�� ���� select
			RelationVo vo2 = move(dao->selectOneFriend(vo));

			if (strcmp(vo2.getNickName(), "") == 0) { // ģ������ ��ã��
				string sendMsg = "ģ�������� ã�� �� �����ϴ�\n";
				sendMsg += waitRoomMessage;
				InsertSendQueue(SendTo::SEND_ME, sendMsg, "", sock, ClientStatus::STATUS_WAITING);
			}
			else {
				EnterCriticalSection(&userCs);
				unordered_map<SOCKET, PER_HANDLE_DATA> userCopyMap = userMap; // �α��ε� ģ������ Ȯ������ ����
				LeaveCriticalSection(&userCs);

				unordered_map<SOCKET, PER_HANDLE_DATA>::const_iterator iter;

				bool exist = false;

				for (iter = userCopyMap.begin(); iter != userCopyMap.end(); iter++) {
					if (strcmp(vo2.getNickName(), iter->second.userName) == 0) { // ��ġ �г��� ã��
						if (strcmp(iter->second.roomName, "") == 0) { // ���ǿ� ����
							string sendMsg = string(message);
							sendMsg += " ���� ���ǿ� �ֽ��ϴ�";

							InsertSendQueue(SendTo::SEND_ME, sendMsg, "", sock, ClientStatus::STATUS_WAITING);
							exist = true;
							break;
						}
						else { // �� ���� ���μ���
							EnterCriticalSection(&roomCs);
							string roomName = iter->second.roomName;
							auto iter = roomMap.find(roomName);
							shared_ptr<ROOM_DATA> second = nullptr;
							if (iter != roomMap.end()) {
								second = iter->second;
							}
							LeaveCriticalSection(&roomCs); // ������ ����� ��ü �� Lock ����

							if (second != nullptr) { // ���� ������� �� ���� ������Ʈ + list insert
								EnterCriticalSection(&userCs);
								strncpy(userMap.find(sock)->second.roomName, roomName.c_str(),
									NAME_SIZE); // �α��� ���� ���� ����
								userMap.find(sock)->second.status = ClientStatus::STATUS_CHATTIG;
								string nick = userMap.find(sock)->second.userName;
								LeaveCriticalSection(&userCs);
								{
									lock_guard<recursive_mutex> guard(roomMap.find(msg)->second->listCs);
									roomMap.find(roomName)->second->userList.push_back(sock);
								}// ���� Lock
								
								string sendMsg = "";
								sendMsg.append(nick);
								sendMsg += " ���� �����ϼ̽��ϴ�. ";
								sendMsg += chatRoomMessage;

								InsertSendQueue(SendTo::SEND_ROOM, sendMsg, roomName, 0, ClientStatus::STATUS_CHATTIG);
							}

							exist = true;
							break;
						}
					}
				}

				if (!exist) {
					string sendMsg = string(message);
					sendMsg += " ���� �������� �ƴմϴ�";

					InsertSendQueue(SendTo::SEND_ME, sendMsg, "", sock, ClientStatus::STATUS_WAITING);
				}
			}
		}
		else if (direction == Direction::FRIEND_DELETE) { // ģ�� ����

			RelationVo vo;
			vo.setUserId(id.c_str());
			vo.setRelationcode(1);
			vo.setNickName(msg.c_str());

			int res = dao->DeleteRelation(vo);
			if (res != -1) { // ���� ����
				string sendMsg = msg;
				sendMsg += " ���� ģ�� �����߽��ϴ�";

				InsertSendQueue(SendTo::SEND_ME, sendMsg, "", sock, ClientStatus::STATUS_WAITING);
			}
			else { // ��������
				string sendMsg = "ģ�� ��������\n";
				sendMsg.append(waitRoomMessage);

				InsertSendQueue(SendTo::SEND_ME, sendMsg, "", sock, ClientStatus::STATUS_WAITING);
			}
		}
		else if (direction == Direction::WHISPER) { // �ӼӸ�

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

					InsertSendQueue(SendTo::SEND_ME, sendMsg, "", sock, ClientStatus::STATUS_WAITING);

				}
				else {
					bool find = false;
					EnterCriticalSection(&userCs);
					unordered_map<SOCKET, PER_HANDLE_DATA> userCopyMap = userMap; // �α��ε� ģ������ Ȯ������ ����
					LeaveCriticalSection(&userCs);

					unordered_map<SOCKET, PER_HANDLE_DATA>::const_iterator iter;

					for (iter = userCopyMap.begin(); iter != userCopyMap.end(); iter++) {
						if (name.compare(iter->second.userName) == 0) {
							find = true;
							sendMsg = userCopyMap.find(sock)->second.userName;
							sendMsg += " �Կ��� �� �ӼӸ� : ";
							sendMsg += msg;

							InsertSendQueue(SendTo::SEND_ME, sendMsg, "", iter->first, ClientStatus::STATUS_WHISPER);

							break;
						}
					}

					// �ӼӸ� ����� ��ã��
					if (!find) {
						sendMsg = name;
						sendMsg += " ���� ã�� �� �����ϴ�";

						InsertSendQueue(SendTo::SEND_ME, sendMsg, "", sock, ClientStatus::STATUS_WAITING);
					}
				}
			}

		}
		else if (direction == Direction::LOG_OUT) { // �α׾ƿ�
			char id[NAME_SIZE];
			EnterCriticalSection(&idCs);
			strncpy(id, userMap.find(sock)->second.userId, NAME_SIZE);
			idSet.erase(userMap.find(sock)->second.userId); // �α��� �¿��� ����
			LeaveCriticalSection(&idCs);

			EnterCriticalSection(&userCs);
			userMap.erase(sock); // ���� ���� ���� ����
			cout << "���� ���� �ο� �� : " << userMap.size() << endl;
			LeaveCriticalSection(&userCs);

			string sendMsg = loginBeforeMessage;
			InsertSendQueue(SendTo::SEND_ME, sendMsg, "", sock, ClientStatus::STATUS_LOGOUT);
		}
		else { // �׿� ��ɾ� �Է�
			string sendMsg = errorMessage;
			sendMsg += waitRoomMessage;
			// ������ �����̹Ƿ� ClientStatus::STATUS_WAITING ���·� �����Ѵ�

			InsertSendQueue(SendTo::SEND_ME, sendMsg, "", sock, ClientStatus::STATUS_WAITING);
		}

		LogVo vo; // DB SQL���� �ʿ��� Data
		vo.setNickName(name.c_str());
		vo.setMsg(message);
		vo.setDirection(direction);
		vo.setStatus(status);

		{
			lock_guard<mutex> guard(sqlCs);
			SQL_DATA sqlData;
			sqlData.vo = vo;
			sqlData.direction = SqlWork::INSERT_DIRECTION;
			sqlQueue.push(sqlData); // ���� ��� insert
		}

	}

	// ä�ù濡���� ���� ó��
	// ���ǰ� ����
	void BusinessService::StatusChat(SOCKET sock, ClientStatus status, Direction direction, const char *message) {

		string name;
		string msg;
		string id;
		// ���ǿ��� �̸� ���� ����
		name = userMap.find(sock)->second.userName;
		id = userMap.find(sock)->second.userId;
		msg = string(message);

		if (msg.compare("\\out") == 0) { // ä�ù� ����

			string sendMsg;
			string roomName;
			sendMsg = name;
			sendMsg += " ���� �������ϴ�!";

			// ���̸� �ӽ� ����
			roomName = string(userMap.find(sock)->second.roomName);

			// ���� ����ÿ��� Room List ���� Lock��
			{
				lock_guard<recursive_mutex> guard(roomMap.find(roomName)->second->listCs);
				// �������� ��� BoardCast
				iocpService->SendToRoomMsg(sendMsg.c_str(), roomMap.find(roomName)->second->userList, ClientStatus::STATUS_CHATTIG);

				roomMap.find(roomName)->second->userList.remove(sock); // ������ ��� ���� out
			}
			// Room List ���� Lock��

			msg = waitRoomMessage;
			InsertSendQueue(SendTo::SEND_ME, msg, "", sock, ClientStatus::STATUS_WAITING);

			EnterCriticalSection(&userCs); // �α��ε� ��������� ���� Lock
			strncpy(userMap.find(sock)->second.roomName, "", NAME_SIZE); // ���̸� �ʱ�ȭ
			userMap.find(sock)->second.status = ClientStatus::STATUS_WAITING; // ���� ����
			LeaveCriticalSection(&userCs);

			// room Lock�� �� ���� ������ ����
			EnterCriticalSection(&roomCs);
			if (roomMap.find(roomName) != roomMap.end()) {
				if ((roomMap.find(roomName)->second)->userList.size() == 0) { // ���ο� 0���̸� �� ����
					roomMap.erase(roomName);
				}
			}
			LeaveCriticalSection(&roomCs);
		}
		else if (msg.find("\\add") != -1) { // ģ���߰�

			msg.erase(0, 5); // ģ�� �̸� ��ȯ
			// ģ���߰�
			AddFriend(sock, msg, id, ClientStatus::STATUS_CHATTIG);
		}
		else { // ä�ù濡�� ä����

			string sendMsg;
			sendMsg = name;
			sendMsg += ":";
			sendMsg += msg;

			char roomName[NAME_SIZE];

			EnterCriticalSection(&userCs);
			map<string, shared_ptr<ROOM_DATA>>::iterator it = roomMap.find(
				userMap.find(sock)->second.roomName);
			strncpy(roomName, userMap.find(sock)->second.roomName, NAME_SIZE);
			LeaveCriticalSection(&userCs);

			if (it != roomMap.end()) { // null �˻�
				InsertSendQueue(SendTo::SEND_ROOM, sendMsg, userMap.find(sock)->second.roomName, 0, ClientStatus::STATUS_CHATTIG);
			}

			LogVo vo; // DB SQL���� �ʿ��� Data
			vo.setNickName(name.c_str());
			vo.setRoomName(roomName);
			vo.setMsg(msg.c_str());
			vo.setDirection(0);
			vo.setStatus(0);

			{
				lock_guard<mutex> guard(sqlCs);

				SQL_DATA sqlData;
				sqlData.vo = vo;
				sqlData.direction = SqlWork::INSERT_CHATTING;
				sqlQueue.push(sqlData); // ��ȭ ��� ������Ʈ
			}
		}
	}

	// ä�ù濡���� ���� ����� ���̽�
	void BusinessService::StatusFile(SOCKET sock) {

		string fileDir = fileService->RecvFile(sock);
		// SendQueue�� Insert
		InsertSendQueue(SendTo::SEND_ROOM, "", userMap.find(sock)->second.roomName, 0, ClientStatus::STATUS_FILE_SEND);
		// ���⼭���� UDP BroadCast
		InsertSendQueue(SendTo::SEND_FILE, fileDir.c_str(), userMap.find(sock)->second.roomName, 0, ClientStatus::STATUS_FILE_SEND);

	}

	// Ŭ���̾�Ʈ���� ���� ������ ������ ����ü ����
	// ��� �޸� ���� ��ȯ
	string BusinessService::DataCopy(LPPER_IO_DATA ioInfo, ClientStatus *status, Direction *direction) {

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

		if (iocpService->RECV == ioInfo->serverMode) {
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
			ioInfo->serverMode = iocpService->RECV;
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
		}
		else {
			return false;
		}
	}

	// Ŭ���̾�Ʈ�� �������� ��ȯ
	ClientStatus BusinessService::GetStatus(SOCKET sock) {
		EnterCriticalSection(&this->userCs);
		ClientStatus status = userMap.find(sock)->second.status;
		LeaveCriticalSection(&this->userCs);
		return status;
	}

	// ģ���߰� ���
	void BusinessService::AddFriend(SOCKET sock, const string& msg, const string& id, ClientStatus status) {
		RelationVo vo;
		vo.setNickName(msg.c_str());
		RelationVo vo2 = move(dao->findUserId(vo)); // ���̵� ���� ���� �˻�

		string sendMsg;

		if (strcmp(vo2.getRelationto(), "") == 0) {
			sendMsg = "���̵� ���� ģ�� �Ұ���";
			InsertSendQueue(SendTo::SEND_ME, sendMsg, "", sock, status);
		}
		else {

			vo2.setUserId(id.c_str());
			vo2.setRelationcode(1);
			int res = dao->InsertRelation(vo2);
			if (res != -1) { // ģ�� ����
				sendMsg = msg;
				sendMsg.append("���� ģ���߰� �Ǿ����ϴ�");
				InsertSendQueue(SendTo::SEND_ME, sendMsg, "", sock, status);
			}
			else {
				sendMsg = msg;
				sendMsg.append("���� ģ���߰� ����");
				InsertSendQueue(SendTo::SEND_ME, sendMsg, "", sock, status);
			}
		}
	}

	void BusinessService::InsertLiveSocket(const SOCKET& hClientSock, const SOCKADDR_IN& addr) {
		{
			lock_guard<mutex> guard(liveSocketCs);
			liveSocket.insert(pair<SOCKET, string>(hClientSock, inet_ntoa(addr.sin_addr)));
		}
	}

	bool BusinessService::IsSocketDead(SOCKET socket){
		return liveSocket.find(socket) == liveSocket.end();
	}

	void BusinessService::BanUser(SOCKET socket, const char* nickName) {

		EnterCriticalSection(&userCs);
		unordered_map<SOCKET, PER_HANDLE_DATA> userCopyMap = userMap; // �α��ε� ģ������ Ȯ������ ����
		LeaveCriticalSection(&userCs);

		unordered_map<SOCKET, PER_HANDLE_DATA>::const_iterator iter;

		for (iter = userCopyMap.begin(); iter != userCopyMap.end(); iter++) {
			if (strcmp(nickName, iter->second.userName) == 0) {
				SOCKET sock = iter->first;

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
				if (userMap.find(sock)->second.status == ClientStatus::STATUS_CHATTIG) { // �濡 �������� ���
					string sendMsg;
					string roomName;
					sendMsg = name;
					sendMsg += " ���� �������ϴ�!";

					// ���̸� �ӽ� ����
					roomName = string(userMap.find(sock)->second.roomName);

					// ���� ����ÿ��� Room List ���� Lock��
					{
						lock_guard<recursive_mutex> guard(roomMap.find(roomName)->second->listCs);
						// �������� ��� BoardCast
						iocpService->SendToRoomMsg(sendMsg.c_str(), roomMap.find(roomName)->second->userList, ClientStatus::STATUS_CHATTIG);

						roomMap.find(roomName)->second->userList.remove(sock); // ������ ��� ���� out

					}				
					// Room List ���� Lock��


					EnterCriticalSection(&userCs); // �α��ε� ��������� ���� Lock
					strncpy(userMap.find(sock)->second.roomName, "", NAME_SIZE); // ���̸� �ʱ�ȭ
					userMap.find(sock)->second.status = ClientStatus::STATUS_WAITING; // ���� ����
					LeaveCriticalSection(&userCs);

					// room Lock�� �� ���� ������ ����
					EnterCriticalSection(&roomCs);
					if (roomMap.find(roomName) != roomMap.end()) {
						if ((roomMap.find(roomName)->second)->userList.size() == 0) { // ���ο� 0���̸� �� ����
							roomMap.erase(roomName);
						}
					}
					LeaveCriticalSection(&roomCs);
				}
				string str = "����� �����ڿ����� ����Ǿ����ϴ�";
				InsertSendQueue(SendTo::SEND_ME, str.c_str(), "", sock, ClientStatus::STATUS_LOGOUT);

				EnterCriticalSection(&userCs);
				userMap.erase(sock); // ���� ���� ���� ����
				cout << "���� ���� �ο� �� : " << userMap.size() << endl;
				LeaveCriticalSection(&userCs);

				break;
			}
		}
	}

	void BusinessService::CallCnt(SOCKET socket, const DWORD& cnt) {
		char msg[30];
		sprintf(msg, "{\"packet\":%d , \"cnt\":%d}", cnt, userMap.size());
		InsertSendQueue(SendTo::SEND_ME, msg, "", socket, ClientStatus::STATUS_INIT);
	}

	IocpService::IocpService* BusinessService::getIocpService() {
		return this->iocpService;
	}

} /* namespace Service */