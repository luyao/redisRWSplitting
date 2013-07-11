#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
//#include <iostream>
#include "hiredis.h"
#include "async.h"
#include "adapters/libevent.h"
#include "redis_rw_split_client.h"
#include "adapters/ae.h"
using namespace std;

static aeEventLoop *loop;

void getCallback(redisAsyncContext *c, void *r, void *privdata) {
    redisReply *reply = (redisReply*)r;
    if (reply == NULL) return;
    //printf("argv[%s]: %s\n", (char*)privdata, reply->str);

    /* Disconnect after receiving the reply to GET */
    //redisAsyncDisconnect(c);
}

void connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Connected...\n");
}

void disconnectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }

    printf("Disconnected...\n");
}

void* Callback(void *pParam)
{
	((RedisRWSplitClient*)pParam)->eventRun();
	return NULL;
}

const int MAX_LOOP = 10000;
class dis_timer
{
	timeval s, e;
	public:
	void start() {
		gettimeofday(&s, NULL);
	}
	void end() {
		gettimeofday(&e, NULL);
	}
	int costms () {
		return (int)((e.tv_sec-s.tv_sec)*1000 + (e.tv_usec-s.tv_usec)/1000);
	}
};


static void* __callback__(void *p)
{
	struct event_base *base = event_base_new();
	redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
	redisLibeventAttach(c,base);
	redisAsyncSetConnectCallback(c,connectCallback);
	redisAsyncSetDisconnectCallback(c,disconnectCallback);
	int i = 0;
	dis_timer timer;
	timer.start();
	while(++i < MAX_LOOP){
		redisAsyncCommand(c, NULL, NULL, "SET 123 abc");
	}
	timer.end();
	printf("同步写入10000条:%d\n",timer.costms() );
	event_base_dispatch( base );
	return NULL;
}

void bearedFunc(){

	pthread_t th;
	pthread_create(&th, NULL, __callback__, NULL);
	pthread_join(th, NULL);
	//event_base_dispatch( base );

}

void* aeFunction(void *param)
{
	dis_timer timer;
	timer.start();
	redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
	if (c->err) {
		/* Let *c leak for now... */
		printf("Error: %s\n", c->errstr);
		return NULL;
	}   

	loop = aeCreateEventLoop(8192);
	redisAeAttach(loop, c); 
	redisAsyncSetConnectCallback(c,connectCallback);
	redisAsyncSetDisconnectCallback(c,disconnectCallback);
	int i = 0;
	while (++i<MAX_LOOP) {
		redisAsyncCommand(c, NULL, NULL, "SET key 123");
	}
	timer.end();
	printf("同步写入%d条:%d\n", MAX_LOOP, timer.costms() );

	return NULL;
}

int multiTreadTest()
{
	const int THREAD_CNT = 32;
	pthread_t threadArray[THREAD_CNT];
	for (int i = 0; i<THREAD_CNT;++i) {
		pthread_create(&threadArray[i], NULL, aeFunction, NULL);
	}
	for (int i = 0; i<THREAD_CNT; ++i) {
		pthread_join(threadArray[i], NULL);
	}
	return 0;
}

void clientFunc(bool bAsyn)
{
	char sCmd[1024];
	int num = 0;
	RedisRWSplitClient client;
	client.initReadLayer("127.0.0.1", 6379);
	if (bAsyn) {
		//client.initWriteLayer("127.0.0.1",6379);
		//client.setWriteCallback(NULL);
		//client.eventRun();
	}

	dis_timer timer;

	timer.start();
	int i = 0;
	while(++i < MAX_LOOP){
		if (bAsyn) {
			client.cmdWriteLayer("SET 123 abc");
		}else{
			client.cmdReadLayer("SET 123 abc");
		}
	}
	timer.end();
	printf("写入10000条:%d\n", timer.costms() );
}

int main (int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);

//	aeFunction();
	multiTreadTest();
	//clientFunc(false);
	//bearedFunc();
//	clientFunc(true);
	while (true) {
		sleep(1);
	}
	printf("DONE\n");
	aeMain(loop);
	assert(1);
	//pthread_join(th, NULL);
    return 0;
}

