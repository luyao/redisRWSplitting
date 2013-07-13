#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <iostream>
#include "hiredis.h"
#include "async.h"
#include "adapters/libevent.h"
#include "redis_rw_split_client.h"
#include "adapters/ae.h"
using namespace std;

#ifdef CLIENT
#else
static aeEventLoop *loop;
#endif

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

const int MAX_LOOP = 10000;
const int THREAD_CNT = 32;
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
//
void* __start__(void*p)
{
	//RedisRWSplitClient::eventRun();
	aeProcessEvents(loop, AE_ALL_EVENTS);
	return NULL;
}

void* aeFunctionClient(void *param)
{
	dis_timer timer;
	timer.start();
	RedisRWSplitClient client("127.0.0.1", 6379);
	RedisRWSplitClient::eventRun();

	int i = 0;
	char sCmd[128];
	while (++i<MAX_LOOP) {
		snprintf(sCmd, 128, "SET key_%d %d", i, i);
		client.cmdWriteLayer(sCmd);
	}
	timer.end();
	printf("写入%d条:%d\n", MAX_LOOP, timer.costms() );

	while (1) {
		sleep(1);
	}
	return NULL;
}

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void* aeFunction(void *param)
{
	dis_timer timer;
	timer.start();
	//aeEventLoop *loop = NULL;
	//loop = aeCreateEventLoop(1024);
	redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
	if (c->err) {
		/* Let *c leak for now... */
		printf("Error: %s\n", c->errstr);
		return NULL;
	}   

	redisAeAttach(loop, c); 
	redisAsyncSetConnectCallback(c,connectCallback);
	redisAsyncSetDisconnectCallback(c,disconnectCallback);
	int i = 0;
	char sCmd[128];
	while (++i<MAX_LOOP) {
		snprintf(sCmd, 128, "SET %d_%d %d", (int)param, i, i);
		redisAsyncCommand(c, NULL, NULL, sCmd);
	}
	timer.end();
	printf("threads %d 写入%d条:%d\n", (int)param, MAX_LOOP, timer.costms() );
	while (true) {
		sleep(1);
	}
	//if (1 || !loop->stop) {
	//	if (loop->beforesleep != NULL)
	//		loop->beforesleep(loop);
	//	aeProcessEvents(loop, AE_ALL_EVENTS);
	//}
	//aeMain(loop);

//	aeEventLoop *eventLoop = loop;
//	while ( 1 ) {
//		if (pthread_mutex_trylock(&lock)) {
//			return 0;
//		}
//		cout<<"inner the loop"<<endl;
//event_loop:
//		eventLoop->stop = 0;
//		while (!eventLoop->stop) {
//			if (eventLoop->beforesleep != NULL)
//				eventLoop->beforesleep(eventLoop);
//			aeProcessEvents(eventLoop, AE_ALL_EVENTS);
//		}  
//		goto event_loop;
//		//never give the lock
//		//pthread_mutex_unlock(&lock);
//	}
	cout<<"REACH THE END?"<<endl;
	return NULL;
}


int multiTreadTest()
{
	pthread_t threadArray[THREAD_CNT];
	for (int i = 0; i<THREAD_CNT;++i) {
#ifdef CLIENT
		pthread_create(&threadArray[i], NULL, aeFunctionClient, NULL);
#else
		pthread_create(&threadArray[i], NULL, aeFunction, i);
#endif
	}

	for (int i = 0; i<THREAD_CNT; ++i) {
		pthread_join(threadArray[i], NULL);
	}
	//pthread_t th;
	//pthread_create(&th, NULL, __start__, loop);
	//pthread_join(th, NULL);
	return 0;
}

void clientFunc(bool bAsyn)
{
	char sCmd[1024];
	int num = 0;
	RedisRWSplitClient client;
	client.initReadLayer("127.0.0.1", 6379);
	if (bAsyn) {
		client.initWriteLayer("127.0.0.1",6379);
		client.setWriteCallback(NULL);
		client.eventRun();
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

int main (int argc, char **argv) 
{
    signal(SIGPIPE, SIG_IGN);
	loop = aeCreateEventLoop(8192);
	if (!loop) {
		cout<<"NULL LOOP POINTER"<<endl;
		return 0;
	}
	pthread_t th;
	//pthread_create(&th, NULL, __start__, NULL);
	pthread_create(&th, NULL, __start__, NULL);
	multiTreadTest();
	pthread_join(th, NULL);
	//aeMain(loop);
	//pthread_join(th, NULL);
	
	//pthread_t th;
	//pthread_create(&th, NULL, __start__, loop);
	cout<<"DONE!"<<endl;
	while (true) {
		sleep(1);
	}
	//pthread_join(th, NULL);
	return 0;
}

//{
//#ifdef CLIENT
//	//pthread_create(&th, NULL, __start__, NULL);
//#else
//	loop = aeCreateEventLoop(8192);
//	//pthread_create(&th, NULL, __start__, loop);
//#endif
////	aeFunction();
//	multiTreadTest();
//	//clientFunc(false);
//	//bearedFunc();
////	clientFunc(true);
//	aeMain(loop);
//#ifdef CLIENT
//	pthread_create(&th, NULL, __start__, NULL);
//#else
//	//pthread_create(&th, NULL, __start__, loop);
//#endif
//	//RedisRWSplitClient::eventRun();
//	printf("DONE\n");
//	while (true) {
//		sleep(1);
//	}
//	//pthread_join(th, NULL);
//
//    return 0;
//}
//
