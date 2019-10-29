# Overview
- Documents and some files to build our NVMM Emulator on ZC706

# Requirements
- Xilinx Vivado v2018.2
- Xilinx SDK v2018.2

**NOTICE**
  - Vivado later v2018.2 may work fine, however, project is sensitive to Vivado version.
    - If you use later one, please solve problems by yourself.

# Procedures
### Generate BOOT.bin
- Refer
  - [u-boot.md](./u-boot.md)
  - [bitstream.md](./bitstream.md)
  - [FSBL.md](./FSBL.md)
  - [BOOT.bin.md](./BOOT.bin.md)

### Generate uImage (Linux kernel)
- Refer
  - [uImage.md](./uImage.md)

### Generate device tree
- Refer
  - [device_tree.md](./device_tree.md)

### Format SD card
- Refer
  - [SDcard-format.md](./SDcard-format.md)

### Extract rootfs of Linux
- Refer
  - [rootfs.md](./rootfs.md)

### Put files to board/linux boot & Check them
- Refer
  - [SDcard-structure.md](./SDcard-structure.md)
