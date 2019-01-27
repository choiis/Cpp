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

	string msg;
	char name[NAME_SIZE];

	strncpy(name, idMap.find(id)->second.nickname, NAME_SIZE);

	PER_HANDLE_DATA userInfo;
	cout << "START user : " << name << endl;
	// ������ ���� ���� �ʱ�ȭ
	userInfo.status = STATUS_WAITING;

	strncpy(userInfo.userId, id, NAME_SIZE);
	strncpy(userInfo.userName, name, NAME_SIZE);
	strncpy(userInfo.roomName, "", NAME_SIZE);

	EnterCriticalSection(&userCs);
	// �������� insert
	this->userMap[sock] = userInfo;
	cout << "���� ���� �ο� �� : " << userMap.size() << endl;
	LeaveCriticalSection(&userCs);

	// id�� key���� ���� �̸��� �ƴ϶� ���̵�!!
	idMap.find(id)->second.logMode = NOW_LOGIN;

	msg = "������ ȯ���մϴ�!\n";
	msg.append(waitRoomMessage);

	iocpService->SendToOneMsg(msg.c_str(), sock, STATUS_WAITING);
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
			// �α��� ���� ���󺹱�
			idMap.find(id)->second.logMode = NOW_LOGOUT;

			// ���̸� �ӽ� ����
			if (strcmp(roomName, "") != 0) { // �濡 �������� ���
				// ������ ��� ���� out

				EnterCriticalSection(&roomCs);
				int roomMapCnt = roomMap.count(roomName);
				LeaveCriticalSection(&roomCs);

				if (roomMapCnt != 0) { // null üũ �켱
					EnterCriticalSection(
							&roomMap.find(roomName)->second.listCs);
					(roomMap.find(roomName)->second).userList.remove(sock);
					LeaveCriticalSection(
							&roomMap.find(roomName)->second.listCs);

					if ((roomMap.find(roomName)->second).userList.size() == 0) { // ���ο� 0���̸� �� ����
						DeleteCriticalSection(
								&roomMap.find(roomName)->second.listCs);
						// �� ���� ���� CS�� Delete ģ��
						EnterCriticalSection(&roomCs);
						roomMap.erase(roomName);
						LeaveCriticalSection(&roomCs);
					} else {
						string sendMsg = name;
						// �̸����� ���� NAME_SIZE����
						sendMsg += " ���� �������ϴ�!";

						EnterCriticalSection(&userCs);
						string roomN = userMap.find(sock)->second.roomName;
						LeaveCriticalSection(&userCs);

						EnterCriticalSection(&roomCs);
						int roomMapCnt = roomMap.count(roomN);
						LeaveCriticalSection(&roomCs);

						if (roomMapCnt != 0) { // null �˻�

							iocpService->SendToRoomMsg(sendMsg.c_str(),
									roomMap.find(
											userMap.find(sock)->second.roomName)->second.userList,
									STATUS_CHATTIG,
									&roomMap.find(
											userMap.find(sock)->second.roomName)->second.listCs);
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
void BusinessService::StatusLogout(SOCKET sock, int direction, char *message) {

	string msg = "";

	if (direction == USER_MAKE) { // 1�� ��������

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
		if (sArr[0] != NULL && sArr[1] != NULL && sArr[2] != NULL) {
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
		}

	} else if (direction == USER_ENTER) { // 2�� �α��� �õ�

		char *sArr[2] = { NULL, };
		char *ptr = strtok(message, "\\"); // ���� ���ڿ��� �������� ���ڿ��� �ڸ�
		int i = 0;
		while (ptr != NULL)            // �ڸ� ���ڿ��� ������ ���� ������ �ݺ�
		{
			sArr[i] = ptr;           // ���ڿ��� �ڸ� �� �޸� �ּҸ� ���ڿ� ������ �迭�� ����
			i++;                       // �ε��� ����
			ptr = strtok(NULL, "\\");   // ���� ���ڿ��� �߶� �����͸� ��ȯ
		}
		if (sArr[0] != NULL && sArr[1] != NULL) {
			EnterCriticalSection(&idCs);
			if (idMap.find(sArr[0]) == idMap.end()) { // ���� ����
				LeaveCriticalSection(&idCs); // Case Lock ����
				msg.append("���� �����Դϴ� ���̵� Ȯ���ϼ���!\n");
				msg.append(loginBeforeMessage);

				iocpService->SendToOneMsg(msg.c_str(), sock, STATUS_LOGOUT);
			} else if (idMap.find(sArr[0])->second.password.compare(
					string(sArr[1])) == 0) { // ��й�ȣ ��ġ
				LeaveCriticalSection(&idCs); // Case Lock ����
				if (idMap.find(sArr[0])->second.logMode == NOW_LOGIN) { // �ߺ��α��� ��ȿ�� �˻�

					msg.append("�ߺ� �α����� �ȵ˴ϴ�!\n");
					msg.append(loginBeforeMessage);

					iocpService->SendToOneMsg(msg.c_str(), sock, STATUS_LOGOUT);
				} else { // �ߺ��α��� X
					InitUser(sArr[0], sock); // �������� ����
				}

			} else { // ��й�ȣ Ʋ��
				LeaveCriticalSection(&idCs); // Case Lock ����
				msg.append("��й�ȣ Ʋ��!\n");
				msg.append(loginBeforeMessage);

				iocpService->SendToOneMsg(msg.c_str(), sock, STATUS_LOGOUT);
			}
		}

	} else if (direction == USER_LIST) { // ������ ���� ��û

		string userList = "���̵� ���� ����Ʈ";
		if (idMap.size() == 0) {
			userList += " ����";
		} else {
			EnterCriticalSection(&idCs);
			unordered_map<string, USER_DATA> idCopyMap = idMap;
			LeaveCriticalSection(&idCs);

			// ����ȭ ���� ��������
			unordered_map<string, USER_DATA>::const_iterator iter;
			for (iter = idCopyMap.begin(); iter != idCopyMap.end(); ++iter) {
				userList += "\n";
				userList += (iter->first).c_str();
			}
		}
		iocpService->SendToOneMsg(userList.c_str(), sock, STATUS_LOGOUT);
	} else { // �׿� ��ɾ� �Է�
		string sendMsg = errorMessage;
		sendMsg += waitRoomMessage;
		// ������ �����̹Ƿ� STATUS_WAITING ���·� �����Ѵ�
		iocpService->SendToOneMsg(sendMsg.c_str(), sock, STATUS_LOGOUT);
	}

	CharPool* charPool = CharPool::getInstance();
	charPool->Free(message);
}

// ���ǿ����� ���� ó��
// ���ǰ� ����
void BusinessService::StatusWait(SOCKET sock, int status, int direction,
		char *message) {

	string name = userMap.find(sock)->second.userName;
	string msg = string(message);
	// ���ǿ��� �̸� ���� ����

	if (direction == ROOM_MAKE) { // ���ο� �� ���鶧

		// ��ȿ�� ���� ����
		EnterCriticalSection(&roomCs);
		int cnt = roomMap.count(msg);
		LeaveCriticalSection(&roomCs);

		if (cnt != 0) { // ���̸� �ߺ�
			msg += "�̹� �ִ� �� �̸��Դϴ�!\n";
			msg += waitRoomMessage;

			iocpService->SendToOneMsg(msg.c_str(), sock, STATUS_WAITING);
			// �ߺ� ���̽��� �� ���� �� ����
		} else { // ���̸� �ߺ� �ƴ� ���� ����
				 // ���ο� �� ���� ����
			ROOM_DATA roomData;
			list<SOCKET> chatList;
			chatList.push_back(sock);
			// �� ����Ʈ�� CS��ü
			CRITICAL_SECTION cs;
			InitializeCriticalSection(&cs);
			// �� ����Ʈ�� CS��ü Init
			roomData.userList = chatList;
			roomData.listCs = cs;

			EnterCriticalSection(&roomCs);
			roomMap[msg] = roomData;
			LeaveCriticalSection(&roomCs);

			// User�� ���� ���� �ٲ۴�
			strncpy((userMap.find(sock))->second.roomName, msg.c_str(),
			NAME_SIZE);
			(userMap.find(sock))->second.status =
			STATUS_CHATTIG;

			msg += " ���� �����Ǿ����ϴ�.";
			msg += chatRoomMessage;
			// cout << "���� ������ �� ���� : " << roomMap.size() << endl;

			iocpService->SendToOneMsg(msg.c_str(), sock, STATUS_CHATTIG);
		}

	} else if (direction == ROOM_ENTER) { // �� ���� ��û

		// ��ȿ�� ���� ����
		EnterCriticalSection(&roomCs);
		int cnt = roomMap.count(msg);
		LeaveCriticalSection(&roomCs);
		if (cnt == 0) { // �� ��ã��
			msg = "���� �� �Դϴ�!\n";
			msg += waitRoomMessage;
			iocpService->SendToOneMsg(msg.c_str(), sock, STATUS_WAITING);
		} else {

			//���� �����ϱ� ������ insert
			EnterCriticalSection(&(roomMap.find(msg)->second.listCs));
			roomMap.find(msg)->second.userList.push_back(sock);
			LeaveCriticalSection(&(roomMap.find(msg)->second.listCs));

			strncpy(userMap.find(sock)->second.roomName, msg.c_str(),
			NAME_SIZE);
			userMap.find(sock)->second.status = STATUS_CHATTIG;

			string sendMsg = name;
			sendMsg += " ���� �����ϼ̽��ϴ�. ";
			sendMsg += chatRoomMessage;

			if (roomMap.find(msg) != roomMap.end()) { // ������ �˻�

				iocpService->SendToRoomMsg(sendMsg.c_str(),
						roomMap.find(msg)->second.userList,
						STATUS_CHATTIG, &roomMap.find(msg)->second.listCs);

			}
		}

	} else if (direction == ROOM_INFO) { // �� ���� ��û��

		string str;
		if (roomMap.size() == 0) {
			str = "������� ���� �����ϴ�";
		} else {
			str += "�� ���� ����Ʈ";
			EnterCriticalSection(&roomCs);
			unordered_map<string, ROOM_DATA> roomCopyMap = roomMap;
			LeaveCriticalSection(&roomCs);
			// ����ȭ ���� ���� ��������
			unordered_map<string, ROOM_DATA>::const_iterator iter;

			// �������� ���ڿ��� �����
			for (iter = roomCopyMap.begin(); iter != roomCopyMap.end();
					iter++) {

				str += "\n";
				str += iter->first.c_str();
				str += ":";
				str += to_string((iter->second).userList.size());
				str += "��";
			}

		}

		iocpService->SendToOneMsg(str.c_str(), sock, STATUS_WAITING);
	} else if (direction == ROOM_USER_INFO) { // ���� ���� ��û��
		string str = "���� ���� ����Ʈ";

		EnterCriticalSection(&userCs);
		unordered_map<SOCKET, PER_HANDLE_DATA> userCopyMap = userMap;
		LeaveCriticalSection(&userCs);
		// ����ȭ ���� ���� ��������
		unordered_map<SOCKET, PER_HANDLE_DATA>::iterator iter;
		for (iter = userCopyMap.begin(); iter != userCopyMap.end(); iter++) {
			str += "\n";
			str += (iter->second).userName;
			str += ":";
			if ((iter->second).status == STATUS_WAITING) {
				str += "���� ";
			} else {
				str += (iter->second).roomName;
			}
		}

		iocpService->SendToOneMsg(str.c_str(), sock, STATUS_WAITING);
	} else if (direction == WHISPER) { // �ӼӸ�

		char *sArr[2] = { NULL, };
		char *ptr = strtok(message, "\\"); // ���� ���ڿ��� �������� ���ڿ��� �ڸ�
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

				iocpService->SendToOneMsg(sendMsg.c_str(), sock,
				STATUS_WAITING);
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
		}

	} else if (msg.compare("6") == 0) { // �α׾ƿ�

		EnterCriticalSection(&userCs);
		string userId = idMap.find(userMap.find(sock)->second.userId)->first;
		userMap.erase(sock); // ���� ���� ���� ����
		LeaveCriticalSection(&userCs);

		EnterCriticalSection(&idCs);
		idMap.find(userId)->second.logMode = NOW_LOGOUT;
		LeaveCriticalSection(&idCs);

		string sendMsg = loginBeforeMessage;

		iocpService->SendToOneMsg(sendMsg.c_str(), sock, STATUS_LOGOUT);
	} else { // �׿� ��ɾ� �Է�
		string sendMsg = errorMessage;
		sendMsg += waitRoomMessage;
		// ������ �����̹Ƿ� STATUS_WAITING ���·� �����Ѵ�

		iocpService->SendToOneMsg(sendMsg.c_str(), sock, STATUS_WAITING);
	}

	CharPool* charPool = CharPool::getInstance();
	charPool->Free(message);
}

// ä�ù濡���� ���� ó��
// ���ǰ� ����
void BusinessService::StatusChat(SOCKET sock, int status, int direction,
		char *message) {

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

		if (roomMap.find(userMap.find(sock)->second.roomName)
				!= roomMap.end()) { // null �˻�

			iocpService->SendToRoomMsg(sendMsg.c_str(),
					roomMap.find(userMap.find(sock)->second.roomName)->second.userList,
					STATUS_CHATTIG,
					&roomMap.find(userMap.find(sock)->second.roomName)->second.listCs);

			roomName = userMap.find(sock)->second.roomName;
			// ���̸� �ӽ� ����
			strncpy(userMap.find(sock)->second.roomName, "", NAME_SIZE);
			userMap.find(sock)->second.status = STATUS_WAITING;
		}

		msg = waitRoomMessage;

		iocpService->SendToOneMsg(msg.c_str(), sock, STATUS_WAITING);

		// ������ ��� ���� out
		EnterCriticalSection(&roomMap.find(roomName)->second.listCs);
		(roomMap.find(roomName)->second).userList.remove(sock);
		LeaveCriticalSection(&roomMap.find(roomName)->second.listCs);

		EnterCriticalSection(&roomCs);
		if ((roomMap.find(roomName)->second).userList.size() == 0) { // ���ο� 0���̸� �� ����
			DeleteCriticalSection(&roomMap.find(roomName)->second.listCs);
			roomMap.erase(roomName);
		}
		LeaveCriticalSection(&roomCs);
	} else { // ä�ù濡�� ä����

		string sendMsg;
		sendMsg = name;
		sendMsg += " : ";
		sendMsg += msg;
		EnterCriticalSection(&roomCs);
		unordered_map<string, ROOM_DATA>::iterator it = roomMap.find(
				userMap.find(sock)->second.roomName);
		LeaveCriticalSection(&roomCs);

		if (it != roomMap.end()) { // null �˻�
			iocpService->SendToRoomMsg(sendMsg.c_str(), it->second.userList,
			STATUS_CHATTIG,
					&roomMap.find(userMap.find(sock)->second.roomName)->second.listCs);
		}

	}
	CharPool* charPool = CharPool::getInstance();
	charPool->Free(message);
}

// Ŭ���̾�Ʈ���� ���� ������ ������ ����ü ����
// ��� �޸� ���� ��ȯ
char* BusinessService::DataCopy(LPPER_IO_DATA ioInfo, int *status,
		int *direction) {

	copy(((char*) ioInfo->recvBuffer) + 4, ((char*) ioInfo->recvBuffer) + 8,
			(char*) status);
	copy(((char*) ioInfo->recvBuffer) + 8, ((char*) ioInfo->recvBuffer) + 12,
			(char*) direction);

	CharPool* charPool = CharPool::getInstance();
	char* msg = charPool->Malloc(); // 512 Byte���� ī�� ����
	copy(((char*) ioInfo->recvBuffer) + 12,
			((char*) ioInfo->recvBuffer) + 12
					+ min(ioInfo->bodySize, (DWORD) BUF_SIZE), msg);

	// �� ���� �޾����� �Ҵ� ����
	charPool->Free(ioInfo->recvBuffer);

	MPool* mp = MPool::getInstance();
	mp->Free(ioInfo);

	return msg;
}

// ��Ŷ ������ �б�
void BusinessService::PacketReading(LPPER_IO_DATA ioInfo, DWORD bytesTrans) {
	// IO �Ϸ��� ���� �κ�
	if (READ == ioInfo->serverMode) {
		if (bytesTrans < 4) { // ����� �� �� �о�� ��Ȳ
			copy(((char*) &(ioInfo->bodySize)) + ioInfo->recvByte,
					((char*) &(ioInfo->bodySize)) + ioInfo->recvByte
							+ bytesTrans, ioInfo->buffer);
			ioInfo->recvByte += bytesTrans; // ���ݱ��� ���� ������ �� ����
		} else {
			copy(ioInfo->buffer, ioInfo->buffer + 4,
					(char*) &(ioInfo->bodySize));
			if (ioInfo->bodySize >= 0 && ioInfo->bodySize <= BUF_SIZE - 12) {
				CharPool* charPool = CharPool::getInstance();
				ioInfo->recvBuffer = charPool->Malloc(); // 512 Byte���� ī�� ����
				copy(ioInfo->buffer, ioInfo->buffer + bytesTrans,
						((char*) ioInfo->recvBuffer));
				ioInfo->recvByte += bytesTrans; // ���ݱ��� ���� ������ �� ����
			} else { // �߸��� bodySize;
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
				ioInfo->recvBuffer = charPool->Malloc(); // 512 Byte���� ī�� ����
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
