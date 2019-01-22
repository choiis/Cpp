/*
 * common.h
 *
 *  Created on: 2019. 1. 8.
 *      Author: choiis1207
 */

#ifndef COMMON_H_
#define COMMON_H_

using namespace std;
#define SIZE 256
#define BUF_SIZE 512
#define NAME_SIZE 20

// CP�� Recv ������ READ Send ������ WRITE
#define READ 6
#define WRITE 7
#define READ_MORE 8

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
#define ROOM_INFO 5
#define ROOM_USER_INFO 6
// �α��� �ߺ� ����
#define NOW_LOGOUT 0
#define NOW_LOGIN 1

#define loginBeforeMessage "1.�������� 2.�α����ϱ� 3.���� ����Ʈ 4.���� 5.�ܼ������"
#define waitRoomMessage	"1.�� ���� ���� 2.�� ����� 3.�� �����ϱ� 4.���� ���� 5.�ӼӸ� 6.�α׾ƿ� 7.�ܼ������"
#define errorMessage "�߸��� ��ɾ� �Դϴ�"
#define chatRoomMessage  "�����÷��� \\out�� �ܼ������� \\clear�� �Է��ϼ���"

// ���� ��� ��Ŷ ������
typedef struct {
	int bodySize;
	int clientStatus;
	int direction;
	char *message;
} PACKET_DATA, *P_PACKET_DATA;

// �񵿱� ��ſ� �ʿ��� ����ü
typedef struct { // buffer info
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	char buffer[SIZE];  // ���ۿ� PACKET_DATA�� �� ���̹Ƿ� ũ�� ����
	int serverMode;
	DWORD recvByte; // ���ݱ��� ���� ����Ʈ�� �˷���, ���ݱ��� ���� ���
	DWORD totByte; //��ü ���� ����Ʈ�� �˷���, �� ���� ���
	int bodySize; // ��Ŷ�� body ������
	char *recvBuffer;
} PER_IO_DATA, *LPPER_IO_DATA;

#endif /* COMMON_H_ */
