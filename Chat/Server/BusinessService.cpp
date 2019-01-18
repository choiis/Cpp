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
	InitializeCriticalSection(&idCs);

	InitializeCriticalSection(&userCs);

	InitializeCriticalSection(&roomCs);

	iocpService = new IocpService::IocpService();
}

BusinessService::~BusinessService() {

	delete iocpService;
	// �Ӱ迵�� Object ��ȯ
	DeleteCriticalSection(&idCs);

	DeleteCriticalSection(&userCs);

	DeleteCriticalSection(&roomCs);
}

// �ʱ� �α���
// �������� �߰�
void BusinessService::InitUser(const char *id, SOCKET sock) {

	char msg[BUF_SIZE];
	char name[NAME_SIZE];

	strncpy(name, idMap.find(id)->second.nickname, NAME_SIZE);

	LPPER_HANDLE_DATA handleInfo = new PER_HANDLE_DATA;
	cout << "START user : " << name << endl;
	cout << "sock : " << sock << endl;
	// ������ ���� ���� �ʱ�ȭ
	// ioInfo = new PER_IO_DATA;
	handleInfo->status = STATUS_WAITING;

	strncpy(handleInfo->userId, id, NAME_SIZE);
	strncpy(handleInfo->userName, name, NAME_SIZE);
	strncpy(handleInfo->roomName, "", NAME_SIZE);
	EnterCriticalSection(&userCs);
	// �������� insert
	this->userMap.insert(pair<SOCKET, LPPER_HANDLE_DATA>(sock, handleInfo));
	cout << "���� ���� �ο� �� : " << userMap.size() << endl;
	LeaveCriticalSection(&userCs);

	// id�� key���� ���� �̸��� �ƴ϶� ���̵�!!
	idMap.find(id)->second.logMode = NOW_LOGIN;

	strncpy(msg, "������ ȯ���մϴ�!\n", BUF_SIZE);
	strcat(msg, waitRoomMessage);

	iocpService->SendToOneMsg(msg, sock, STATUS_WAITING);
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

			strncpy(roomName, userMap.find(sock)->second->roomName,
			NAME_SIZE);
			strncpy(name, userMap.find(sock)->second->userName, NAME_SIZE);
			strncpy(id, userMap.find(sock)->second->userId, NAME_SIZE);
			// �α��� ���� ���󺹱�
			idMap.find(id)->second.logMode = NOW_LOGOUT;

			// ���̸� �ӽ� ����
			if (strcmp(roomName, "") != 0) { // �濡 �������� ���
				// ������ ��� ���� out

				if (roomMap.find(roomName) != roomMap.end()) { // null üũ �켱
					(roomMap.find(roomName)->second).remove(sock);

					if ((roomMap.find(roomName)->second).size() == 0) { // ���ο� 0���̸� �� ����
						EnterCriticalSection(&roomCs);
						roomMap.erase(roomName);
						LeaveCriticalSection(&roomCs);
					} else {
						char sendMsg[BUF_SIZE];
						// �̸����� ���� NAME_SIZE����
						strncpy(sendMsg, name, NAME_SIZE);
						strcat(sendMsg, " ���� �������ϴ�!");

						if (roomMap.find(userMap.find(sock)->second->roomName)
								!= roomMap.end()) { // null �˻�

							iocpService->SendToRoomMsg(sendMsg,
									roomMap.find(
											userMap.find(sock)->second->roomName)->second,
									STATUS_CHATTIG);
						}
					}
				}
			}
			EnterCriticalSection(&userCs);
			userMap.erase(sock); // ���� ���� ���� ����
			LeaveCriticalSection(&userCs);
			cout << "���� ���� �ο� �� : " << userMap.size() << endl;
		}

	}
}

