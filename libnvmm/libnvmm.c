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

#include <sys/types.h> /* open() */
#include <sys/stat.h>  /* open() */
#include <fcntl.h>     /* open() */
#include <unistd.h>    /* close() */
#include <sys/mman.h>  /* mmap(), munmap() */
#include <stdio.h>     /* perror() */
#include <errno.h>     /* perror() */
#include <stdlib.h>    /* exit(), malloc(), free() */
#include <sys/ioctl.h> /* ioctl() */
#include <time.h>      /* clock() */
#include <string.h>    /* memset() */
#include <stdarg.h>    /* va_start(), va_arg(), va_end() */

#include "libnvmm.h"

/* size def */
#if !defined(KiB)
#define KiB (1024)
#endif
#if !defined(MiB)
#define MiB (1024*KiB)
#endif
#if !defined(GiB)
#define GiB (1024*MiB)
#endif
#define PAGESIZE (4*KiB)
#define CACHELINE (32)

/* physical addr of NVMM */
#define NVMM_BEGIN (0x80000000) /* head 1 MiB is reserved */
#define NVMM_END   (0xBFFFFFFF)

/* minimum size for mmap */
#define NB_SIZEMIN (4*MiB)

/* branch prediction */
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)


/* typedef */
typedef unsigned char byte;    /* treat addr as BYTE */
typedef unsigned long addr_t;  /* phys/virt addr bits */


/* sturuct to manage NVMM */
struct _nvmm_block;
typedef struct _nvmm_region {
    size_t size;            /* allocated size */
    byte  *ptr;             /* allocated ptr */
    struct _nvmm_block *nb; /* pointer to parent nvmm_block */

    struct _nvmm_region *prev; /* pointer to prev nvmm_region */
    struct _nvmm_region *next; /* pointer to next nvmm_region */
} nvmm_region;

typedef struct _nvmm_block {
    addr_t  pa;   /* physical address */
    void   *va;   /* virtual addr (begin) */
    size_t  size; /* bytes used in mmap */
    size_t  free; /* free bytes */

    struct _nvmm_region *nr; /* allocatable region */
} nvmm_block;

typedef struct _region_info {
    struct _nvmm_region *nr;
    size_t  size;
} region_info;


/* nvmm_block_table is sorted by nb->free or not */
/* if has been sorted already, dont sort again */
static byte sorted_by_free;


/* cache for nvmm_block (previous searched nvmm_block) */
nvmm_block *nbb;

/* nvmm_block table */
#define MAXN_NB_TABLE (1*GiB / NB_SIZEMIN)
static nvmm_block *nvmm_block_table[MAXN_NB_TABLE];
static int num_nvmm_block; /* allocated nvmm_block */

/* file descriptors */
static int fd_devmem;   /* /dev/mem (    cacheable) */
static int fd_devmem_s; /* /dev/mem (non-cacheable) */
static int fd_wbmod;    /* /dev/wbmod */

/* pool of freeed nvmm_region */
static nvmm_region *nvmm_region_pool;

/* state of nvmmlib */
static byte is_initialized = 0;
static byte is_finalized   = 0;


/* func for ptr */
static inline int isNull(void *ptr)  { return ptr == NULL; }
static inline int nonNull(void *ptr) { return ptr != NULL; }


/* func for nvmm_block */
static inline int deallocatable(nvmm_block *nb) { return nb->size == nb->free; }

/* func for qsort() */
/* sort by free (ascending order) */
int cmp_by_free(const void *p1, const void *p2)
{
    size_t f1 = (*((nvmm_block **) p1))->free;
    size_t f2 = (*((nvmm_block **) p2))->free;

    if (f1 < f2)
        return -1;
    else if (f1 == f2)
        return 0;
    else
        return 1;
}

/* sort by free with NULL check (ascending order) */
int cmp_by_free_nc(const void *p1, const void *p2)
{
    /* NULL check. NULL is treated as quite big */
    if (isNull((void *) p1) && isNull((void *) p2)) /* p1 and p2 is equal */
        return 0;
    else if (isNull((void *) p1)) /* p1 is bigger than p2 */
        return 1;
    else if (isNull((void *) p2)) /* p2 is bigger than p1 */
        return -1;

    size_t f1 = (*((nvmm_block **) p1))->free;
    size_t f2 = (*((nvmm_block **) p2))->free;

    if (f1 < f2)
        return -1;
    else if (f1 == f2)
        return 0;
    else
        return 1;
}

