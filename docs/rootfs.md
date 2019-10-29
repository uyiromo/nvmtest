# Overview
- Root filesystem of Ubuntu for our NVMM emulator

# Requirements
- Following packages are required:
   - debootstrap
   - qemu-user-static
   - arm-linux-gnueabihf-gcc

# Procedures
### Create symbolic links to use debootstrap
```
% cd /lib/
% sudo ln -s /usr/arm-linux-gnueabihf/lib/ld-linux-armhf.so.3 .
% sudo ln -s /usr/arm-linux-gnueabihf/lib arm-linux-gnueabihf
```


### mount ext4 partition
- In this example, we assume that ext4 partition is recognized on "dev/sdX", and this will be mounted on "/mnt"
 ```
 % sudo mount /dev/sdX2 /mnt
 ```
 
### Extract Ubuntu for armhf on /mnt
```
% sudo debootstrap --foreign --arch=armhf xenial /mnt http://ports.ubuntu.com/
```
- If you want to use Ubutnu 18, change **xenial** to **bionic**

### Jail with QEMU
```
% sudo cp /usr/bin/qemu-arm-static /mnt/usr/bin/
% sudo chroot /mnt bash
> /usr/bin/groups: cannot find name for group ID 0
> I have no name!@ubuntu:/#
```

### Complete debootstrap
```
I have no name!@ubuntu:/# ./debootstrap/debootstrap --second-stage
```

### Configure this rootfs to use Ubuntu with ssh
- enable **root** and **zc706 (priviledged)** users
```
I have no name!@ubuntu:/# passwd
I have no name!@ubuntu:/# su
I have no name!@ubuntu:/# apt-get update
I have no name!@ubuntu:/# adduser zc706
I have no name!@ubuntu:/# gpasswd -a zc706 sudo
I have no name!@ubuntu:/# apt-get install openssh-server emacs sudo build-essential
```

### Enable network
```
% sudo cat /mnt/etc/network/interfaces
auto eth0
iface eth0 inet dhcp
```

### Install kernel modules and kernel headers
```
I have no name!@ubuntu:/# exit

% export ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
% export CROSS_COMPILE=arm-linux-gnueabihf-
% cd <path_to_linux-xlnx>
% make modules
% sudo INSTALL_MOD_PATH=/mnt modules_install
% sudo INSTALL_HDR_PATH=/mnt/usr headers_install
```
