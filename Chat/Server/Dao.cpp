/*
* IocpService.h
*
*  Created on: 2019. 1. 28.
*      Author: choiis1207
*/

#include "Dao.h"

Dao::Dao() {

	// ȯ�� �ڵ鷯 �Ҵ�
	SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
	// ODBC ����̹� ���� ���
	SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, SQL_IS_INTEGER);
	// ���� �ڵ鷯 �Ҵ�
	SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);

	string SqlConnection = "conf/SqlConnection.properties";

	ifstream conf(SqlConnection);
	
	if (!conf.is_open()) {
		cout << "File conf read error!" << endl;
		exit(1);
	}

	char serverIp[32];
	char serverPort[32];
	char serverDB[32];
	char serverUser[32];
	char serverPassword[32];
	conf >> serverIp >> serverPort >> serverDB >> serverUser >> serverPassword;

	char connect[1024];
	sprintf(connect, "DRIVER={SQL Server};SERVER=%s, %s; DATABASE=%s; UID=%s; PWD=%s", serverIp, serverPort, serverDB, serverUser, serverPassword);
	cout << "connect " << connect << endl;
	// DB connection
	res = SQLDriverConnect(hDbc, NULL,
		(SQLCHAR*) connect,
		SQL_NTS, NULL, 1024, NULL, SQL_DRIVER_NOPROMPT); //���� ���� �Է�. (���� = 1, ���� = -1 ����)
	if (res == 1) {
		std::cout << "DB connection success" << std::endl;
	}
	else {
		std::cout << "DB connection fail" << std::endl;
	}
}

Dao::~Dao() {

	if (hStmt) {
		SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
	}
	if (hDbc){
		SQLDisconnect(hDbc);
	}
	if (hDbc){
		SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
	}
	if (hDbc){
		SQLFreeHandle(SQL_HANDLE_ENV, hDbc);
	}
}

// ID���� select
UserVo Dao::selectUser(UserVo& vo){

	lock_guard<mutex> guard(this->lock);

	res = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
	char query[512] = "select userid,password,nickname from cso_id with (nolock) where userid = ? ";

	SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0, (SQLCHAR*)vo.getUserId(), sizeof(vo.getUserId()), NULL);
	SQLPrepare(hStmt, (SQLCHAR*)query, SQL_NTS);

	char userid[10];
	char password[10];
	char nickname[20];
	SQLINTEGER num1, num2, num3;

	SQLBindCol(hStmt, 1, SQL_C_CHAR, userid, sizeof(userid), &num1);
	SQLBindCol(hStmt, 2, SQL_C_CHAR, password, sizeof(password), &num2);
	SQLBindCol(hStmt, 3, SQL_C_CHAR, nickname, sizeof(nickname), &num3);

	res = SQLExecute(hStmt);

	if (SQLFetch(hStmt) != SQL_NO_DATA) {
		vo.setPassword(password);
		vo.setNickName(nickname);
	}
	else {
		vo.setUserId("");
	}

	SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

	return vo;
}


// ID���� update
void Dao::UpdateUser(const LogVo& vo){

	lock_guard<mutex> guard(this->lock);

	res = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);


	char query[512] = "update cso_id set lastlogdate = getdate()  where userid = ? ";

	res = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0, (SQLCHAR*)vo.getUserId(), sizeof(vo.getUserId()), NULL);

	SQLPrepare(hStmt, (SQLCHAR*)query, SQL_NTS);

	res = SQLExecute(hStmt);

	SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

// ID���� insert
void Dao::InsertUser(const UserVo& vo) {

	lock_guard<mutex> guard(this->lock);
	res = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

	char query[512] = "insert into cso_id(userid, password , nickname , regdate) values(? ,? ,? ,getdate() ) ";

	res = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0, (SQLCHAR*)vo.getUserId(), sizeof(vo.getUserId()), NULL);
	res = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 10, 0, (SQLCHAR*)vo.getPassword(), sizeof(vo.getPassword()), NULL);
	res = SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0, (SQLCHAR*)vo.getNickName(), sizeof(vo.getNickName()), NULL);

	SQLPrepare(hStmt, (SQLCHAR*)query, SQL_NTS);

	res = SQLExecute(hStmt);

	SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

