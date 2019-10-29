# Overview
- The Linux kernel for our NVMM emulator
  - This is based on Linux kennel (linux-xlnx) provided by Xilinx.
  - https://github.com/Xilinx/linux-xlnx.git

- Our patch for linux-xlnx is under GPLv2.0 license.


# Requirements
- Following packages are required:
  - git
  - make
  - arm-linux-gnueabihf-gcc



# Procedures
### Fetch linux-xlnx from Xilinx official repository
 ```
 % git clone git@github.com:Xilinx/linux-xlnx.git
 ```

### Checkout **xilinx-v2019.1** tag
```
% cd linux-xlnx
% git checkout -b xilinx-v2019.1
```

### Copy **linux-xlnx-for-emulator.patch** and Apply it
```
% cp <path_to_linux-xlnx-for-emulator.patch> .
% git apply linux-xlnx-for-emulator.patch
```

### Run make
```
% export ARCH=arm
% export CROSS_COMPILE=arm-linx-gnueabihf-
% make xilinx_zynq_defconfig
% make UIMAGE_LOADADDR=0x8000 uImage
```

### When compilation was completed successfully, the kernel image (uImage) is located at:
- Copy it to your work directory
```
<path_to_linux-xlnx>/arch/arm/boot/uImage
```
