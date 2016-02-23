/*
 * Copyright (c) 2013, William W.L. Chuang
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Pyraemon Studio nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _THE_UV_SOCKET_EXAMPLE_CORE_H_INCLUDED
#define _THE_UV_SOCKET_EXAMPLE_CORE_H_INCLUDED


#include <uv.h>
//#include "queue.h"
#include <glib.h>

#define MSG "msg"
#define CONN  "conn"
#define KEEP    "keep"
#define UUID   "uuid"
#define PWD     "pwd"
#define LOGIN   "login"
#define CMD     "cmd"
#define ADDR    "addr"
#define AID     "aid"
#define HOST     "host"
#define ERR      "err"



static char SUFIX[]={0xd,0xa,0xd,0xa};

 static  char unkown_cmd[] = {0x0,0x19,0x7b,0x22,0x65,0x72,0x72,0x22,0x3a,0x20,0x22,0x75,0x6e,0x6b,0x6f,0x77,0x6e,0x20,0x63,0x6f,0x6d,0x6d,0x61,0x6e,0x64,0x22,0x7d,0x0d,0x0a,0x0d,0x0a,
};  //  {'err':'unkown command'}

 static char unkown_format[] = {0x0,0x30,0x7b,0x22,0x65,0x72,0x72,0x22,0x3a,0x20,0x22,0x75,0x6e,0x6b,0x6f,0x77,0x6e,0x20,0x66,0x6f,0x72,0x6d,0x61,0x74,0x22,0x7d,0x0d,0x0a,0x0d,0x0a,
};


static char msg_ok[] = {0x0,0xf,0x7b,0x22,0x6d,0x73,0x67,0x22,0x3a,0x20,0x22,0x6f,0x6b,0x22,0x7d,0xd,0xa,0xd,0xa};



struct server;


struct client
{
	//uv_loop_t* loop;
	struct server* server;
	uv_tcp_t   socket;
	char  buffer[8192];
	char  *uuid;  /*uuid 做为一个用户名*/
	char  *pwd;      /*uuid 访问密码*/
};

typedef enum  {
    APP_SRV=0,
    DEV_SRV=1
}srv_type;

struct server
{
	uv_loop_t*  loop;

	uv_tcp_t    socket;
	uv_timer_t  watchdog;

	uv_signal_t sighup;
	uv_signal_t sigint;
	uv_signal_t sigkill;
	uv_signal_t sigterm;
    GHashTable *c_gtable; /* 所有的客户端hash表 */
   // int  client_type; /* 1 == dev,0 = app */
    srv_type stype;
    struct server* part_server; /*另一个服务器的指针*/
};


typedef struct __ThreadObj_t
{
    uv_thread_t thread_;
    GHashTable *app_;
    GHashTable *dev_;
    int sync_flag_;
}T_THREADOBJ;

typedef struct __ThreadArg_t
{
    uv_loop_t* loop_;
    struct server* server_;
    int sync_flag_;
}T_THREADARG;




int  server_initialize(uv_loop_t* loop, struct server* server);
void server_destroy(struct server* server);
int server_start(struct server* server,struct sockaddr_in *addr );
void server_detach(struct server* server, struct client* client);

int  client_initialize(struct client* client, struct server* server);
void client_destroy(struct client* client);
void client_read(struct client* client);
void client_disconnect(struct client* client, uv_close_cb callback);
//void client_close(struct client* client);


#endif /* _THE_UV_SOCKET_EXAMPLE_CORE_H_INCLUDED */