// �α��� ���� ����ó��
// ���ǰ� ���� �� ����
void BusinessService::StatusLogout(SOCKET sock, int status, int direction,
		char *message) {

	if (status == STATUS_LOGOUT) {

		// memcpy �κ� ����
		string msg = "";

		if (direction == USER_MAKE) { // 1�� ��������
			cout << "USER MAKE" << endl;
			USER_DATA userData;
			char *sArr[3] = { NULL, };
			char *ptr = strtok(message, "\\"); // ���� ���ڿ��� �������� ���ڿ��� �ڸ�
			int i = 0;
			while (ptr != NULL)            // �ڸ� ���ڿ��� ������ ���� ������ �ݺ�
			{
				sArr[i] = ptr;           // ���ڿ��� �ڸ� �� �޸� �ּҸ� ���ڿ� ������ �迭�� ����
				i++;                       // �ε��� ����
				ptr = strtok(NULL, "\\");   // ���� ���ڿ��� �߶� �����͸� ��ȯ
			}
			// ��ȿ�� ���� �ʿ�
			EnterCriticalSection(&idCs); // ���������� ���� ID �Է� ����
			if (idMap.find(sArr[0]) == idMap.end()) { // ID �ߺ�üũ

				userData.password = string(sArr[1]);
				strncpy(userData.nickname, sArr[2], NAME_SIZE);
				userData.logMode = NOW_LOGOUT;

				idMap[string(sArr[0])] = userData; // Insert

				LeaveCriticalSection(&idCs); // Insert �� Lock ����
				msg.append(sArr[0]);
				msg.append(" ���� ���� �Ϸ�!\n");
				msg.append(loginBeforeMessage);

				iocpService->SendToOneMsg(msg.c_str(), sock, STATUS_LOGOUT);
			} else { // ID�ߺ�����
				LeaveCriticalSection(&idCs); // Case Lock ����
				msg.append("�ߺ��� ���̵� �ֽ��ϴ�!\n");
				msg.append(loginBeforeMessage);

				iocpService->SendToOneMsg(msg.c_str(), sock, STATUS_LOGOUT);
			}

		} else if (direction == USER_ENTER) { // 2�� �α��� �õ�
			cout << "USER ENTER" << endl;
			char *sArr[2] = { NULL, };
			char *ptr = strtok(message, "\\"); // ���� ���ڿ��� �������� ���ڿ��� �ڸ�
			int i = 0;
			while (ptr != NULL)            // �ڸ� ���ڿ��� ������ ���� ������ �ݺ�
			{
				sArr[i] = ptr;           // ���ڿ��� �ڸ� �� �޸� �ּҸ� ���ڿ� ������ �迭�� ����
				i++;                       // �ε��� ����
				ptr = strtok(NULL, "\\");   // ���� ���ڿ��� �߶� �����͸� ��ȯ
			}

			if (idMap.find(sArr[0]) == idMap.end()) { // ���� ����
				msg.append("���� �����Դϴ� ���̵� Ȯ���ϼ���!\n");
				msg.append(loginBeforeMessage);

				iocpService->SendToOneMsg(msg.c_str(), sock, STATUS_LOGOUT);
			} else if (idMap.find(sArr[0])->second.password.compare(
					string(sArr[1])) == 0) { // ��й�ȣ ��ġ

				if (idMap.find(sArr[0])->second.logMode == NOW_LOGIN) { // �ߺ��α��� ��ȿ�� �˻�
					msg.append("�ߺ� �α����� �ȵ˴ϴ�!\n");
					msg.append(loginBeforeMessage);

					iocpService->SendToOneMsg(msg.c_str(), sock, STATUS_LOGOUT);
				} else { // �ߺ��α��� X
					InitUser(sArr[0], sock); // �������� ����
				}

			} else { // ��й�ȣ Ʋ��
				msg.append("��й�ȣ Ʋ��!\n");
				msg.append(loginBeforeMessage);

				iocpService->SendToOneMsg(msg.c_str(), sock, STATUS_LOGOUT);
			}

		} else if (direction == USER_LIST) { // ������ ���� ��û
			cout << "USER LIST" << endl;
			char userList[BUF_SIZE];
			strncpy(userList, "���̵� ���� ����Ʈ", BUF_SIZE);
			if (idMap.size() == 0) {
				strcat(userList, " ����");
			} else {
				// ���� �ƴϹǷ� Lock ����
				unordered_map<string, USER_DATA>::const_iterator iter;
				for (iter = idMap.begin(); iter != idMap.end(); ++iter) {
					strcat(userList, "\n");
					strcat(userList, (iter->first).c_str());
				}
			}

			iocpService->SendToOneMsg(userList, sock, STATUS_LOGOUT);
		} else { // �׿� ��ɾ� �Է�
			char sendMsg[BUF_SIZE];

			strncpy(sendMsg, errorMessage, BUF_SIZE);
			strcat(sendMsg, loginBeforeMessage);
			// ������ �����̹Ƿ� STATUS_WAITING ���·� �����Ѵ�

			iocpService->SendToOneMsg(sendMsg, sock, STATUS_LOGOUT);
		}
	}

}

