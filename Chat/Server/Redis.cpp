
#include "Redis.h"

Redis::Redis() {

	// ����
	conn = redisConnect("172.30.1.23",  // ����ó redis ����
		6379           // ��Ʈ ��ȣ
	);

	if ((NULL != conn) && conn->err) {
		// error
		printf("error : %s\n", conn->errstr);
		redisFree(conn);
		exit(-1);
	}
	else if (NULL == conn) {
		exit(-1);
	}
};

Redis::~Redis() {
	// �� ����
	redisFree(conn);
};



void Redis::Set(const string& key, const string& value) {

	lock_guard<mutex> guard(this->lock);
	
	char query[512];
	sprintf(query, "set %s %s", key.c_str(), value.c_str());
	resp = (redisReply*)redisCommand(conn, query);// ��ɾ�
};

string Redis::Get(const string& key) {

	lock_guard<mutex> guard(this->lock);
	
	char query[512];
	sprintf(query, "get %s", key.c_str());
	resp = (redisReply*)redisCommand(conn, query);// ��ɾ�
	return string(resp->str);
};

void Redis::Zadd(const string& key, const string& value, int num) {

	lock_guard<mutex> guard(this->lock);
	
	char query[512];
	sprintf(query, "zincrby %s %d %s", key.c_str(),num ,value.c_str());
	resp = (redisReply*)redisCommand(conn, query);// ��ɾ�
};

void Redis::Zincr(const string& key, const string& value) {
	
	lock_guard<mutex> guard(this->lock); 
	
	char query[512];
	sprintf(query, "zincrby %s 1 %s", key.c_str(), value.c_str());
	resp = (redisReply*)redisCommand(conn, query);// ��ɾ�
};

vector<RankVo> Redis::Zrevrange(const string& key, int num) {

	{
		lock_guard<mutex> guard(this->lock);

		char query[512];
		sprintf(query, "zrevrange %s 0 %d withscores", key.c_str(), num);
		resp = (redisReply*)redisCommand(conn, query);// ��ɾ�
	}
	std::vector<RankVo> vec;

	for (size_t i = 0; i < resp->elements; i+=2)
	{
		RankVo vo;
		vo.setNickName(resp->element[i]->str);
		vo.setPoint(stoi(resp->element[i + 1]->str));
		vec.push_back(vo);
	}

	return vec;
};
