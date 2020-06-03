#ifndef  __TRANSMEM_API_H__
#define __TRANSMEM_API_H__

#include "base_type.h"

#ifdef __cplusplus
extern "C" {
#endif

//#define CHECK_MEM_LEAK_TRANS

#ifdef CHECK_MEM_LEAK_TRANS
int TransCheckMemLeakInit();
int TransCheckMemLeakGotResult();
int trans_calc_alloc_mem_inc();
int trans_calc_free_mem_inc();
#endif

void *Trans_malloc_func(u32 n, const char *func_name, const int line);
void *Trans_calloc_func(u32 n, u32 s, const char *func_name, const int line);
void Trans_free_func(void *p, const char *func_name, const int line);


#ifdef CHECK_MEM_LEAK_TRANS
#define Trans_malloc(n)      Trans_malloc_func(n, __FUNCTION__, __LINE__)
#define Trans_calloc(n, s)   Trans_calloc_func(n, s, __FUNCTION__, __LINE__)
#define Trans_free(p)        Trans_free_func(p, __FUNCTION__, __LINE__)
#else
#define Trans_malloc(n)      malloc(n)
#define Trans_calloc(n, s)   calloc(n, s)
#define Trans_free(p)        free(p)
#endif


#ifdef __cplusplus
}  // extern "C"
#endif

#endif