/* sort by pa (ascending order) */
int cmp_by_pa(const void *p1, const void *p2)
{
    size_t pa1 = (*((nvmm_block **) p1))->pa;
    size_t pa2 = (*((nvmm_block **) p2))->pa;

    if (pa1 < pa2)
        return -1;
    else if (pa1 == pa2)
        return 0;
    else
        return 1;
}


/* undef malloc/calloc/realloc/free in nvmmlib */
#undef malloc
#undef calloc
#undef realloc
#undef free
#undef NVMM_Malloc
#undef NVMM_Calloc
#undef NVMM_Realloc
#undef NVMM_Free

/*
 ********** error handling **********
 */
char errmsg[100];
#define set_msg(...) sprintf(errmsg, __VA_ARGS__);

#define EXIT_NONE   (0x0)
#define EXIT_PERROR (0x1)
#define EXIT_STDERR (0x2)

static inline void
nvmmlib_exit(int errn, int caller)
{
    switch (caller) {
    case EXIT_NONE:
        break;
    case EXIT_PERROR:
        perror(errmsg);
        break;
    case EXIT_STDERR:
        fprintf(stderr, "%s", errmsg);
        break;
    default:
        break;
    }

    NVMM_Finalize();
    exit(errn);
}

static inline void exit_perror(int errn) { nvmmlib_exit(errn, EXIT_PERROR); }
static inline void exit_stderr()         { nvmmlib_exit(1,    EXIT_STDERR); }



/*
 ********** NVMM Management **********
 */
static inline
region_info *
ptr_to_ri(void *ptr)
{
    return (region_info *) (((byte *) ptr) - sizeof(region_info));
}



/**
 * Look for nvmm_region corresponding ptr
 *
 * @param ptr
 *            pointer to allocated region
 *
 * @return if ptr is   valid, corresponding nvmm_block
 *         if ptr is invalid, NULL
 *
 */
static inline nvmm_region *
get_nvmm_region(void *ptr)
{
    return ptr_to_ri(ptr)->nr;
}


/**
 * Align
 *
 * @param size
 *            source size
 * @param alignment
 *            size alignment
 *
 */
static inline size_t
align_size(size_t size, size_t alignment)
{
    size_t r = size % alignment;

    return (r == 0) ? size : (size + (alignment - r));
}


/**
 * Calculate size for mmap
 *
 * @param size
 *            request region size
 *
 * @return size for mmap
 *
 */
static inline size_t
to_mmapsize(size_t size)
{
    if (size <= NB_SIZEMIN)
        return NB_SIZEMIN;
    else
        return align_size(size, PAGESIZE);
}


/**
 * Allocate NVMM
 *
 * @param nb
 *            target nvmm_block
 *
 * @return none
 *
 */
static inline void
alloc_nvmm(nvmm_block *nb)
{
    void *ptr;

#if defined(ZC706)
    ptr = mmap(0, nb->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_devmem, nb->pa);
    if (ptr == MAP_FAILED) {
        set_msg("alloc_nvmm::mmap(ptr)");
        exit_perror(errno);
    }
#else
    ptr = malloc(nb->size);
    if (isNull(ptr)) {
        set_msg("alloc_nvmm::malloc(ptr)");
        exit_perror(errno);
    }
#endif

    nb->va = ptr;
    return;
}


/**
 * De-allocate NVMM
 *
 * @param nb
 *            nvmm_block
 *
 * @return none
 *
 */
static inline void
dealloc_nvmm(nvmm_block *nb)
{
#if defined(ZC706)
    munmap(nb->va, nb->size);
#else
    free(nb->va);
#endif

    return;
}


/**
 * Allocate nvmm_region
 *
 * @param none
 *
 * @return allocated nvmm_region
 *
 */
static inline nvmm_region *
alloc_nvmm_region()
{
    nvmm_region *nr;

    /*
     * At first, try to get from nvmm_region_pool.
     * if failed, call malloc
     */
    if (nonNull(nvmm_region_pool)) {
        nr = nvmm_region_pool;
        nvmm_region_pool = nvmm_region_pool->next;
    } else {
        nr = (nvmm_region *) malloc(sizeof(nvmm_region));
        if (unlikely(isNull(nr))) {
            set_msg("alloc_nvmm_region::malloc(nr)");
            exit_perror(errno);
        }
    }

    return nr;
}


