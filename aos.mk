NAME := mqtt_c
$(NAME)_MBINS_TYPE := app
$(NAME)_VERSION := 1.0.0
$(NAME)_SUMMARY := Jimi SDK
$(NAME)_SOURCES := src/source/base64.c \
                   src/source/hash-table.c \
                   src/source/iot_proto.c \
                   src/source/jimi_buffer.c \
                   src/source/jimi_iot.c \
                   src/source/jimi_log.c \
                   src/source/jimi_memory.c \
                   src/source/md5.c \
                   src/source/mqtt.c \
                   src/source/mqtt_buffer.c \
                   src/source/mqtt_wrapper.c \
                   src/source/avl-tree.c \
                   src/source/jimi_http.c \
                   src/source/jimi_shell.c \
                   src/aos/app_entry.c \
                   src/aos/mqtt.c \
                   src/aos/sensor.c \
                   net.c \
                   shell_cmd.c


$(NAME)_INCLUDES :=  . src src/include src/source src/aos
$(NAME)_DEFINES  := __alios__
$(NAME)_COMPONENTS += cli netmgr
GLOBAL_INCLUDES += src/aos
GLOBAL_DEFINES  := WITH_SAL
# GLOBAL_DEFINES  += AT_CHECK_SUM=1 DEBUG