// ���ǿ����� ���� ó��
// ���ǰ� ����
void BusinessService::StatusWait(SOCKET sock, int status, int direction,
		char *message) {

	string name = userMap.find(sock)->second->userName;
	string msg = string(message);
	// ���ǿ��� �̸� ���� ����

	if (direction == ROOM_MAKE) { // ���ο� �� ���鶧
		cout << "ROOM MAKE" << endl;
		// ��ȿ�� ���� ����
		if (roomMap.count(msg) != 0) { // ���̸� �ߺ�
			msg += "�̹� �ִ� �� �̸��Դϴ�!\n";
			msg += waitRoomMessage;

			iocpService->SendToOneMsg(msg.c_str(), sock, STATUS_WAITING);
			// �ߺ� ���̽��� �� ���� �� ����
		} else { // ���̸� �ߺ� �ƴ� ���� ����

			// ���ο� �� ���� ����
			list<SOCKET> chatList;
			chatList.push_back(sock);
			EnterCriticalSection(&roomCs);
			roomMap.insert(pair<string, list<SOCKET> >(msg, chatList));
			LeaveCriticalSection(&roomCs);

			// User�� ���� ���� �ٲ۴�
			strncpy((userMap.find(sock))->second->roomName, msg.c_str(),
			NAME_SIZE);
			(userMap.find(sock))->second->status =
			STATUS_CHATTIG;

			msg += " ���� �����Ǿ����ϴ�.";
			msg += chatRoomMessage;
			cout << "���� ������ �� ���� : " << roomMap.size() << endl;

			iocpService->SendToOneMsg(msg.c_str(), sock, STATUS_CHATTIG);
		}

	} else if (direction == ROOM_ENTER) { // �� ���� ��û
		cout << "ROOM ENTER" << endl;

		// ��ȿ�� ���� ����
		EnterCriticalSection(&roomCs);
		if (roomMap.count(msg) == 0) { // �� ��ã��
			LeaveCriticalSection(&roomCs);
			msg = "���� �� �Դϴ�!\n";
			msg += waitRoomMessage;
			;
			iocpService->SendToOneMsg(msg.c_str(), sock, STATUS_WAITING);
		} else {

			if (roomMap.find(msg) != roomMap.end()) {
				roomMap.find(msg)->second.push_back(sock);
			}
			LeaveCriticalSection(&roomCs); // �� ������ �� ���� ���� ���̽� ����
			strncpy(userMap.find(sock)->second->roomName, msg.c_str(),
			NAME_SIZE);
			userMap.find(sock)->second->status = STATUS_CHATTIG;

			string sendMsg = name;
			sendMsg += " ���� �����ϼ̽��ϴ�. ";
			sendMsg += chatRoomMessage;

			if (roomMap.find(msg) != roomMap.end()) { // ������ �˻�

				iocpService->SendToRoomMsg(sendMsg.c_str(),
						roomMap.find(msg)->second,
						STATUS_CHATTIG);

			}
		}

	} else if (direction == ROOM_INFO) { // �� ���� ��û��

		string str;
		if (roomMap.size() == 0) {
			str = "������� ���� �����ϴ�";
		} else {
			str += "�� ���� ����Ʈ";

			unordered_map<string, list<SOCKET> >::const_iterator iter;
			// �������� ���ڿ��� �����
			EnterCriticalSection(&roomCs);
			for (iter = roomMap.begin(); iter != roomMap.end(); iter++) {

				str += "\n";
				str += iter->first.c_str();
				str += " : ";
				str += to_string((iter->second).size());
				str += "��";
			}
			LeaveCriticalSection(&roomCs);
		}

		iocpService->SendToOneMsg(str.c_str(), sock, STATUS_WAITING);
	} else if (direction == ROOM_USER_INFO) { // ���� ���� ��û��
		string str = "���� ���� ����Ʈ";

		unordered_map<SOCKET, LPPER_HANDLE_DATA>::iterator iter;

		// ���� ������ ���ڿ��� �����
		EnterCriticalSection(&userCs);
		for (iter = userMap.begin(); iter != userMap.end(); iter++) {
			str += "\n";
			str += (iter->second)->userName;
			str += " : ";
			if ((iter->second)->status == STATUS_WAITING) {
				str += "���� ";
			} else {
				str += (iter->second)->roomName;
			}
		}
		LeaveCriticalSection(&userCs);

		iocpService->SendToOneMsg(str.c_str(), sock, STATUS_WAITING);
	} else if (direction == WHISPER) { // �ӼӸ�
		cout << "WHISPER " << endl;
		char *sArr[2] = { NULL, };
		char *ptr = strtok(message, "\\"); // ���� ���ڿ��� �������� ���ڿ��� �ڸ�
		int i = 0;
		while (ptr != NULL)            // �ڸ� ���ڿ��� ������ ���� ������ �ݺ�
		{
			sArr[i] = ptr;           // ���ڿ��� �ڸ� �� �޸� �ּҸ� ���ڿ� ������ �迭�� ����
			i++;                       // �ε��� ����
			ptr = strtok(NULL, "\\");   // ���� ���ڿ��� �߶� �����͸� ��ȯ
		}
		name = string(sArr[0]);
		msg = string(sArr[1]);

		unordered_map<SOCKET, LPPER_HANDLE_DATA>::iterator iter;
		string sendMsg;

		if (name.compare(userMap.find(sock)->second->userName) == 0) { // ���ο��� ����
			sendMsg = "�ڽſ��� ������ ������ �����ϴ�\n";
			sendMsg += waitRoomMessage;

			iocpService->SendToOneMsg(sendMsg.c_str(), sock, STATUS_WAITING);
		} else {
			bool find = false;
			EnterCriticalSection(&userCs);
			for (iter = userMap.begin(); iter != userMap.end(); iter++) {
				if (name.compare(iter->second->userName) == 0) {
					find = true;
					sendMsg = userMap.find(sock)->second->userName;
					sendMsg += " �Կ��� �� �ӼӸ� : ";
					sendMsg += msg;

					iocpService->SendToOneMsg(sendMsg.c_str(), iter->first,
					STATUS_WHISPER);
					break;
				}
			}
			LeaveCriticalSection(&userCs);
			// �ӼӸ� ����� ��ã��
			if (!find) {
				sendMsg = name;
				sendMsg += " ���� ã�� �� �����ϴ�";

				iocpService->SendToOneMsg(sendMsg.c_str(), sock,
				STATUS_WAITING);
			}
		}

	} else if (msg.compare("6") == 0) { // �α׾ƿ�
		EnterCriticalSection(&userCs);

		string userId = idMap.find(userMap.find(sock)->second->userId)->first;
		userMap.erase(sock); // ���� ���� ���� ����
		idMap.find(userId)->second.logMode = NOW_LOGOUT;
		LeaveCriticalSection(&userCs);
		string sendMsg = loginBeforeMessage;

		iocpService->SendToOneMsg(sendMsg.c_str(), sock, STATUS_LOGOUT);
	} else { // �׿� ��ɾ� �Է�
		string sendMsg = errorMessage;
		sendMsg += waitRoomMessage;
		// ������ �����̹Ƿ� STATUS_WAITING ���·� �����Ѵ�

		iocpService->SendToOneMsg(sendMsg.c_str(), sock, STATUS_WAITING);
	}
}