/**
 * De-allocate NVMM (Return nr to nvmm_region_pool)
 *
 * @param nr
 *            target nvmm_region
 *
 * @return none
 *
 */
static inline void
dealloc_nvmm_region(nvmm_region *nr)
{
    nr->next = nvmm_region_pool;
    nvmm_region_pool = nr;
    return;
}


/**
 * Allocate nvmm_block
 *
 * @param none
 *
 * @return allocated nvmm_block
 *
 */
static inline nvmm_block *
alloc_nvmm_block()
{
    nvmm_block *nb;

    nb = (nvmm_block *) malloc(sizeof(nvmm_block));
    if (unlikely(isNull(nb))) {
        set_msg("alloc_nvmm_block::malloc(nb)");
        exit_perror(errno);
    }

    nb->nr = NULL;

    /* add to nvmm_block_table */
    nvmm_block_table[num_nvmm_block++] = nb;

    return nb;
}


/**
 * Add new nvmm_block
 *
 * @param srcnb
 *             source nvmm_block
 * @param size
 *             size of nvmm_region
 *
 * @return pointer to new nvmm_block
 *
 */
static inline nvmm_block *
new_nvmm_block(nvmm_block *srcnb, size_t size)
{
    size_t mmapsize;
    nvmm_block *nb;
    nvmm_region *nr;

    /* size alignment */
    mmapsize = to_mmapsize(size);

    /* allocate new nvmm_block */
    nb = alloc_nvmm_block();

    /* cutoff mmapsize from srcnb */
    nb->pa     = srcnb->pa;
    srcnb->pa += mmapsize;

    nb->size     = mmapsize;
    nb->free     = mmapsize;
    srcnb->free -= mmapsize;

    /* allocate NVMM */
    alloc_nvmm(nb);

    /* add allocatable region */
    nr = alloc_nvmm_region();
    nr->size = mmapsize;
    nr->ptr  = (byte *) (nb->va);
    nr->prev = NULL;
    nr->next = NULL;
    nr->nb   = nb;
    nb->nr   = nr;

    sorted_by_free = 0;

    /* all done */
    return nb;
}


/**
 * Remove nvmm_region from linked-list
 *
 * @param nr
 *            target nvmm_region
 * @param nb
 *            parent nvmm_block of nr
 *
 * @return none
 *
 */
static inline void
remove_nvmm_region(nvmm_region *nr)
{
    if (unlikely(isNull(nr->prev)))
        nr->nb->nr = nr->next;
    else
        nr->prev->next = nr->next;

    if (unlikely(isNull(nr->next)))
        ;
    else
        nr->next->prev = nr->prev;

    /* all done */
    return;
}


/**
 * Remove nvmm_region and free
 *
 * @param nr
 *            target nvmm_region
 * @param nb
 *            parent nvmm_block of nr
 *
 * @return none
 *
 */
static inline void
del_nvmm_region(nvmm_region *nr)
{
    /* remove from linked-list */
    remove_nvmm_region(nr);

    /* free */
    dealloc_nvmm_region(nr);

    /* all done */
    return;
}


/**
 * Delete nvmm_block
 *
 * @param nb
 *            target nvmm_block
 *
 * @return none
 *
 */
static inline void
del_nvmm_block(nvmm_block *nb)
{
    nvmm_region *nr, *nrn;

    /* free all nvmm_region */
    for (nr = nb->nr; nr != NULL; nr = nrn) {
        nrn = nr->next;

        free(nr);
    }

    /* free */
    free(nb);

    /* all done */
    return;
}


/**
 * Add new nvmm_region to given nvmm_block
 * +++ENTRY FUNCTION from NVMM_Malloc+++
 *
 * @param nb
 *            source nvmm_block
 * @param size
 *            size of NVMM region
 *
 * @return pointer to allocated region
 *
 */
