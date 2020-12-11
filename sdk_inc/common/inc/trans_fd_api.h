#ifndef __TRANS_FD_H__
#define __TRANS_FD_H__

#include "base_type.h"

#ifdef __cplusplus
extern "C" {
#endif

int TranscodeOpenFD(const char *device, int mode);
int TranscodeCloseFD(int fd);

#ifdef __cplusplus
}
#endif

#endif


