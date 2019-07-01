//
// Created by xzl on 2019/7/1.
//

#include <stdlib.h>
#include <string.h>
#include "jimi_memory.h"
#include "jimi_log.h"

static malloc_ptr s_malloc_ptr = NULL;
static free_ptr s_free_ptr = NULL;
static realloc_ptr s_realloc_ptr = NULL;
static strdup_ptr s_strdup_ptr = NULL;


void set_malloc_ptr(malloc_ptr ptr){
    s_malloc_ptr = ptr;
}

void set_free_ptr(free_ptr ptr){
    s_free_ptr = ptr;
}

void set_realloc_ptr(realloc_ptr ptr){
    s_realloc_ptr = ptr;
}

void set_strdup_ptr(strdup_ptr ptr){
    s_strdup_ptr = ptr;
}

void *jimi_malloc(int size){
    void *ptr = s_malloc_ptr ? s_malloc_ptr(size) : malloc(size);
    CHECK_PTR(ptr,NULL);
    return ptr;
}

void jimi_free(void *ptr){
    CHECK_PTR(ptr,);
    s_free_ptr ? s_free_ptr(ptr) : free(ptr);
}

void *jimi_realloc(void *ptr,int size){
    CHECK_PTR(ptr,NULL);
    CHECK_PTR(size,NULL);
    void *ret = s_realloc_ptr ? s_realloc_ptr(ptr,size) : realloc(ptr,size);
    CHECK_PTR(ret,NULL);
    return ret;
}

char *jimi_strdup(const char *str){
    CHECK_PTR(str,NULL);
    char *ret = s_strdup_ptr ? s_strdup_ptr(str) : strdup(str);
    CHECK_PTR(ret,NULL);
    return ret;
}