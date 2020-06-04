#ifndef __HUGEPAGE_H__
#define __HUGEPAGE_H__

#include "base_type.h"
#include "transcoder.h"

#ifdef __cplusplus
extern "C" {
#endif

void *fbtrans_get_huge_pages(unsigned int len);
int fbtrans_free_huge_pages(void *ptr, unsigned int len);

#ifdef __cplusplus
}
#endif

#endif