// ä�ù濡���� ���� ó��
// ���ǰ� ����
void BusinessService::StatusChat(SOCKET sock, int status, int direction,
		char *message) {

	string name;
	string msg;

	// ���ǿ��� �̸� ���� ����
	name = userMap.find(sock)->second->userName;
	msg = message;

	if (msg.compare("\\out") == 0) { // ä�ù� ����

		string sendMsg;
		string roomName;
		sendMsg = name;
		sendMsg += " ���� �������ϴ�!";

		if (roomMap.find(userMap.find(sock)->second->roomName)
				!= roomMap.end()) { // null �˻�

			iocpService->SendToRoomMsg(sendMsg.c_str(),
					roomMap.find(userMap.find(sock)->second->roomName)->second,
					STATUS_CHATTIG);

			roomName = userMap.find(sock)->second->roomName;
			// ���̸� �ӽ� ����
			strncpy(userMap.find(sock)->second->roomName, "", NAME_SIZE);
			userMap.find(sock)->second->status = STATUS_WAITING;
		}

		msg = waitRoomMessage;

		iocpService->SendToOneMsg(msg.c_str(), sock, STATUS_WAITING);
		cout << "������ ��� name : " << name << endl;
		cout << "������ �� name : " << roomName << endl;

		// ������ ��� ���� out
		EnterCriticalSection(&roomCs);
		(roomMap.find(roomName)->second).remove(sock);

		if ((roomMap.find(roomName)->second).size() == 0) { // ���ο� 0���̸� �� ����
			roomMap.erase(roomName);
		}
		LeaveCriticalSection(&roomCs);
	} else { // ä�ù濡�� ä����

		string sendMsg;
		sendMsg = name;
		sendMsg += " : ";
		sendMsg += msg;

		if (roomMap.find(userMap.find(sock)->second->roomName)
				!= roomMap.end()) { // null �˻�

			iocpService->SendToRoomMsg(sendMsg.c_str(),
					roomMap.find(userMap.find(sock)->second->roomName)->second,
					STATUS_CHATTIG);
		}
	}

}

