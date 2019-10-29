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

#ifndef WBMOD_H
#define WBMOD_H
#include <linux/ioctl.h>

#define WBMOD_IOC_TYPE 'M'
#define WBMOD_DCCMVAC       _IOW(WBMOD_IOC_TYPE, 1, unsigned long)
#define WBMOD_DCCMVAC_RANGE _IOW(WBMOD_IOC_TYPE, 2, unsigned long)
#define WBMOD_DCIMVAC_RANGE _IOW(WBMOD_IOC_TYPE, 3, unsigned long)
#define WBMOD_DCCMVAC_RANGE_RELAX _IOW(WBMOD_IOC_TYPE, 4, unsigned long)

#define WBMOD_NAME "wbmod0"

typedef struct _flush_range {
    unsigned long va_base;
    unsigned long size ;
} flush_range;

#endif /* WBMOD_H */
