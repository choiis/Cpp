/*
 * common.h
 *
 *  Created on: 2019. 1. 8.
 *      Author: choiis1207
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <winsock2.h>
#include <list>

using namespace std;
#define BUF_SIZE 4096
#define NAME_SIZE 20
#define FILE_BUF_SIZE 32768

// Ŭ���̾�Ʈ ���� ���� => �������� �����Ұ�
#define STATUS_INIT 0
#define STATUS_LOGOUT 1
#define STATUS_WAITING 2
#define STATUS_CHATTIG 3
#define STATUS_WHISPER 4
#define STATUS_FILE_SEND 5

// �������� ���� ����
#define USER_MAKE 1
#define USER_ENTER 2

// ������ �� ���� ����
#define ROOM_MAKE 3
#define ROOM_ENTER 4
#define ROOM_OUT 5
#define WHISPER 6
#define ROOM_INFO 7
#define ROOM_USER_INFO 8
#define FRIEND_INFO 9
#define FRIEND_ADD 10
#define FRIEND_GO 11
#define FRIEND_DELETE 12
#define LOG_OUT 13
#define FILE_SEND 14
// Node �����ֿܼ��� ���� �޼���
#define CALLCOUNT 20
#define BAN 21
#define EXIT 22
// �α��� �ߺ� ����
#define NOW_LOGOUT 0
#define NOW_LOGIN 1

#define loginBeforeMessage "1.�������� 2.�α����ϱ� 3.���� 4.�ܼ������"
#define waitRoomMessage	"1.�� ���� ���� 2.�� ����� 3.�� �����ϱ� 4.���� ���� 5.ģ�� ���� 6.ģ�� ���� 7.�ӼӸ� 8.�α׾ƿ� 9.�ܼ������"
#define errorMessage "�߸��� ��ɾ� �Դϴ�"
#define chatRoomMessage  "�����÷��� \\out�� �ܼ������� \\clear�� ģ���߰��� \\add �г����� ����������\\send�� �Է��ϼ���"

#define SEND_ME 1
#define SEND_ROOM 2
#define SEND_FILE 3

#define SERVER_PORT "1234"
#define UDP_PORT "1236"
#define UDP_PORT_SEND "2222"

// �񵿱� ��ſ� �ʿ��� ����ü
typedef struct { // buffer info
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	char buffer[BUF_SIZE];  // 512Byte
	int serverMode;
	short recvByte; // ���ݱ��� ���� ����Ʈ�� �˷���
	short totByte; //��ü ���� ����Ʈ�� �˷���
	short bodySize; // ��Ŷ�� body ������
	char *recvBuffer;
} PER_IO_DATA, *LPPER_IO_DATA;

// JOB Queue�� ���� ����ü
typedef struct { // buffer info
	SOCKET socket;
	int direction;
	int nowStatus;
	string msg;
	int job;
} JOB_DATA, *P_JOB_DATA;

// Send Queue�� ���� ����ü
typedef struct { // buffer info
	string msg;
	SOCKET mySocket;
	string roomName;
	int direction;
	int status;
} Send_DATA, *P_Send_DATA;

// �񵿱� ��ſ� �ʿ��� ����ü
typedef struct { // buffer info
	list<SOCKET> userList;
	CRITICAL_SECTION listCs;
} ROOM_DATA, *P_ROOM_DATA;


#endif /* COMMON_H_ */
