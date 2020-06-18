/*------------------------------------------------------------------------------
--       Copyright (c) 2015-2017, VeriSilicon Holdings Co., Ltd. All rights reserved        --
--         Copyright (c) 2011-2014, Google Inc. All rights reserved.          --
--         Copyright (c) 2007-2010, Hantro OY. All rights reserved.           --
--                                                                            --
-- This software is confidential and proprietary and may be used only as      --
--   expressly authorized by VeriSilicon in a written licensing agreement.    --
--                                                                            --
--         This entire notice must be reproduced on all copies                --
--                       and may not be removed.                              --
--                                                                            --
--------------------------------------------------------------------------------
-- Redistribution and use in source and binary forms, with or without         --
-- modification, are permitted provided that the following conditions are met:--
--   * Redistributions of source code must retain the above copyright notice, --
--       this list of conditions and the following disclaimer.                --
--   * Redistributions in binary form must reproduce the above copyright      --
--       notice, this list of conditions and the following disclaimer in the  --
--       documentation and/or other materials provided with the distribution. --
--   * Neither the names of Google nor the names of its contributors may be   --
--       used to endorse or promote products derived from this software       --
--       without specific prior written permission.                           --
--------------------------------------------------------------------------------
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"--
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE  --
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE --
-- ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE  --
-- LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR        --
-- CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF       --
-- SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS   --
-- INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN    --
-- CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    --
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE --
-- POSSIBILITY OF SUCH DAMAGE.                                                --
--------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

#ifndef __DWL_THREAD_H__
#define __DWL_THREAD_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* This header file is the POSIX pthread.h and semaphore.h entry point
 * for the entire decoder. If pthread cannot be used the decoder can be
 * directed to use a replacing implementation via this interface. */

/* Undefine _HAVE_PTHREAD_H to replace POSIX pthread.h and semaphore.h */
#ifndef NON_PTHREAD_H
#define _HAVE_PTHREAD_H
#endif

#ifdef _HAVE_PTHREAD_H
#include <pthread.h>
#include <semaphore.h>
#else /* _HAVE_PTHREAD_H */

#include "basetype.h"

/* The following error check can be removed when inplementation available. */
//#error "Threading and semaphore interface not implemented."

#if 0
typedef enum {
  FALSE = 0,
  TRUE  = 1
} dwl_bool;
#else
typedef int dwl_bool;
#endif
typedef struct tagAttribute
{
        i32 a;
} Attribute, *PAttribute;

typedef struct tagMutex
{
        dwl_bool locked;
        Attribute attrib;
} Mutex, *PMutex;

typedef struct tagEvent
{
        dwl_bool singaled;
        Attribute attrib;
} Event, *PEvent;

typedef struct tagSemaphore
{
        i32 val;
} Semaphore, *PSemaphore;

#define sem_t Semaphore

#define __u8 u8
#define __i8 i8
#define __u32 u32
#define __i32 i32


/*=========================================================*/
/*======  Declaration of dwl_os APIs ===============================*/
/*=========================================================*/
int MutexInit(PMutex m, PAttribute a);
int MutexDestroy(PMutex m);
int MutexLock(PMutex m);
int MutexUnlock(PMutex m);
int EventInit(PEvent e, PAttribute a);
int EventDestroy(PEvent e);
int EventWait(PEvent e, PMutex m);
int EventSignal(PEvent e);
int SemaphoreInit(PSemaphore s, u32 value);
int SemaphoreDestroy(PSemaphore s);
int SemaphoreWait(PSemaphore s);
int SemaphorePost(PSemaphore s);
int SemaphoreGetValue(PSemaphore s);
/*=========================================================*/

/*=========================================================*/
/*======  redefinitions of all Linux Threads/Semaphores/Signals functions  ======*/
/*=========================================================*/
#define pthread_mutex_t Mutex
#define pthread_cond_init(c,a) EventInit(c,a)
#define pthread_cond_destroy(c) EventDestroy(c)
#define pthread_cond_wait(c,m) EventWait(c,m)
#define pthread_cond_signal(c) EventSignal(c)
#define pthread_mutex_init(m,a) MutexInit(m,a)
#define pthread_mutex_destroy(m) MutexDestroy(m)
#define pthread_mutex_lock(m) MutexLock(m)
#define pthread_mutex_unlock(m) MutexUnlock(m)

