/***************************************************************************
 * 
 * Copyright (c) 2013 izptec.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file src/redis_rw_split_client_imp.cpp
 * @author YAO LU(luyao@izptec.com)
 * @date 2013/07/08 14:05:18
 * @version 1.0 
 * @brief 
 *  
 **/

#include "redis_rw_split_client.h"
#include <string>
#include <iostream>
#include "adapters/libevent.h" 
#include "adapters/ae.h"

using namespace std;

class RedisRWSplitClientImp
{
public:
	RedisRWSplitClientImp();
	RedisRWSplitClientImp(const char *sHost,  uint16_t nPort);
	RedisRWSplitClientImp(const char *sHost1, uint16_t nPort1,
			              const char *sHost2, uint16_t nPort2);

	~RedisRWSplitClientImp();

	int initReadLayer (const char *sHost, uint16_t nPort);

	int initWriteLayer(const char *sHost, uint16_t nPort);

	struct redisReply* cmdReadLayer (const char *sCmd);
	int				   cmdWriteLayer(const char *sCmd);

	bool isConnect()const{return _bConnect;}

	int setConnectCallback(connectHandle    pF = NULL);
	int setDisconnectCallback(connectHandle pF = NULL);
	int setWriteCallback(functionHandle     pF = NULL);

	static int eventRun();
private:
	void _reconnectCallback(const redisAsyncContext *c, int status);
	static aeEventLoop* getEventHandle();
	static void writeCallback(void *pParam);
	RedisRWSplitClientImp(const RedisRWSplitClientImp&);
	RedisRWSplitClientImp operator=(const RedisRWSplitClientImp&);
	void _getContext(bool bAsyn);

	std::string _sReadHost;
	std::string _sWriteHost;
	uint16_t    _nReadPort;
	uint16_t    _nWritePort;
	bool        _bConnect;
	bool        _bEventRun;
	connectHandle  _pConnect;
	connectHandle  _pDisconnect;
	functionHandle _pWriteHandle;

	struct redisAsyncContext *_pAsynContext;
	struct redisContext      *_pContext;
	static aeEventLoop       *_pLoop;
	static pthread_t         _th;
};

void RedisRWSplitClientImp::_reconnectCallback(const redisAsyncContext *c, int status)
{	
	if (status != REDIS_OK) {
		printf("Error: %s\n", c->errstr);
		//_pAsynContext;
	}   
	printf("Disconnected...\n");
}

aeEventLoop* RedisRWSplitClientImp::_pLoop = aeCreateEventLoop(8192);

pthread_t RedisRWSplitClientImp::_th;

aeEventLoop* RedisRWSplitClientImp::getEventHandle()
{
	return _pLoop;
}

RedisRWSplitClientImp::RedisRWSplitClientImp():
	_sReadHost(), _sWriteHost(), _nReadPort(0), _nWritePort(0),
	_bConnect(false), _bEventRun(false), _pConnect(NULL), _pDisconnect(NULL),
	_pWriteHandle(NULL), _pAsynContext(NULL), _pContext(NULL)
	{}

void _connectCallback(const redisAsyncContext *c, int status) {
	if (status != REDIS_OK) {
		printf("Error: %s\n", c->errstr);
		return;
	}   
	printf("Connected...\n");
}

void _disconnectCallback(const redisAsyncContext *c, int status) {
	if (status != REDIS_OK) {
		printf("Error: %s\n", c->errstr);
		return;
	}   
	cout<<"Disconnected..."<<endl;
}

void _getCallback(redisAsyncContext *c, void *r, void *privdata) {
    redisReply *reply = (redisReply*)r;
    if (reply == NULL);
    printf("argv[%s]: %s\n", (char*)privdata, reply->str);
}

void RedisRWSplitClientImp::_getContext(bool bAsyn)
{
	if (bAsyn) {
		_pAsynContext = redisAsyncConnect(_sWriteHost.c_str(), _nWritePort);
		if (_pAsynContext->err) {
			/* Let *c leak for now... */
			redisAsyncFree(_pAsynContext);
			_pAsynContext = NULL;
		}
		redisAeAttach(_pLoop, _pAsynContext);
		redisAsyncSetConnectCallback(_pAsynContext, _connectCallback);
		redisAsyncSetDisconnectCallback(_pAsynContext, _disconnectCallback);
	}else{
		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		_pContext = redisConnectWithTimeout(_sReadHost.c_str(), _nReadPort, timeout);
	}
}

int RedisRWSplitClientImp::initReadLayer(const char *sHost, uint16_t nPort)
{
	_sReadHost = sHost;
	_nReadPort = nPort;
	_getContext(false);
	if (_pContext) {
		return 0;
	}
	return -1;
}


int RedisRWSplitClientImp::initWriteLayer(const char *sHost, uint16_t nPort)
{
	_sWriteHost = sHost;
	_nWritePort = nPort;
	_getContext(true);
	if (_pAsynContext) {
		return 0;
	}
	return -1;
}

RedisRWSplitClientImp::~RedisRWSplitClientImp()
{
	cout<<"De-construct"<<endl;
	if (_pAsynContext) {
//		redisAsyncDisconnect(_pAsynContext);
//		redisAsyncFree(_pAsynContext);
//		_pAsynContext = NULL;
	}
	if (_pContext) {
		redisFree(_pContext);
		_pContext = NULL;
	}
}

