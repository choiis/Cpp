/*
 * common.h
 *
 *  Created on: 2019. 1. 8.
 *      Author: choiis1207
 */

#ifndef COMMON_H_
#define COMMON_H_

using namespace std;

#define BUF_SIZE 4096
#define NAME_SIZE 20

// CP�� Recv ������ READ Send ������ WRITE
#define READ 6
#define WRITE 7
#define READ_MORE 8

// Ŭ���̾�Ʈ ���� ���� => �������� �����Ұ�
#define STATUS_NO_CHANGE 0
#define STATUS_LOGOUT 1
#define STATUS_WAITING 2
#define STATUS_CHATTIG 3
#define STATUS_WHISPER 4

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
// �α��� �ߺ� ����
#define NOW_LOGOUT 0
#define NOW_LOGIN 1

#define loginBeforeMessage "1.�������� 2.�α����ϱ� 3.���� 4.�ܼ������"
#define waitRoomMessage	"1.�� ���� ���� 2.�� ����� 3.�� �����ϱ� 4.���� ���� 5.�ӼӸ� 6.�α׾ƿ� 7.�ܼ������"
#define errorMessage "�߸��� ��ɾ� �Դϴ�"
#define chatRoomMessage  "�����÷��� \\out�� �ܼ������� \\clear�� �Է��ϼ���"

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

#endif /* COMMON_H_ */
