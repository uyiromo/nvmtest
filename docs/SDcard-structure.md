# Overview
- SD card structure

# Procedures
### Put files shown below: 
```
% sudo mount /dev/sdX1 /mnt/BOOT
% sudo mount /dev/sdX2 /mnt/root

% tree /mnt/BOOT
/mnt/BOOT
├── BOOT.bin
├── devicetree.dtb
├── uImage
└── uEnv.txt

 % tree -L 1 /mnt/root
/mnt/root
├── bin
├── boot
├── dev
├── etc
├── home
├── lib
├── lost+found
├── media
├── mnt
├── opt
├── proc
├── root
├── run
├── sbin
├── srv
├── sys
├── tmp
├── usr
└── var
```
