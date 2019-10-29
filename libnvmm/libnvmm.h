/*
 * The MIT License (MIT)

 * Copyright (c) 2019 Yu Omori

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnis-
 * hed to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABI-
 * LITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
 * OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#ifndef _NVMLIB_H_INCLUDED
#define _NVMLIB_H_INCLUDED

#include <time.h>     /* clock_t */
#include <stdlib.h>   /* malloc/caloc/realloc/free */
#include <stddef.h>   /* size_t */
#include <inttypes.h> /* int64_t */

typedef struct _memreq {
    int64_t read;
    int64_t write;
    int64_t act;
    int64_t pre;
    int64_t bdr; // bank_diff_r
    int64_t bdw; // bank_diff_w
    clock_t clock;
} memreq;


#if defined(__cplusplus)
extern "C" {
#endif
void  NVMM_Initialize();
void  NVMM_Finalize();
void *NVMM_Malloc(size_t size);
void *NVMM_Calloc(size_t nmemb, size_t size);
void *NVMM_Realloc(void *ptr, size_t size);
void  NVMM_Free(void *ptr);
void  NVMM_FlushRange(void *va_base, size_t bytes);
void  NVMM_FlushRangeRelax(void *va_base, size_t bytes);
void  NVMM_StartRequestStat(memreq *start);
void  NVMM_EndRequestStat(memreq *start);
#if defined(__cplusplus)
}
#endif


/******************** Flags *************************/
/**
 * Flags
 * - ZC706
 *   If this flag is     defined, use mmap/munmap for mamory management (for DRAM)
 *   If this flag is NOT defined, use malloc/free for memory management (for NVM)
 *
 * - DO_FLUSH
 *   If this flag is     defined, do DCCMVAC in NVM_FlashRange()
 *   If this flag is NOT defined, do NOTHING in NVM_FlashRange()
 *
 */
//#define ZC706         /* use NVMM */

/* wbmod & mrr is enable on only ZC706 */
#if !defined(ZC706)
#undef DO_FLUSH
#undef NVMM_REQUEST_STAT
#endif

//#define ALL_IN_NVMM
//#define NO_LIBNVMM

/******************** MACRO and OVERRIDE ***************************/
#if defined(ALL_IN_NVMM)
#define malloc(...)  NVMM_Malloc(__VA_ARGS__)
#define calloc(...)  NVMM_Calloc(__VA_ARGS__)
#define realloc(...) NVMM_Realloc(__VA_ARGS__)
#define free(...)    NVMM_Free(__VA_ARGS__)
#endif

#if defined(NO_LIBNVMM)
#define NVMM_Malloc(...)  malloc(__VA_ARGS__)
#define NVMM_Calloc(...)  calloc(__VA_ARGS__)
#define NVMM_Realloc(...) realloc(__VA_ARGS__)
#define NVMM_Free(...)    free(__VA_ARGS__)
#endif

/********************** Copy of wbmod.h ***************************/
#include <linux/ioctl.h>

#define WBMOD_IOC_TYPE 'M'
#define WBMOD_DCCMVAC _IOW(WBMOD_IOC_TYPE, 1, unsigned long)
#define WBMOD_DCCMVAC_RANGE _IOW(WBMOD_IOC_TYPE, 2, unsigned long)
#define WBMOD_DCIMVAC_RANGE _IOW(WBMOD_IOC_TYPE, 3, unsigned long)
#define WBMOD_DCCMVAC_RANGE_RELAX _IOW(WBMOD_IOC_TYPE, 4, unsigned long)

typedef struct _flush_range {
    unsigned long va_base;
    unsigned long size;
} flush_range;

#endif