RedisRWSplitClientImp::RedisRWSplitClientImp(const char *sHost, uint16_t nPort):
	_sReadHost(sHost), _sWriteHost(sHost), _nReadPort(nPort), _nWritePort(nPort),
	_bConnect(false), _bEventRun(false), _pConnect(NULL), _pDisconnect(NULL),
	_pWriteHandle(NULL), _pAsynContext(NULL), _pContext(NULL)
{
	//already set the host and port,
	//we can initialize the redis handle, 4real
	_bConnect = true;
	_getContext(false);	
	if (!_pContext) {
		_bConnect = false;
	}
	_getContext(true);
	if (!_pAsynContext) {
		_bConnect = false;
	}
}

RedisRWSplitClientImp::RedisRWSplitClientImp(const char *sHost, uint16_t nPort,
		const char *sHost2, uint16_t nPort2):_sReadHost(sHost), _sWriteHost(sHost2), _nReadPort(nPort), _nWritePort(nPort2),
	_bConnect(false), _bEventRun(false), _pConnect(NULL), _pDisconnect(NULL),
	_pWriteHandle(NULL) , _pAsynContext(NULL), _pContext(NULL)

{
	//already set the host and port,
	//we can initialize the redis handle, 4real
	_getContext(false);	
	if (!_pContext) {
		_bConnect = false;
	}
	_getContext(true);
	if (!_pAsynContext) {
		_bConnect = false;
	}
}

static void* __start__(void *p)
{
	aeMain((aeEventLoop*)p);
	return NULL;
}

int RedisRWSplitClientImp::eventRun()
{
	if (_pLoop) {
		//pthread_create(&_th, NULL, __start__, _pLoop);
		aeMain(_pLoop);
		return 0;
	}
	return -1;
}

int RedisRWSplitClient::eventRun()
{
	return RedisRWSplitClientImp::eventRun();
}

struct redisReply* RedisRWSplitClientImp::cmdReadLayer(const char *sFormat)
{
	if (!_pContext) {
		//TODO: retry
		return NULL;
	}
	return (redisReply*)redisCommand(_pContext, sFormat);
}


int RedisRWSplitClientImp::cmdWriteLayer(const char *sFormat)
{
	if (!_pAsynContext) {
		//TODO: retry
		return -1;
	}
	return redisAsyncCommand(_pAsynContext, _pWriteHandle, NULL, "%s", sFormat);
}

int RedisRWSplitClientImp::setConnectCallback(connectHandle h)
{
	if (!_pAsynContext) {
		return -1;
	}
	return redisAsyncSetConnectCallback(_pAsynContext, h);
}

int RedisRWSplitClientImp::setDisconnectCallback(connectHandle h)
{
	if (!_pAsynContext) {
		return -1;
	}
	return redisAsyncSetDisconnectCallback(_pAsynContext, h);
}

int RedisRWSplitClientImp::setWriteCallback(functionHandle h)
{
	if (!_pAsynContext) {
		return -1;
	}
	_pWriteHandle = h;
	return 0;
}

//interface class
RedisRWSplitClient::RedisRWSplitClient():_pHandle(NULL)
{
	_pHandle = new (std::nothrow) RedisRWSplitClientImp;
}

RedisRWSplitClient::RedisRWSplitClient(const char *sHost, uint16_t nPort):_pHandle(NULL)
{
	_pHandle = new (std::nothrow) RedisRWSplitClientImp(sHost, nPort);
}

RedisRWSplitClient::RedisRWSplitClient(const char *sHost1, uint16_t nPort1, 
		const char *sHost2, uint16_t nPort2):_pHandle(NULL)
{
	_pHandle = new (std::nothrow) RedisRWSplitClientImp(sHost1, nPort1, sHost2, nPort2);
}

bool RedisRWSplitClient::isConnect()const{
	return _pHandle &&  _pHandle->isConnect();
}


RedisRWSplitClient::~RedisRWSplitClient(){
	if (_pHandle) {
		delete _pHandle;
		_pHandle = NULL;
	}
}

int RedisRWSplitClient::initReadLayer(const char *sHost, uint16_t nPort)
{
	if ( !_pHandle ) {
		return -1;
	}
	return _pHandle->initReadLayer(sHost, nPort);
}

int RedisRWSplitClient::initWriteLayer(const char *sHost, uint16_t nPort)
{
	if ( !_pHandle ) {
		return -1;
	}
	return _pHandle->initWriteLayer(sHost, nPort);
}

struct redisReply* RedisRWSplitClient::cmdReadLayer(const char *sFormat, ...)
{
	if (!_pHandle) {
		return NULL;
	}
	va_list ap;
	char sCmd[1024];
	va_start(ap, sFormat);
	vsnprintf(sCmd, sizeof(sCmd), sFormat, ap);
	redisReply* reply = _pHandle->cmdReadLayer(sCmd);
	va_end(ap);
	return reply;
}

int RedisRWSplitClient::cmdWriteLayer(const char *sFormat, ...)
{
	if (!_pHandle) {
		return -1;
	}
	va_list ap;
	char sCmd[1024];
	va_start(ap, sFormat);
	vsnprintf(sCmd, sizeof(sCmd), sFormat, ap);
	va_end(ap);

	return  _pHandle->cmdWriteLayer(sCmd);
}


int RedisRWSplitClient::setConnectCallback(connectHandle h)
{
	if (!_pHandle) {
		return -1;
	}
	return _pHandle->setConnectCallback(h);
}

int RedisRWSplitClient::setDisconnectCallback(connectHandle h)
{
	if (!_pHandle) {
		return -1;
	}
	return _pHandle->setDisconnectCallback(h);
}

int RedisRWSplitClient::setWriteCallback(functionHandle h)
{
	if (!_pHandle) {
		return -1;
	}
	return _pHandle->setWriteCallback(h);
}


/* vim: set ts=4 sw=4 sts=4 tw=100 noet: */
