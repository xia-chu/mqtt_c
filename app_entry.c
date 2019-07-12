/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "jimi_log.h"
#include "jimi_memory.h"

#include "aos/kernel.h"
#include "ulog/ulog.h"
#include "aos/yloop.h"

#include "netmgr.h"
#include "app_entry.h"

#ifdef AOS_ATCMD
#include <atparser.h>
#endif
#ifdef CSP_LINUXHOST
#include <signal.h>
#endif

static char linkkit_started = 0;

static app_main_paras_t entry_paras;

typedef void (*task_fun)(void *);

static char *my_strdup(const char *str){
    char *ret = jimi_malloc(strlen(str));
    strcpy(ret,str);
    return ret;
}

static void alios_setup(){
    set_malloc_ptr(aos_malloc);
    set_free_ptr(aos_free);
    set_realloc_ptr(aos_realloc);
    set_strdup_ptr(my_strdup);
}

static void wifi_service_event(input_event_t *event, void *priv_data)
{
    if (event->type != EV_WIFI) {
        return;
    }

    if (event->code != CODE_WIFI_ON_GOT_IP) {
        return;
    }

    netmgr_ap_config_t config;
    memset(&config, 0, sizeof(netmgr_ap_config_t));
    netmgr_get_ap_config(&config);
    LOGI("连接wifi成功 %s", config.ssid);
    if (strcmp(config.ssid, "adha") == 0 || strcmp(config.ssid, "aha") == 0) {
        //clear_wifi_ssid();
        return;
    }

    if (!linkkit_started) {
        alios_setup();
        aos_task_new("jimi_task",(task_fun)linkkit_main,(void *)&entry_paras,1024*6);
        linkkit_started = 1;
    }
}

extern void regist_cmd();

int application_start(int argc, char **argv)
{
#ifdef CSP_LINUXHOST
    signal(SIGPIPE, SIG_IGN);
#endif
#if AOS_ATCMD
    at_init();
#endif
 
    entry_paras.argc = argc;
    entry_paras.argv = argv;

#ifdef WITH_SAL
    sal_init();
#endif
    aos_set_log_level(AOS_LL_DEBUG);

    netmgr_init();

    aos_register_event_filter(EV_WIFI, wifi_service_event, NULL);

#ifdef AOS_COMP_CLI

#endif
    netmgr_start(false);

    //注册各种命令
    regist_cmd();
    aos_loop_run();
    shell_destory();
    return 0;
}