// �α��� ���� insert
void Dao::InsertLogin(const LogVo& vo) {

	lock_guard<mutex> guard(this->lock);
	res = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

	char query[512] = "insert into cso_login(userid, logindate , nickname) values(? ,getdate() ,? ) ";

	SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0, (SQLCHAR*)vo.getUserId(), sizeof(vo.getUserId()), NULL);
	SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0, (SQLCHAR*)vo.getNickName(), sizeof(vo.getNickName()), NULL);

	SQLPrepare(hStmt, (SQLCHAR*)query, SQL_NTS);

	res = SQLExecute(hStmt);

	SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

}

// ���� �α� insert
void Dao::InsertDirection(const LogVo& vo) {

	lock_guard<mutex> guard(this->lock);

	res = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

	char query[512] = "insert into cso_direction(nickname, logdate, status, direction , message) values(? ,getdate() ,? ,? ,? ) ";

	SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0, (SQLCHAR*)vo.getNickName(), sizeof(vo.getNickName()), NULL);

	int status = vo.getStatus();
	int direction = vo.getDirection();

	SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &status, 0, NULL);
	SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &direction, 0, NULL);
	SQLBindParameter(hStmt, 4, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 512, 0, (SQLCHAR*)vo.getMsg(), sizeof(vo.getMsg()), NULL);

	SQLPrepare(hStmt, (SQLCHAR*)query, SQL_NTS);

	res = SQLExecute(hStmt);
	SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

}

// ä�� �α� insert
void  Dao::InsertChatting(const LogVo& vo) {

	lock_guard<mutex> guard(this->lock);
	res = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

	char query[512] = "insert into cso_chatting(nickname,logdate,roomname,message)  values(? ,getdate() , ? , ?)";

	SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0, (SQLCHAR*)vo.getNickName(), sizeof(vo.getNickName()), NULL);
	SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 10, 0, (SQLCHAR*)vo.getRoomName(), sizeof(vo.getRoomName()), NULL);
	SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 512, 0, (SQLCHAR*)vo.getMsg(), sizeof(vo.getMsg()), NULL);

	SQLPrepare(hStmt, (SQLCHAR*)query, SQL_NTS);

	res = SQLExecute(hStmt);
	SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

}

// ģ�� �Ǵ� ���ܰ��� insert
int Dao::InsertRelation(const RelationVo& vo){

	lock_guard<mutex> guard(this->lock);
	res = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

	char query[512] = "insert into cso_relation  values(? , ? , ? ,getdate())";

	int relation = vo.getRelationcode();; // relation�� 1 ģ������ 2����

	SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0, (SQLCHAR*)vo.getUserId(), sizeof(vo.getUserId()), NULL);
	SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0, (SQLCHAR*)vo.getRelationto(), sizeof(vo.getRelationto()), NULL);
	SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &relation, 0, NULL);

	SQLPrepare(hStmt, (SQLCHAR*)query, SQL_NTS);

	res = SQLExecute(hStmt);
	int result = res;
	SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

	return result;
}

// ID���� �ִ��� Ȯ��
RelationVo Dao::findUserId(const RelationVo& vo) {

	lock_guard<mutex> guard(this->lock);
	res = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
	char query[512] = "select userid, nickname from cso_id  with (nolock) where nickname = ? ";

	SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0, (SQLCHAR*)vo.getNickName(), sizeof(vo.getNickName()), NULL);
	SQLPrepare(hStmt, (SQLCHAR*)query, SQL_NTS);

	char userid[10];
	char nickname[20];
	SQLINTEGER num1, num2;

	SQLBindCol(hStmt, 1, SQL_C_CHAR, userid, sizeof(userid), &num1);
	SQLBindCol(hStmt, 2, SQL_C_CHAR, nickname, sizeof(nickname), &num2);

	res = SQLExecute(hStmt);

	RelationVo vo2;

	if (SQLFetch(hStmt) != SQL_NO_DATA) {
		vo2.setRelationto(userid);
		vo2.setNickName(nickname);
	}
	else {
		vo2.setRelationto("");
	}


	SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

	return vo2;
}
// ģ�� ���� select
vector<RelationVo> Dao::selectFriends(const RelationVo& vo) {

	lock_guard<mutex> guard(this->lock);
	res = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
	char query[1024] = "select T1.relationfrom, T1.relationto, T2.nickname from cso_relation T1 with (nolock), cso_id T2 with (nolock)	where T1.relationto = T2.userid	and T1.relationfrom = ? and T1.relationcode = 1";

	SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0, (SQLCHAR*)vo.getUserId(), sizeof(vo.getUserId()), NULL);
	SQLPrepare(hStmt, (SQLCHAR*)query, SQL_NTS);

	char userid[10];
	char relationto[10];
	char nickname[20];
	SQLINTEGER num1, num2, num3;

	SQLBindCol(hStmt, 1, SQL_C_CHAR, userid, sizeof(userid), &num1);
	SQLBindCol(hStmt, 2, SQL_C_CHAR, relationto, sizeof(relationto), &num2);
	SQLBindCol(hStmt, 3, SQL_C_CHAR, nickname, sizeof(nickname), &num3);

	res = SQLExecute(hStmt);

	vector<RelationVo> vec;

	while (SQLFetch(hStmt) != SQL_NO_DATA) {
		RelationVo vo;
		vo.setUserId(userid);
		vo.setRelationto(relationto);
		vo.setNickName(nickname);
		vec.push_back(vo);
	}


	SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

	return vec;
}