#define pthread_attr_init(a)
#define pthread_attr_destroy(a)
#define pthread_attr_setdetachstate(a,f)
#define pthread_create(l,a,f,p) (0)
#define pthread_yield()
#define sched_yield()
#define pthread_join(t, r) (0)


#define sema_init(s,v) SemaphoreInit(s,v)
#define sem_init(s,p,v) SemaphoreInit(s,v)
#define sem_wait(s) SemaphoreWait(s)
#define sem_post(s) SemaphorePost(s)
#define sem_destroy(s) SemaphoreDestroy(s)
#define sem_getvalue(s,v) ((*v) = SemaphoreGetValue(s))


#define  DWL_PLACEHOLDER_VALUE 0

typedef void * DWL_OS_OBJECT_PTR;
/*
#define pthread_mutex_t Mutex */
#define pthread_cond_t Event


#define pthread_t DWL_OS_OBJECT_PTR
#define pthread_attr_t DWL_OS_OBJECT_PTR
#define pthread_condattr_t DWL_OS_OBJECT_PTR
#define pthread_mutexattr_t DWL_OS_OBJECT_PTR

/*--------------------------------*/
/* IO */
#define O_RDONLY (1)
#define O_WRONLY (1 << 1)
#define O_RDWR (O_RDONLY | O_WRONLY)
#define O_SYNC (1 << 2)
#define MAP_FAILED NULL

#define getpagesize() (1)
#define ioremap_nocache(addr,size) (addr)
#define iounmap(addr)
#define mmap(va, size, access, flag, fd, pa) (pa)
#define munmap(pRegs, size)

#define open(name,flag) g1_sim_open(name,flag)
#define ioctl(fd,cmd,arg) g1_sim_ioctl(fd,cmd,(unsigned long)arg)
#define close(fd) g1_sim_close(fd)
#define printk(a,...)
#define access_ok(a,b,c) (1)
#define __init
#define __exit
#define KERN_INFO
#define KERN_ERR
#define KERN_WARNING

/*
 * Let any architecture override either of the following before
 * including this file.
 */

#ifndef _IOC_SIZEBITS
# define _IOC_SIZEBITS  14
#endif

#ifndef _IOC_DIRBITS
# define _IOC_DIRBITS   2
#endif
#define _IOC_NRBITS     8
#define _IOC_TYPEBITS   8

#define _IOC_NRMASK     ((1 << _IOC_NRBITS)-1)
#define _IOC_TYPEMASK   ((1 << _IOC_TYPEBITS)-1)
#define _IOC_SIZEMASK   ((1 << _IOC_SIZEBITS)-1)
#define _IOC_DIRMASK    ((1 << _IOC_DIRBITS)-1)

#define _IOC_NRSHIFT    0
#define _IOC_TYPESHIFT  (_IOC_NRSHIFT+_IOC_NRBITS)
#define _IOC_SIZESHIFT  (_IOC_TYPESHIFT+_IOC_TYPEBITS)
#define _IOC_DIRSHIFT   (_IOC_SIZESHIFT+_IOC_SIZEBITS)

/*
 * Direction bits, which any architecture can choose to override
 * before including this file.
 */
#ifndef _IOC_NONE
# define _IOC_NONE      0U
#endif

#ifndef _IOC_WRITE
# define _IOC_WRITE     1U
#endif

#ifndef _IOC_READ
# define _IOC_READ      2U
#endif

#define _IOC(dir,type,nr,size) \
        (((dir)  << _IOC_DIRSHIFT) | \
         ((type) << _IOC_TYPESHIFT) | \
         ((nr)   << _IOC_NRSHIFT) | \
         ((size) << _IOC_SIZESHIFT))

#define _IOC_TYPECHECK(t) (sizeof(t))