// Ŭ���̾�Ʈ���� ���� ������ ������ ����ü ����
char* BusinessService::DataCopy(LPPER_IO_DATA ioInfo, int *status,
		int *direction) {
	memcpy(status, ((char*) ioInfo->recvBuffer) + 4, 4); // Status
	memcpy(direction, ((char*) ioInfo->recvBuffer) + 8, 4); // Direction
	char* msg = new char[ioInfo->bodySize];	// Msg
	memcpy(msg, ((char*) ioInfo->recvBuffer) + 12, ioInfo->bodySize);
	// �� ���� �޾����� �Ҵ� ����
	delete ioInfo->recvBuffer;
	// �޸� ����
	// delete ioInfo;
	MPool* mp = MPool::getInstance();
	mp->free(ioInfo);

	return msg;
}

// ��Ŷ ������ �б�
void BusinessService::PacketReading(LPPER_IO_DATA ioInfo, DWORD bytesTrans) {

	// IO �Ϸ��� ���� �κ�
	if (READ == ioInfo->serverMode) {
		if (bytesTrans < 4) { // ����� �� �� �о�� ��Ȳ
			memcpy(((char*) &(ioInfo->bodySize)) + ioInfo->recvByte,
					ioInfo->buffer, bytesTrans);
		} else {
			memcpy(&(ioInfo->bodySize), ioInfo->buffer, 4);
			ioInfo->recvBuffer = new char[ioInfo->bodySize + 12]; // BodySize��ŭ ���� �Ҵ�
			memcpy(((char*) ioInfo->recvBuffer), ioInfo->buffer, bytesTrans);
		}
		ioInfo->recvByte += bytesTrans; // ���ݱ��� ���� ������ �� ����
	} else { // �� �б�
		if (ioInfo->recvByte >= 4) { // ��� �� �о���
			memcpy(((char*) ioInfo->recvBuffer) + ioInfo->recvByte,
					ioInfo->buffer, bytesTrans);
			ioInfo->recvByte += bytesTrans; // ���ݱ��� ���� ������ �� ����
		} else { // ����� �� ���о��� ���
			int recv = min(4 - ioInfo->recvByte, bytesTrans);
			memcpy(((char*) &(ioInfo->bodySize)) + ioInfo->recvByte,
					ioInfo->buffer, recv); // ������� ä���
			ioInfo->recvByte += bytesTrans; // ���ݱ��� ���� ������ �� ����
			if (ioInfo->recvByte >= 4) {
				ioInfo->recvBuffer = new char[ioInfo->bodySize + 12]; // BodySize��ŭ ���� �Ҵ�
				memcpy(((char*) ioInfo->recvBuffer) + 4,
						((char*) ioInfo->buffer) + recv, bytesTrans - recv); // ���� ����� �ʿ���� => �̶��� ���� �����͸� ����
			}
		}
	}

	if (ioInfo->totByte == 0 && ioInfo->recvByte >= 4) { // ����� �� �о�� ��Ż ����Ʈ ���� �� �� �ִ�
		ioInfo->totByte = ioInfo->bodySize + 12;
	}
}

IocpService::IocpService* BusinessService::getIocpService() {
	return this->iocpService;
}

} /* namespace Service */
