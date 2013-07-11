/***************************************************************************
 * 
 * Copyright (c) 2013 izptec.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file include/redis_rw_split_client.h
 * @author YAO LU(luyao@izptec.com)
 * @date 2013/07/08 13:55:05
 * @version 1.0 
 * @brief 
 *  
 **/


#ifndef  __INCLUDE_REDIS_RW_SPLIT_CLIENT_H_
#define  __INCLUDE_REDIS_RW_SPLIT_CLIENT_H_

#include <stdlib.h>
typedef unsigned short uint16_t;
typedef void (*functionHandle)(struct redisAsyncContext*, void*, void*);
typedef void (*connectHandle)(const redisAsyncContext*, int);

///<该类的设计初衷如下：
//实现redis的读写分离，由于redis在使用的时候，如果
//是读取操作，则一般都是同步顺序操作；
//而redis的更新操作，在实时性要求不高的情况下，可以是
//异步的。
//该类非线程安全, 若在多线程生产环境下使用，需要每个线程
//都有自己独立的client对象
//usage:
//new RedisRWSplitClient client;
//assert(client != NULL);
//client->setConnectCallback(conectCallback);   //成功连接的回调函数
//client->setDisconnectCallback(conectCallback);//断开连接的回到函数
//client->setWriteCallback(conectCallback);     //写入的回调函数
//int ret = client->initReadLayer("host", 123); //初始化server的地址（写入层）
//ret +=    client->initWriteLayer("host",123); //初始化server的地址（读取层）
//if (ret) {
//    return something error;
//}
//replay = client->cmdReadLayer();
//ret    = client->cmdWriteLayer();
//delete client;
//
class RedisRWSplitClient
{
public:

	/**
	 * @brief Default Client constructor, must use initxxx Functions.
	 *
	 * @author luyao
	 * @date 2013/07/08 14:40:31
	**/
	RedisRWSplitClient();


	/**
	 * @brief this interface will use same server as 
	 * read layer and write layer
	 *
	 * @param [in] sHost   : const char*
	 * @param [in] nPort   : uint16_t
	 * @author luyao
	 * @date 2013/07/08 14:39:41
	**/
	RedisRWSplitClient(const char *sHost, uint16_t nPort);

	/**
	 * @brief this interface is uesed to init the write layer
	 * and the read layer in the same time. U can use the same 
	 * server as the read layer and write layer, if U want( just
	 * use the same param). 
	 *
	 * @param [in] sReadHost   : const char*
	 * @param [in] nReadPort   : uint16_t
	 * @param [in] sWriteHost  : const char*
	 * @param [in] nWritePort  : uint16_t
	 * @author luyao
	 * @date 2013/07/08 14:41:36
	**/
	RedisRWSplitClient(const char *sReadHost,  uint16_t nReadPort,
					   const char *sWriteHost, uint16_t nWritePort);

	~RedisRWSplitClient();

	bool isConnect()const;

	int initReadLayer (const char *sHost, uint16_t nPort);

	int initWriteLayer(const char *sHost, uint16_t nPort);

	struct redisReply* cmdReadLayer (const char *sFormat, ...);

	int				   cmdWriteLayer(const char *sFormat, ...);

	int setConnectCallback(connectHandle pF = NULL);

	int setDisconnectCallback(connectHandle pF = NULL);

	int setWriteCallback(functionHandle pF = NULL);

	void eventRun();
private:
	RedisRWSplitClient(const RedisRWSplitClient&);
	RedisRWSplitClient operator=(const RedisRWSplitClient&);

	class RedisRWSplitClientImp* _pHandle;
};















#endif  //__INCLUDE/REDIS_RW_SPLIT_CLIENT_H_

/* vim: set ts=4 sw=4 sts=4 tw=100 noet: */