/* used to create numbers */
#define _IO(type,nr)            _IOC(_IOC_NONE,(type),(nr),0)
#define _IOR(type,nr,size)      _IOC(_IOC_READ,(type),(nr),(_IOC_TYPECHECK(size)))
#define _IOW(type,nr,size)      _IOC(_IOC_WRITE,(type),(nr),(_IOC_TYPECHECK(size)))
#define _IOWR(type,nr,size)     _IOC(_IOC_READ|_IOC_WRITE,(type),(nr),(_IOC_TYPECHECK(size)))
#define _IOR_BAD(type,nr,size)  _IOC(_IOC_READ,(type),(nr),sizeof(size))
#define _IOW_BAD(type,nr,size)  _IOC(_IOC_WRITE,(type),(nr),sizeof(size))

/* used to decode ioctl numbers.. */
#define _IOC_DIR(nr)            (((nr) >> _IOC_DIRSHIFT) & _IOC_DIRMASK)
#define _IOC_TYPE(nr)           (((nr) >> _IOC_TYPESHIFT) & _IOC_TYPEMASK)
#define _IOC_NR(nr)             (((nr) >> _IOC_NRSHIFT) & _IOC_NRMASK)
#define _IOC_SIZE(nr)           (((nr) >> _IOC_SIZESHIFT) & _IOC_SIZEMASK)

/* ...and for the drivers/sound files... */
#define IOC_IN          (_IOC_WRITE << _IOC_DIRSHIFT)
#define IOC_OUT         (_IOC_READ << _IOC_DIRSHIFT)
#define IOC_INOUT       ((_IOC_WRITE|_IOC_READ) << _IOC_DIRSHIFT)
#define IOCSIZE_MASK    (_IOC_SIZEMASK << _IOC_SIZESHIFT)
#define IOCSIZE_SHIFT   (_IOC_SIZESHIFT)

#define DECLARE_WAIT_QUEUE_HEAD(a)

/* Kernel/User space */
#define copy_from_user(des,src,size) (memcpy(des,src,size),0)
#define copy_to_user(des,src,size) (memcpy(des,src,size),0)
#define __put_user(val,user) (*(user) = (val))
#define __get_user(val,user) ((val) = *user)

/* Interrupt */
#define request_irq(i,isr,flag,name,data) RegisterIRQ(i, isr, flag, name, data)
#define disable_irq(i) IntDisableIRQ(i)
#define enable_irq(i) IntEnableIRQ(i)
#define free_irq(i,data)
#define irqreturn_t int
#define down_interruptible(a) SemaphoreWait(a)
#define up(a) SemaphorePost(a)
#define wait_event_interruptible(a,b) (!(b))
#define wake_up_interruptible_all(a)
#define IRQ_RETVAL(a) (a)
#define IRQ_HANDLED 1
#define IRQ_NONE 0
#define register_chrdev(m,name,op) (0)
#define unregister_chrdev(m,name)
#define IRQF_SHARED 1
#define IRQF_DISABLED 0x00000020
#define request_mem_region(addr,size,name) (1)
#define release_mem_region(addr,size)
#define kill_fasync(queue,sig,flag)

#define ERR_OS_FAIL (0xffff)
#define ERESTARTSYS ERR_OS_FAIL
#define EFAULT ERR_OS_FAIL
#define ENOTTY ERR_OS_FAIL
#define EINVAL ERR_OS_FAIL
#define EBUSY ERR_OS_FAIL

/* kernel sync objects */
#define atomic_t i32
#define ATOMIC_INIT(a) a
#define atomic_inc(a) ((*(a))++)
#define atomic_read(a) (a)
typedef int sig_atomic_t;
//typedef int sigset_t;
#define sigemptyset(set)
#define sigaddset(set, sig)
#define sigsuspend(set)
#define sigwait(set, signo)

#define spin_lock_irqsave(a,b)
#define spin_unlock_irqrestore(a,b)
/*=========================================================*/

#endif /*_HAVE_PTHREAD_H*/

#ifdef __cplusplus
}
#endif

#endif /* __DWL_THREAD_H__ */
