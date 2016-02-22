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
#include <unistd.h>

#include "core.h"



 static struct server dev_server;
 static  struct server app_server;

int get_cpu_count()
{

    uv_cpu_info_t *info;
    int cpu_count;
    uv_cpu_info(&info,&cpu_count);
    uv_free_cpu_info(info,cpu_count);
    printf("this host has %d core\n",cpu_count);
    return cpu_count;
}


static void srv_thread_entry(void *arg)
{
    T_THREADARG* t = (T_THREADARG*)arg;
    uv_run(t->loop_, UV_RUN_DEFAULT);
       g_hash_table_remove_all(t->server_->c_gtable);
    g_hash_table_destroy(t->server_->c_gtable);
    server_destroy(t->server_);
    uv_stop(t->loop_);
    uv_loop_delete(t->loop_);
    free(t->loop_);
}


int main(int argc, char** argv)
{


	uv_loop_t* dev_loop;
	uv_loop_t* app_loop;


	dev_loop = uv_default_loop();
	app_loop = uv_default_loop();


    struct sockaddr_in dev_addr;
	uv_ip4_addr("0.0.0.0", 5560,&dev_addr);

	struct sockaddr_in app_addr;
	uv_ip4_addr("0.0.0.0", 5561,&app_addr);


	server_initialize(dev_loop, &dev_server);
	server_initialize(app_loop, &app_server);
	dev_server.stype = DEV_SRV;
	app_server.stype = APP_SRV;
    dev_server.c_gtable = g_hash_table_new(g_str_hash,g_str_equal);
    app_server.c_gtable = g_hash_table_new(g_int_hash,g_int_equal);

	dev_server.part_server = &app_server;
	server_start(&dev_server,&dev_addr);


	app_server.part_server = &dev_server;
	server_start(&app_server,&app_addr);


    T_THREADARG app_arg;
    app_arg.loop_ = app_loop;
    app_arg.server_ = &app_server;
    app_arg.sync_flag_ = 1;
    uv_thread_t app_th;
    if(uv_thread_create(&app_th,srv_thread_entry,&app_arg))
    {
        printf("create app thread failure");
        return -1;
    }



    T_THREADARG dev_arg;
    dev_arg.loop_ = dev_loop;
    dev_arg.server_ = &dev_server;
    dev_arg.sync_flag_ = 1;
    uv_thread_t dev_th;
    if(uv_thread_create(&dev_th,srv_thread_entry,&dev_arg))
    {
        printf("create app thread failure");
        return -1;
    }
    uv_thread_join(&dev_th);
    uv_thread_join(&app_th);

/*
    g_hash_table_remove_all(dev_server.c_gtable);
    g_hash_table_destroy(dev_server.c_gtable);
    g_hash_table_remove_all(app_server.c_gtable);
    g_hash_table_destroy(app_server.c_gtable);

*/




    /*
	pid_t devPID;
	pid_t appPID;
	devPID = fork();
	if(devPID >= 0 )
    {
        fprintf(stdout, "start dev camera server");

        uv_run(dev_loop, UV_RUN_DEFAULT);
        server_destroy(&dev_server);
    }
    else{
        printf("dev loop fork failed");
        server_destroy(&dev_server);
    }

    appPID = fork();
    if(appPID >=0 )
    {
        fprintf(stdout, "start app server");

        uv_run(app_loop, UV_RUN_DEFAULT);
        server_destroy(&app_server);
    }else{
        printf("app loop fork failed");
        server_destroy(&app_server);

    }
    */



	return 0;
}