static inline void *
new_nvmm_region(nvmm_block *nb, size_t size)
{
    nvmm_region *nr, *nrb;
    region_info *ri;

    /* add sizeof(region_info) to size */
    size += sizeof(region_info);

    /* if nb has not been mmaped, map */
    if (unlikely(isNull(nb->va)))
        nb = new_nvmm_block(nb, size);

    /* set cache */
    nbb = nb;

    /* look for enough nvmm_region */
    for (nr = nb->nr; nr != NULL; nr = nr->next) {
        if (size <= nr->size)
            break;
    }

    /* if nr is NULL, no enough nvmm_region */
    if (unlikely(isNull(nr)))
        return NULL;

    /* allocate new nvmm_region & cut size from nr */
    nrb = alloc_nvmm_region();
    nrb->size  = size;
    nr->size  -= size;
    nrb->ptr   = nr->ptr;
    nr->ptr   += size;
    nb->free  -= size;
    nrb->nb    = nb;

    /* set region_info */
    ri = (region_info *) (nrb->ptr);
    ri->nr   = nrb;
    ri->size = nrb->size;

    /* register to nvmm_region_table */
    nrb->prev = NULL;
    nrb->next = NULL;

    /* try to delete nr */
    if (nr->size == 0)
        del_nvmm_region(nr);

    return (void *) (nrb->ptr + sizeof(region_info));
}


/**
 * Return region size corresponding ptr
 *
 * @param ptr
 *            pointer to allocated region
 *
 * @return if ptr is     allocated, return size (> 0)
 *         if ptr is NOT allocated, return size (smaller than ZERO)
 */
static inline size_t
get_alloc_size(void *ptr)
{
    return ptr_to_ri(ptr)->size;
}


/**
 * Try to merge nvmm_region
 *
 * @param nr
 *            target nvmm_region
 * @param nb
 *            parent nvmm_block of nr
 *
 * @return none
 *
 */
static inline void
merge_nvmm_region(nvmm_region *nr)
{
    nvmm_region *nrp, *nrn;
    int is_merged;

    do {
        is_merged = 0;

        /* try to merge with nr & nr->prev */
        nrp = nr->prev;
        if (!isNull(nrp) && (nrp->ptr + nrp->size == nr->ptr)) {
            nr->ptr   = nrp->ptr;
            nr->size += nrp->size;

            del_nvmm_region(nrp);
            ++is_merged;
        }

        /* try to merge with nr & nr->next */
        nrn = nr->next;
        if (!isNull(nrn) && (nr->ptr + nr->size == nrn->ptr)) {
            nr->size += nrn->size;

            del_nvmm_region(nrn);
            ++is_merged;
        }
    } while (is_merged);

    /* all done */
    return;
}


/**
 * Try to merge nvmm_block
 *
 * @param nr
 *            target nvmm_block
 *
 * @return none
 *
 */
static inline void
merge_nvmm_block()
{
    nvmm_block *nb, *nbn;
    int idx, idxn;
    int merge_cnt;

    /* quick sort nvmm_block_table by pa */
    qsort(nvmm_block_table, num_nvmm_block, sizeof(nvmm_block *), cmp_by_pa);

    /* deallocate nvmm of deallocatable nvmm_block */
    for (idx = 0; idx < num_nvmm_block; ++idx) {
        nb = nvmm_block_table[idx];
        if (deallocatable(nb))
            dealloc_nvmm(nb);
    }

    /* try to merge */
    merge_cnt = 0; /* number of merged nvmm_block */
    for (idx = 0; idx < num_nvmm_block; ) {
        nb = nvmm_block_table[idx];

        /* if nb->va is NOT NULL, can't merge with others */
        if (nonNull(nb->va)) {
            ++idx;
            continue;
        }

        /* merge with succeesing nvmm_block */
        for (idxn = idx + 1; idxn < num_nvmm_block; ++idxn) {
            nbn = nvmm_block_table[idxn];
            if (isNull(nbn) || nonNull(nbn->va))
                break;

            /* merge */
            nb->free += nbn->free;
            del_nvmm_block(nbn);
            nvmm_block_table[idxn] = NULL;
            ++merge_cnt;
        }

        idx = idxn + 1;
    }

    /* quick sort nvmm_block_table by free */
    qsort(nvmm_block_table, num_nvmm_block, sizeof(nvmm_block *), cmp_by_free_nc);
    sorted_by_free = 1;

    /* update num_nvmm_block */
    num_nvmm_block -= merge_cnt;

    /* clear cache */
    nbb = NULL;

    /* all done */
    return;
}

