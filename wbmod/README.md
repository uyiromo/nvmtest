# Overview
- Kernel module for flushing CPU cache from userspace

# LICENSE
- This is released under the MIT License.


# Usage
## Compilation
- Run **make** in this directory
- **wbmod.ko** should be generated if compilation was finished successfully.

**NOTICE**
- Before make, you must modify **Makefile**
  - **KDIR** must be changed to absolute path to linux-xlnx

## Installation
- Install **wbmod.ko** into Ubuntu filesystem.
- Like below:
```
% insmod <path_to_wbmod.ko>
% major=`cat /proc/devices | grep wbmod | awk '{print $1}'`
% mknod /dev/wbmod --mode=666 c $major 0
```
