#
# The MIT License (MIT)

# Copyright (c) 2019 Yu Omori

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is furnished
# to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

# Makefile for wbmod (Write Back kernel MODule)

CROSS_COMPILE = arm-linux-gnueabihf-
CC = ${CROSS_COMPILE}gcc
AR = ${CROSS_COMPILE}ar

obj-m := wbmod.o

# bsolute path for linux-xlnx
KDIR = /home/oy/linux-xlnx


all:
	make ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} --directory=${KDIR} M=$(shell pwd) modules

PHONY: clean
clean:
	make ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} --directory=${KDIR} M=$(shell pwd) clean
	rm -f *~