/**
 * Move nvmm_region(busy) to nvmm_region(idle)
 *
 * @param ptr
 *            ptr to allocated region
 *
 * @return none
 *
 */
static inline void
free_nvmm_region(void *ptr)
{
    nvmm_block *nb;
    nvmm_region *nr, *nrp, *nrn;

    /* remove from nvmm_region_table */
    nr = ptr_to_ri(ptr)->nr;
    nb = nr->nb;

    /* insert to linked-list (idle) */
    nb->free += nr->size;
    if (isNull(nb->nr)) {
        nb->nr = nr;
    } else {
        nrp = nb->nr;
        nrn = nrp->next;
        for(;;) {
            /* idle list is sorted by ascending order of ptr */
            if (isNull(nrn)) {
                /* if nrn is NULL, insert nrp -> nr -> NULL */
                nrp->next = nr;
                nr->prev = nrp;

                //nrn->prev = nr;
                nr->next = nrn;

                break;
            } else if (nrp->ptr < nr->ptr && nr->ptr < nrn->ptr) {
                /* nrp < nr < nrn, insert nrp -> nr -> nrn */
                nrp->next = nr;
                nr->prev = nrp;

                nrn->prev = nr;
                nr->next = nrn;

                break;
            }

            /* goto next region */
            nrp = nrp->next;
            nrn = nrn->next;
        }
    }

    sorted_by_free = 0;

    /* try to merge nvmm_region */
    merge_nvmm_region(nr);

    /* all done */
    return;
}


/**
 * Return index in nvmm_block_table
 * nvmm_block_table[idx] has enough region for size
 *
 * @param size
 *            allocation size
 *
 * @return index in nvmm_block_table
 *
 */
static inline int
get_nbt_idx(size_t size)
{

    int left, right, mid;

    /* quick sort nvmm_block_table by free */
    if (!sorted_by_free) {
        qsort(nvmm_block_table, num_nvmm_block, sizeof(nvmm_block *), cmp_by_free);
        sorted_by_free = 1;
    }

    /* binary search */
    left = 0;
    right = num_nvmm_block - 1;
    while(left != right) {
        mid = (left + right) / 2;
        if (nvmm_block_table[mid]->free <= size)
            return mid; //right = mid;
        else
            left = mid + 1;
    }

    return left;
}


/**
 * Initialize nb_list
 *
 * @param none
 *
 * @return none
 *
 */
static inline void
initialize_nvmmlib()
{
    nvmm_block *nb;

    /* clear */
    num_nvmm_block = 0;

    nb = alloc_nvmm_block();
    nb->pa   = NVMM_BEGIN;
    nb->va   = NULL;
    nb->size = 0;
    nb->free = 1 * GiB;
    nb->nr   = NULL;

    /* set cache */
    nbb = nb;

    /* no nvmm_region in pool */
    nvmm_region_pool = NULL;

    /* nvmm_region_table is not sorted by free */
    sorted_by_free = 0;

    return;
}


/**
 * Finalize nvmmlib
 *
 * @param none
 *
 * @return none
 *
 */
static inline void
finalize_nvmmlib()
{
    nvmm_region *nr, *nrn;
    int i;

    /* free all allocated nvmm_block */
    for (i = 0; i < num_nvmm_block; ++i)
        del_nvmm_block(nvmm_block_table[i]);

    /* free all pooled nvmm_region */
    for (nr = nvmm_region_pool; nr != NULL; nr = nrn) {
        nrn = nr->next;
        free(nr);
    }

    return;
}


/**
 * Initialize nvmmlib
 *
 * @param none
 *
 * @return none
 *
 */
__attribute__((constructor))
void
NVMM_Initialize()
{
    /* if nvmmlib has been initialized already, do nothing */
    if (likely(is_initialized != 0))
        return;

#if defined(ZC706)
    fd_devmem   = open("/dev/mem", O_RDWR);          /* cacheable */
    fd_devmem_s = open("/dev/mem", O_RDWR | O_SYNC); /* non-cacheable */
    if (unlikely(fd_devmem == -1 || fd_devmem_s == -1)) {
        set_msg("NVMM_Initialize::open(/dev/mem)");
        exit_perror(errno);
    }

    fd_wbmod = open("/dev/wbmod", O_RDWR);
    if (unlikely(fd_wbmod == -1)) {
        set_msg("NVMM_Initialize::open(/dev/wbmod)");
        exit_perror(errno);
    }
#endif /* ZC706 */

    /* initialize free list */
    initialize_nvmmlib();

    /* set initalized flag */
    is_initialized++;

    return;
}


