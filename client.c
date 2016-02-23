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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"

#include "json-c/json.h"
extern GHashTable *app_table;
extern GHashTable *dev_table;



int client_initialize(struct client* client, struct server* server)
{
    memset(client, 0, sizeof(*client));

    client->server = server;
    client->uuid = NULL;
    client->pwd = NULL;

    uv_tcp_init(server->loop, &client->socket);
    client->socket.data = client;

    return 0;
}

void client_destroy(struct client* client)
{

    if (client == NULL) return;
    if(client->uuid)
    {
        free(client->uuid);
    }
    if(client->pwd)
    {
        free(client->pwd);
    }
    client->server = NULL;
    client->socket.data = NULL;

    free(client);

    //if (client->command != NULL) free(client->command);
}

static void _on_alloc_buffer(uv_handle_t* handle, size_t suggested_size,uv_buf_t* buf)
{
    struct client* client;

    client = handle->data;

    memset(client->buffer, 0, sizeof(client->buffer));
   // uv_buf_t t = { NULL, 0 };
  //  t =  uv_buf_init(client->buffer, sizeof(client->buffer));
    buf->base = client->buffer;
    buf->len = sizeof(client->buffer);
}

static void _on_disconnect(uv_handle_t* handle)
{
    // TODO: This is a dummy example

    struct client* client = handle->data;
    client_destroy(client);

}

void print_key_value(gpointer key,gpointer value,gpointer user_data)
{
    printf("hash item \"%s\"---->%08x\n",key,value);
}



static void dev_on_read(uv_stream_t* stream, ssize_t nread, uv_buf_t *buf)
{
    struct client* client;

    client = stream->data;

    if (nread == 0)
    {
        return;
    }
    if (nread < 0)
    {
        fprintf(stderr, "DEV The client disconnected\n");
        server_detach(client->server, client);
        client_disconnect(client, _on_disconnect);
        return;
    }
  //  fprintf(stderr, "DEV Client read: %s\t,strlen %d\n", &buf->base[2],
   //         strlen(&buf->base[2]));
    //parser_to_json(buf->base);

    /*分析jso段*/
    struct json_object * jobj = json_tokener_parse(&(buf->base[2]));
    if(!jobj)
    {
        printf("dev read wrong data!\n");
        server_detach(client->server, client);
        client_disconnect(client, _on_disconnect);
        return;

    }
    // printf("dev read json obj.to_string() = %s\n",
    //       json_object_to_json_string(jobj));

    struct json_object* obj = json_object_object_get(jobj,CMD);
    const char *cmd = json_object_get_string(obj);

//    json_object_put(obj);
    uv_write_t wr;

    uv_buf_t sbuf = { NULL, 0 };
    if (!cmd)
    {
        sbuf = uv_buf_init((char*)unkown_format,sizeof(unkown_cmd));
        uv_write(&wr,stream,&sbuf,1,NULL);
        /*未定义的key,应该断开本连接 */

        server_detach(client->server, client);
        client_disconnect(client, _on_disconnect);

        return ;
    }

   // printf("dev get command %s\n",cmd);
    if (!strcmp(cmd,LOGIN))
    {
        /*登录命令*/
        //  struct json_object* uobj = json_object_object_get(jobj,"uuid");

        const char *tuuid = json_object_get_string(json_object_object_get(jobj,UUID));
        int tlen = strlen(tuuid);
        client->uuid = (char*)malloc(tlen+1);

        memcpy(client->uuid,tuuid,tlen);
        client->uuid[tlen]='\0';
        // struct json_object* pobj = json_object_object_get(jobj,"pwd");
        const char *tpwd =json_object_get_string(json_object_object_get(jobj,PWD));
        tlen=strlen(tpwd);
        client->pwd = malloc(tlen+1);
        client->pwd[tlen] = '\0';
        memcpy(client->pwd,tpwd,tlen);
        /* 添加本客户端到链表里*/
        //g_hash_table_insert(server->c_gtable,stream->u.fd ,(gpointer)client);

      //  printf(" srv type %d\n",client->server->stype);
        //g_hash_table_insert(client->server->c_gtable,
        //                    (gpointer)client->uuid,(gpointer)client);
        g_hash_table_insert(dev_table,
                            (gpointer)client->uuid,(gpointer)client);
        //  printf("dev hash_table size %d\n",g_hash_table_size(dev_table));
        // g_hash_table_foreach(dev_table,print_key_value,NULL);

        sbuf = uv_buf_init(&msg_ok,sizeof(msg_ok));
        uv_write(&wr,stream,&sbuf,1,NULL);
    }
    else if(!strcmp(cmd,KEEP))
    {
        /*心跳包*/
        uv_write(&wr,stream,buf,1,NULL);
    }
    else if(!strcmp(cmd,CONN))
    {

        /*连接请求*/

        gpointer fd = json_object_get_int(json_object_object_get(jobj,AID));
       // printf("get aid is %d\n",fd);
        struct client *appclient =g_hash_table_lookup(app_table,&fd);

        if (appclient)
        {
          //  printf(" I got some one connect request ");
            struct json_object* connjson =   json_object_new_object();
            json_object_object_add(connjson,ADDR,json_object_object_get(jobj,ADDR));
            json_object_object_add(connjson,UUID,json_object_object_get(jobj,UUID));

            const char *jdata = json_object_to_json_string(connjson);
            int connlen = strlen(jdata)+2;
            char *connbuf = (char *)malloc(connlen+4+1);
            connbuf[0] = connlen & 0xff00;
            connbuf[1] = connlen & 0xff;
            memcpy(&connbuf[2],jdata,connlen-2);
            memcpy(&connbuf[connlen],(char*)SUFIX,4);
            connbuf[connlen+4] = '\0';


            sbuf = uv_buf_init(connbuf,connlen+4);

            uv_write(&wr,(uv_stream_t*)&appclient->socket,&sbuf,1,NULL);
            json_object_put(connjson);
            free(connbuf);
            printf("app table size %d\n",g_hash_table_size(app_table));
            server_detach(appclient->server, appclient);
            client_disconnect(appclient, _on_disconnect);

            //g_hash_table_remove(app_table,&fd);

        }

    }
    else
    {
        /*未知命令*/
        sbuf = uv_buf_init((char*)unkown_cmd,sizeof(unkown_cmd));
        int r = uv_write(&wr,stream,&sbuf,1,NULL);
    }
    json_object_put(jobj); // 清理内存
}




