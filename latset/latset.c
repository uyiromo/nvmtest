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

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#define LATSET_BASE (0x43C00000)

typedef unsigned char byte;

#define KB (1024)
#define MB (1024*KB)
#define GB (1024*MB)

#define PL_READ(base, offset) \
    *((volatile unsigned int *) ((base) + (offset)));
#define PL_WRITE(base, offset, val) \
    *((volatile unsigned int *) ((base) + (offset))) = (val);

int main(int argc, char **argv)
{
    int fd;
    byte *base;
    int rlat, wlat;
    unsigned int offr, offw;
    unsigned int rlat_bef, rlat_aft, wlat_bef, wlat_aft;
    int is_err, lat_v;

    /* check args */
    is_err = 0;
    lat_v = 2;
    if (argc != 3 && argc != 4) {
        is_err = 1;
    } else {
        rlat = atoi(argv[1]);
        wlat = atoi(argv[2]);

        if (argc == 4) {
            if (strcmp(argv[3], "coarse") == 0)
                lat_v = 1;
            else if (strcmp(argv[3], "fine") == 0)
                lat_v = 2;
            else
                is_err = 1;
        }
    }

    /* args err */
    if (is_err) {
        fprintf(stderr, "Usage: ./latset rlat wlat\n");
        exit(1);
    }


    /* open /dev/mem */
    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1) {
        perror("failed to open /dev/mem");
        exit(1);
    }

    /* mmap */
    base = mmap(0, 4 * KB, PROT_READ | PROT_WRITE, MAP_SHARED, fd, LATSET_BASE);
    if (base == MAP_FAILED) {
        perror("failed to mmap");
        exit(1);
    }

    offr = lat_v == 1 ? 0x00000000 : 0x00000008;
    offw = lat_v == 1 ? 0x00000004 : 0x0000000C;

    /* read unchanged value */
    rlat_bef = PL_READ(base, offr);
    wlat_bef = PL_READ(base, offw);

    /* reset */
    PL_WRITE(base, 0x00000000, 0);
    PL_WRITE(base, 0x00000004, 0);
    PL_WRITE(base, 0x00000008, 0);
    PL_WRITE(base, 0x0000000C, 0);

    /* set latency */
    PL_WRITE(base, offr, rlat / 5);
    PL_WRITE(base, offw, wlat / 5);

    /* read changed value */
    rlat_aft = PL_READ(base, offr);
    wlat_aft = PL_READ(base, offw);

    /* print */
    fprintf(stderr, "Latency ver. \"v%d\"\n", lat_v);
    fprintf(stderr, "  rlat: %4d [ns] -> %4d [ns]\n", rlat_bef * 5, rlat_aft * 5);
    fprintf(stderr, "  wlat: %4d [ns] -> %4d [ns]\n", wlat_bef * 5, wlat_aft * 5);

    /* de-allocate */
    munmap(base, 4 * KB);
    close(fd);

    return 0;
}
