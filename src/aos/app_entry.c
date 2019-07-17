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

static app_main_paras_t entry_paras;
typedef void (*task_fun)(void *);

static void wifi_service_event(input_event_t *event, void *priv_data) {
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

    static char linkkit_started = 0;
    if (!linkkit_started) {
        linkkit_main(&entry_paras);
        linkkit_started = 1;
    }
}

extern void regist_cmd();

int application_start(int argc, char **argv) {
    entry_paras.argc = argc;
    entry_paras.argv = argv;

#ifdef CSP_LINUXHOST
    signal(SIGPIPE, SIG_IGN);
#endif

#if AOS_ATCMD
    at_init();
#endif

#ifdef WITH_SAL
    sal_init();
#endif

    aos_set_log_level(AOS_LL_DEBUG);
    netmgr_init();
    aos_register_event_filter(EV_WIFI, wifi_service_event, NULL);
    netmgr_start(false);

#ifdef AOS_COMP_CLI
    regist_cmd();
#endif

    aos_loop_run();

#ifdef AOS_COMP_CLI
    shell_destory();
#endif
    return 0;
}
