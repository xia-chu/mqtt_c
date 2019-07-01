//
// Created by xzl on 2019/7/1.
//

#include <stdlib.h>
#include <string.h>
#include "jimi_memory.h"
#include "jimi_log.h"

void *jimi_malloc(int size){
    void *ptr = malloc(size);
    CHECK_PTR(ptr,NULL);
    return ptr;
}

int jimi_free(void *ptr){
    CHECK_PTR(ptr,-1);
    free(ptr);
    return 0;
}

void *jimi_realloc(void *ptr,int size){
    CHECK_PTR(ptr,NULL);
    CHECK_PTR(size,NULL);
    void *ret = realloc(ptr,size);
    CHECK_PTR(ret,NULL);
    return ret;
}

char *jimi_strdup(const char *str){
    CHECK_PTR(str,NULL);
    char *ret = strdup(str);
    CHECK_PTR(ret,NULL);
    return ret;
}