static void app_on_read(uv_stream_t* stream, ssize_t nread, uv_buf_t *buf)
{
    struct client* client;
    client = stream->data;
    if (nread == 0)
    {
        return;
    }
    if (nread < 0 )
    {
        // fprintf(stderr, "APP The client disconnected,recv %s \t, str len %d\n",
        //         buf->base,strlen(buf->base));
        server_detach(client->server, client);
        client_disconnect(client, _on_disconnect);
        return;
    }

    uv_buf_t sbuf = { NULL, 0 };
  //  fprintf(stderr, "APP Client read: %s\t ,len %d\n", &buf->base[2],
   //         strlen(&buf->base[2]));
    //parser_to_json(buf->base);
    /*分析jso段*/
    struct json_object * jobj = json_tokener_parse(&(buf->base[2]));
    if (!jobj)
    {
        //  printf("app recv json obj.to_string() = %s\n",
        server_detach(client->server, client);
        client_disconnect(client, _on_disconnect);
       // printf(" app recv wrong format data\n");
        return;
    }
    uv_write_t wr;

    //printf("uuid is %s\n",clinet->uuid);
    struct json_object* cmdobj = json_object_object_get(jobj,CMD);
    char *cmd = json_object_get_string(cmdobj);
    if (!cmd)
    {
      //  printf("recv unkown cmd, %s\n",buf->base);


        sbuf = uv_buf_init((char*)unkown_cmd,sizeof(unkown_cmd));
        int r = uv_write(&wr,stream,&sbuf,1,NULL);
        server_detach(client->server, client);
        client_disconnect(client, _on_disconnect);
        /*未定义的key,应该断开本连接 */
        return ;
    }
    if (!strcmp(cmd,CONN))
    {
        /*登录命令*/

        struct json_object* uuidjson =  json_object_object_get(jobj,UUID);
        const char *uvalue = json_object_get_string(uuidjson);
        // struct json_object* pobj = json_object_object_get(jobj,"pwd");
        struct json_object* pwdjson =  json_object_object_get(jobj,PWD);
        const char *pvalue = json_object_get_string(pwdjson);
        /*struct client* devclient = g_hash_table_lookup(client->server->part_server->c_gtable,
                                   (gpointer)uvalue);
                                   */

        struct client* devclient = g_hash_table_lookup(dev_table,
                                   (gpointer)uvalue);
      //  g_hash_table_foreach(dev_table,print_key_value,NULL);

        if ((devclient != NULL) && !strcmp(pvalue,
                                           devclient->pwd))
        {
          //  printf("app login ok\n");
            /*app 端认证成功*/
            /* 添加本客户端到链表里*/
            //g_hash_table_insert(server->c_gtable,stream->u.fd ,(gpointer)client);
            g_hash_table_insert(app_table,
                                &client->socket.io_watcher.fd ,(gpointer)client);
            /* 发送请求到dev 客户端*/

            /* {"uuid": <varable len>,"pwd":<varable len> ,"msg":"conn","aid":"0xFFFFFFFF"} */

            struct json_object* connjson =   json_object_new_object();
            json_object_object_add(connjson,UUID,uuidjson);
            json_object_object_add(connjson,PWD,pwdjson);
            json_object_object_add(connjson,MSG,json_object_new_string(CONN));
            json_object_object_add(connjson,ADDR,json_object_object_get(jobj,ADDR));
            json_object_object_add(connjson,AID,json_object_new_int(client->socket.io_watcher.fd));
            // printf("get json string len %d\n", json_object_get_string_len(connjson));
            //printf("json string data %s\n",json_object_to_json_string(connjson));
            const char *jdata = json_object_to_json_string(connjson);
            int connlen = strlen(jdata)+2;
            char *connbuf = (char *)malloc(connlen+4+1);
            connbuf[0] = connlen & 0xff00;
            connbuf[1] = connlen & 0xff;
            memcpy(&connbuf[2],jdata,connlen-2);
            memcpy(&connbuf[connlen],(char*)SUFIX,4);
            connbuf[connlen+4]='\0';

            sbuf = uv_buf_init(connbuf,connlen+4);
            uv_write(&wr,(uv_stream_t*)&devclient->socket,&sbuf,1,NULL);
            //  printf("conn data is %s\n",sbuf.base);
          //  printf("app auth ok send msg to dev\n");
            free(connbuf);
            json_object_put(connjson);

        }

    }
    else
    {
        /*未知命令*/


        sbuf = uv_buf_init((char*)unkown_cmd,strlen(unkown_cmd));
        int r = uv_write(&wr,stream,&sbuf,1,NULL);


    }
    json_object_put(jobj);


}

void client_read(struct client* client)
{

    uv_read_start((uv_stream_t*)&client->socket, _on_alloc_buffer,
                  client->server->stype ? dev_on_read : app_on_read);
}

void client_disconnect(struct client* client, uv_close_cb callback)
{
    uv_close((uv_handle_t*)&client->socket, callback);
}