// ģ���Ѹ����� select
RelationVo Dao::selectOneFriend(const RelationVo& vo) {

	lock_guard<mutex> guard(this->lock);
	res = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
	char query[1024] = "select T1.relationfrom, T1.relationto, T2.nickname from cso_relation T1 with (nolock) , cso_id T2	with (nolock) where T1.relationto = T2.userid	and T1.relationfrom = ? and T2.nickname = ? and T1.relationcode = 1";

	SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0, (SQLCHAR*)vo.getUserId(), sizeof(vo.getUserId()), NULL);
	SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0, (SQLCHAR*)vo.getRelationto(), sizeof(vo.getRelationto()), NULL);
	SQLPrepare(hStmt, (SQLCHAR*)query, SQL_NTS);

	char userid[20];
	char relationto[20];
	char nickname[20];
	SQLINTEGER num1, num2, num3;

	SQLBindCol(hStmt, 1, SQL_C_CHAR, userid, sizeof(userid), &num1);
	SQLBindCol(hStmt, 2, SQL_C_CHAR, relationto, sizeof(relationto), &num2);
	SQLBindCol(hStmt, 3, SQL_C_CHAR, nickname, sizeof(nickname), &num3);

	res = SQLExecute(hStmt);

	RelationVo vo2;
	if (SQLFetch(hStmt) != SQL_NO_DATA) {
		vo2.setUserId(userid);
		vo2.setRelationto(relationto);
		vo2.setNickName(nickname);
	}

	SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

	return vo2;
}

// ģ�� �Ǵ� ���ܰ��� delete
int Dao::DeleteRelation(const RelationVo& vo) {

	lock_guard<mutex> guard(this->lock);
	res = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

	char query[512] = "delete from cso_relation where relationfrom = ? and relationcode = ? and relationto = (select userid from cso_id where nickname = ?)";

	int relation = vo.getRelationcode();; // relation�� 1 ģ������ 2����

	SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0, (SQLCHAR*)vo.getUserId(), sizeof(vo.getUserId()), NULL);
	SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &relation, 0, NULL);
	SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0, (SQLCHAR*)vo.getNickName(), sizeof(vo.getNickName()), NULL);

	SQLPrepare(hStmt, (SQLCHAR*)query, SQL_NTS);

	res = SQLExecute(hStmt);
	int result = res;
	SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

	return result;
}

// File���� ���� insert
int Dao::InsertFiles(const LogVo& vo) {
	lock_guard<mutex> guard(this->lock);
	res = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

	char query[512] = "insert into cso_filerecv  values(? , ? , getdate() , ?)";

	int relation = vo.getBytes();; // relation�� 1 ģ������ 2����

	SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0, (SQLCHAR*)vo.getNickName(), sizeof(vo.getNickName()), NULL);
	SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0, (SQLCHAR*)vo.getFilename(), sizeof(vo.getFilename()), NULL);
	SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &relation, 0, NULL);

	SQLPrepare(hStmt, (SQLCHAR*)query, SQL_NTS);

	res = SQLExecute(hStmt);
	int result = res;
	SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

	return result;
}