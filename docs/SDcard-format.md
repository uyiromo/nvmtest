# Overview
- Prepare SD card partitions for our NVMM emulator

# Requirements
- Following packages are required:
 - fdisk
 - parted

# Procedures
- In following procedures, we assume that SD card is recognized as **/dev/sdX**

### Set header, sector(, cylinder)
- 55 heads, 63 sectores
```
% sudo fdisk /dev/sdX
 
Command (m for help): x
Expert command (m for help): h
Number of heads (1-256, default 30): 255
Expert command (m for help): s
Number of sectors (1-63, default 29): 63
Expert command (m for help): c
Number of cylinders (1-1048576, default 2286): 
Expert Command (m for help): r
Command (m for help): w
```

### Create partitions
- partition1: FAT32, 200MiB, bootable
- partition2: ext4, all the rest
```
% sudo parted /dev/sdX --script 'mklabel msdos mkpart primary FAT32 1MiB 201MiB set 1 boot on mkpart primary ext4 201MiB 100%'
```

### Format partitions
```
% mkfs.vfat -F 32 -n BOOT /dev/sdX1
% mkfs.ext4 -L root /dev/sdX2
```
