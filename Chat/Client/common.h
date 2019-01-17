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
#define READ 1
#define WRITE 2
#define READ_MORE 3

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
#define errorMessage "�߸��� ��ɾ� �Դϴ�\n"
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

// �޸� Ǯ
class MPool {
private:
	char* data;
	bool* arr;
	DWORD cnt;
	DWORD idx;
public:
	MPool(DWORD size) {
		data = new char[size * sizeof(PER_IO_DATA)]; // size ���ϱ� ��ϼ� ��ŭ �Ҵ�
		arr = new bool[size]; // idx��° �޸� Ǯ�� �Ҵ� ���θ� ������
		cnt = size;
		idx = 0;
		memset(arr, 0, size);
	}

	~MPool() {
		delete data;
		delete arr;
	}

	LPPER_IO_DATA malloc() {
		// �Ҵ��� �ȵ� ����Ҹ� ã�´�
		while (arr[idx]) {
			idx++;
			if (idx == cnt) {
				idx = 0;
			}
		}

		arr[idx] = true;
		idx++;
		if (idx == cnt) { // �ε����� ������ ��°�϶�
			idx = 0;
			return (LPPER_IO_DATA) (data + ((cnt - 1) * (sizeof(PER_IO_DATA))));
		} else {
			return (LPPER_IO_DATA) (data + ((idx - 1) * (sizeof(PER_IO_DATA))));
		}

	}

	void free(LPPER_IO_DATA freePoint) { // ��ȯ�� �������� idx�� ���󺹱�
		DWORD returnIdx = ((((char*) freePoint) - data) / sizeof(PER_IO_DATA));
		arr[returnIdx] = false;
	}
};

#endif /* COMMON_H_ */
