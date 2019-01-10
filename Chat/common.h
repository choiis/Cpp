/*
 * common.h
 *
 *  Created on: 2019. 1. 8.
 *      Author: choiis1207
 */

#ifndef COMMON_H_
#define COMMON_H_

using namespace std;
#define BUF_SIZE 512
#define NAME_SIZE 20

// CP�� Recv ������ READ Send ������ WRITE
#define READ 1
#define WRITE 2

// Ŭ���̾�Ʈ ���� ���� => �������� �����Ұ�
#define STATUS_INIT 0
#define STATUS_LOGOUT 1
#define STATUS_WAITING 2
#define STATUS_CHATTIG 3
#define STATUS_WHISPER 4

// �������� ���� ����
#define USER_MAKE 1
#define USER_ENTER 2
#define USER_LIST 3
#define USER_OUT 4

// ������ �� ���� ����
#define ROOM_MAKE 1
#define ROOM_ENTER 2
#define ROOM_OUT 3
#define WHISPER 4

// �α��� �ߺ� ����
#define NOW_LOGOUT 0
#define NOW_LOGIN 1

// ���� ��� ��Ŷ ������
typedef struct {
	int clientStatus;
	char message[BUF_SIZE];
	char message2[NAME_SIZE];
	char message3[NAME_SIZE];
	int direction;
} PACKET_DATA, *P_PACKET_DATA;

// �񵿱� ��ſ� �ʿ��� ����ü
typedef struct { // buffer info
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	char buffer[sizeof(PACKET_DATA)];  // ���ۿ� PACKET_DATA�� �� ���̹Ƿ� ũ�� ����
	int serverMode;
} PER_IO_DATA, *LPPER_IO_DATA;
char loginBeforeMessage[] = "1.�������� 2.�α����ϱ� 3.���� ����Ʈ 4.���� 5.�ܼ������";
char waitRoomMessage[] =
		"1.�� ���� ���� 2.�� ����� 3.�� �����ϱ� 4.���� ���� 5.�ӼӸ� 6.�α׾ƿ� 7.�ܼ������";
char errorMessage[] = "�߸��� ��ɾ� �Դϴ�\n";
char chatRoomMessage[] = "�����÷��� out�� �ܼ������� clear�� �Է��ϼ���";

#endif /* COMMON_H_ */