/**
 * Finalize nvmmlib
 *
 * @param none
 *
 * @return none
 *
 */
__attribute__((destructor))
void
NVMM_Finalize()
{
    /* if nvmlib has been finalized already, do nothing */
    if (unlikely(is_finalized != 0))
        return;

#if defined(ZC706)
    close(fd_devmem);
    close(fd_devmem_s);
    close(fd_wbmod);
#endif /* ZC706 */

    /* clean all free list */
    finalize_nvmmlib();

    /* set finalized flag */
    is_finalized++;

    return;
}


/**
 * Allocate NVMM
 *
 * @param size
 *            size of region
 *
 * @return pointer to allocated region
 *
 */
void *
NVMM_Malloc(size_t size)
{
    void *ptr;
    byte merged;
    int idx;

    /* to allocate NVMM, nvmmlib must be initialized */
    NVMM_Initialize();

    /* 4-byte alignment */
    /* to use vector (NEON), pointer must be aligned by 4 */
    size = align_size(size, 4);

    /* try to allocate from previous used nvmm_block */
    ptr = NULL;
    if (nonNull(nbb) && size <= nbb->free)
        ptr = new_nvmm_region(nbb, size);

    merged = 0;
    if (isNull(ptr))
        idx = get_nbt_idx(size);

    while(isNull(ptr)) {
        /* look for enough block */
        for (; idx < num_nvmm_block; ++idx) {
            if (size <= nvmm_block_table[idx]->free)
                break;
        }

        /* if search is failed */
        if (idx == num_nvmm_block) {
            if (!merged) {
                /* if have not merged, try merge and retry */
                merge_nvmm_block();
                ++merged;
                idx = get_nbt_idx(size);
                continue;
            } else {
                /* if try merge and failed to search again, exhausted. */
                set_msg("NVMM_Malloc::No Available NVMM\n");
                exit_stderr();
            }
        }

        /* try to allocate region */
        ptr = new_nvmm_region(nvmm_block_table[idx], size);

        ++idx;
    }


    return ptr;
}


/**
 * Allocate NVMM and ZERO-fill
 *
 * @param nmemb
 *            number of elements
 * @param size
 *            bytes per elements
 *
 * @return pointer to allocated region
 *
 */
void *
NVMM_Calloc(size_t nmemb, size_t size)
{
    void *retptr = NVMM_Malloc(nmemb * size);
    memset(retptr, 0, nmemb * size);

    return retptr;
}


/**
 * Reallocate ptr with given size
 *
 * @param ptr
 *            original ptr
 * @param size
 *            bytes for new allocation
 *
 * @return pointer to reallocated region
 *
 */
void *
NVMM_Realloc(void *ptr, size_t size)
{
    void *oldptr, *newptr;
    int oldsize, newsize;

    /* if ptr is NULL, work as NVMM_Malloc() */
    if (unlikely(isNull(ptr)))
        return NVMM_Malloc(size);

    /* Get size */
    oldsize = get_alloc_size(ptr);
    newsize = size;

    /* check ptr is valid or not */
    if (unlikely(oldsize <= 0)) {
        set_msg("NVMM_Realloc::Invalid pointer(%p)\n", ptr);
        exit_stderr();
    }

    /* if oldptr is enough for size, do nothing */
    if (unlikely(oldsize > newsize)) {
        return ptr;
    }

    /* Set oldptr, newptr */
    oldptr = ptr;
    newptr = NVMM_Malloc(size);

    /* if can't allocate newptr, return NULL */
    if (unlikely(isNull(newptr))) {
        return NULL;
    }

    /* Copy from oldptr to newptr */
    memcpy(newptr, oldptr, oldsize);

    /* Free oldptr */
    NVMM_Free(oldptr);

    return newptr;
}


/**
 * Free NVMM region
 *
 * @param ptr
 *            pointer to region
 *
 * @return none
 *
 */
void
NVMM_Free(void *ptr)
{
    if (unlikely(isNull(ptr)))
        return;

    if (likely(is_finalized == 0))
        free_nvmm_region(ptr);

    return;
}


