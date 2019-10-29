# Overview
- Library for NVMM region management
- This library provides following functions:
  - NVMM_Malloc
  - NVMM_Calloc
  - NVMM_Realloc
  - NVMM_Free
  - NVMM_FlushRange
  - NVMM_FlushRangeRelax
  - NVMM_StartRequestStat
  - NVMM_EndRequestStat


# LICENSE
- This library is released under the MIT License.


# Usage
- By default, **make** will generate **libnvmm.a** for dyanamic link when compilation.
- Or, **libnvmm.[c|h]** are copied into your work directory and compile **libnvmm.c** with your sources.

```
% make
...
% arm-linux-gnueabihf-gcc <your_src_or_obj>... libnvmm.a

OR

% arm-linux-gnueabihf-gcc <your_src_or_obj>... libnvmm.c
```



## NVMM_Malloc, NVMM_Calloc, NVMM_Realloc, NVMM_Free
- Allocate NVMM region or Release it
  - compatible with malloc, calloc, realloc, free

```
void *NVMM_Malloc(size_t size);
void *NVMM_Calloc(size_t nmemb, size_t size);
void *NVMM_Realloc(void *ptr, size_t size);
void  NVMM_Free(void *ptr);
```

**NOTICE**
- libnvmm uses /dev/mem, so binary with libnvmm must be ran by priviledged user (root, or sudo)
- NVMM region is only 1GB, so you cannot allocate over than 1GB simultaneously.
- You cannot use all of NVMM region because 8 bytes are used per one allocation for metadata.

## NVMM_FlushRange, NVMM_FlushRangeRelax
- Flush CPU cache to NVMM using **virtual address**
- Difference between them is restruction of DMB (data memory barrier)
  - **NVMM_FlushRange** do DMB every call
  - **NVMM_FlushRangeRelax** do not DMB automatically
    - To guarantee data consistency, you must do DMB by yourself if necessary
- libnvmm flush CPU **cache lines** in given flush range

```
// va_base : start VIRTUAL ADDRESS of flush range
// bytes   : size of flush range as BYTES

void  NVMM_FlushRange(void *va_base, size_t bytes);
void  NVMM_FlushRangeRelax(void *va_base, size_t bytes);
```

**NOTICE**
- To use these functions, **wbmod** must be installed to filesystem.


### Example
```
int *a = NVMM_Malloc(10 * sizeof(int));  // a[0] - a[9] are allocated from NVMM
NVMM_FlushRange(a, 5*sizeof(int));       // flush from a[0] to a[5]
```

## NVMM_StartRequestStat, NVMM_EndRequestStat
- Get statistics for memory requests to NVMM
- You can get following statistics:
  - **memreq.read**: How many READ requests are issued (between CPU LLC and memory controller)
  - **memreq.write**: How many WRITE requests are issued (between CPU LLC and memory controller)
  - **memreq.act**: How many ACTIVATE are issued (in memory controller)
  - **memreq.pre**: How many PRECHARGE are issued (in memory controller)
  - **memreq.bdr**: The number of READ requests that are issued to different bank with just before READ requests
  - **memreq.bdw**: The number of WRITE requests that are issued to different bank with just before WRITE requests
  - **memreq.clock**: return value from clock() at the time NVMM_*RequestStat is called
  - These values are declared as **int64_t** or **clock_t**
- NVMM_StartRequestStat reset counter value, so you can only ONE counter at a time.
  - You can get exact statistics for requests by use diff between StartRequestStat and EndRequestStat.

```
void  NVMM_StartRequestStat(memreq *start);
void  NVMM_EndRequestStat(memreq *end);
```




