void
NVMM_FlushRange(void *va_base, size_t size)
{
    flush_range range = { (unsigned long) va_base, size };
    ioctl(fd_wbmod, WBMOD_DCCMVAC_RANGE, &range);
    return;
}

void
NVMM_FlushRangeRelax(void *va_base, size_t size)
{
    flush_range range = { (unsigned long) va_base, size };
    ioctl(fd_wbmod, WBMOD_DCCMVAC_RANGE_RELAX, &range);
    return;
}


/*
 ********** Memory Request **********
 */
static byte *mrr_base = NULL;

static inline int64_t
read_mrr(size_t offset)
{
    return *((volatile addr_t *) (mrr_base + offset));
}

static inline void
write_mrr(size_t offset, int val)
{
    *((volatile addr_t *) (mrr_base + offset)) = val;
    return;
}


/**
 * Reset Read/Write counter
 *
 * @param none
 *
 * @return none
 *
 */
static inline void
reset_rw_cnt()
{
    write_mrr(0x00000000, 1); // reset read_cnt
    write_mrr(0x00000004, 1); // reset write_cnt
    write_mrr(0x00000028, 1); // reset bank_diff[r|w]
}


/**
 * Return counter value
 *
 * @param lf_offset
 *            offset in MemoryRequestRegister of Lower Half
 * @param uf_offset
 *            offset in MemoryRequestRegister of Upper Half
 *
 * @return counter value
 *
 */
static inline int64_t
get_cnt(addr_t lf_offset, addr_t uf_offset)
{
    return (read_mrr(uf_offset) << 32) | read_mrr(lf_offset);
}


/**
 * Read counter & store them
 *
 * @param req
 *            store target memreq
 * @param is_end
 *            if 1, read clock() at first
 *            else, read clock() at last
 *
 * @return none
 *
 */
static inline void
set_memreq(memreq *req, int is_end)
{
    if (is_end)
        req->clock = clock();

    req->read  = get_cnt(0x00000008, 0x0000000C);
    req->write = get_cnt(0x00000010, 0x00000014);
    req->act   = get_cnt(0x00000018, 0x0000001C);
    req->pre   = get_cnt(0x00000020, 0x00000024);
    req->bdr   = get_cnt(0x0000002C, 0x00000030);
    req->bdw   = get_cnt(0x00000034, 0x00000038);

    if (!is_end)
        req->clock = clock();
}


/**
 * Start to observe memory requests
 *
 * @param start
 *            memreq to store counter values
 *
 * @return none
 *
 */
void
NVMM_StartRequestStat(memreq *start)
{
    /* if open() and mmap() has not been called, call */
    if (isNull(mrr_base)) {
        mrr_base = (byte *) mmap(0, 4 * KiB, PROT_READ | PROT_WRITE, MAP_SHARED,
                                 fd_devmem_s, 0x43C10000);
        if (mrr_base == MAP_FAILED) {
            set_msg("NVMM_StartMemerqStat::mmap(mrr_base)");
            exit_perror(errno);
        }
    }

    /* reset counter */
    reset_rw_cnt();
    set_memreq(start, 0);

    return;
}


/**
 * End to observe memory requests
 *
 * @param start
 *            memreq to store counter values
 *
 * @return none
 *
 */
void
NVMM_EndRequestStat(memreq *end)
{
    set_memreq(end, 1);
    return;
}


#if defined(__cplusplus) && defined(ALL_IN_NVMM)
void *operator new(size_t size)         { return NVMM_Malloc(size); }
void  operator delete(void *p) noexcept { NVMM_Free(p); }
#endif


/* re-def malloc/calloc/realloc/free */
#if defined(ALL_IN_NVMM)
#define malloc(...)  NVMM_Malloc(__VA_ARGS__)
#define calloc(...)  NVMM_Calloc(__VA_ARGS__)
#define realloc(...) NVMM_Realloc(__VA_ARGS__)
#define free(...)    NVMM_Free(__VA_ARGS__)
#endif

#if defined(NO_NVMMLIB)
#define NVMM_Malloc(...)  malloc(__VA_ARGS__)
#define NVMM_Calloc(...)  calloc(__VA_ARGS__)
#define NVMM_Realloc(...) realloc(__VA_ARGS__)
#define NVMM_Free(...)    free(__VA_ARGS__)
#endif
