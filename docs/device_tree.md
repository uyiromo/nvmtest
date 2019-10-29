# Overview
- Device tree for our NVMM emulator.

# Requirements
- Xilinx Vivado v2018.2
- Xilinx SDK v2018.2

# Procedures
### Fetch Xilinx Device Tree Generator from official Xilinx Git
```
% git clone git@github.com:Xilinx/device-tree-xlnx.git
```

### Run Vivado, Open nvmtest and Export hardware
- From top menu bar,
  - File -> Export -> Export hardware

### Run Vivado SDK
- From top menu bar,
  - File -> Launch SDK

### Import hardware specification
- From top menu bar,
  - File -> New -> Project
- Select project type & next
  - Xilinx -> HardwarePlatform Specification
- Import system_top.hdf
  - Browse.. -> (select system_top.hdf)
- Project name is set automatically:
  - e.g) **nvmtest_top_hw_platform_0** remember this
- Finish

### Import device-tree-xlnx
- From top menu bar,
  - Xilinx -> Repositories
- Open import wizard
  - Local Repositories -> New
  - move to top of device-tree-xlnx, OK

### Create new project
- From menu bar
  - File -> New -> Board Support Package

### Configure project for device tree
- If "device_tree" is not found at "Board Support Package OS", please check if device-tree-xlnx is imported successfully.  
  
  | item | value |
  |:--:|:--:|
  | Hardware Platform | system_top_hw_platform_0 (as you remember above) |
  | Processor | ps7_cortexa9_0 |
  | Language | C |
  | Board Support Package OS | device_tree |

### Board Support Package Settings Wizard:
- Set **bootargs**
  - Overview -> device_tree
```
console=ttyPS0,115200 root=/dev/mmcblk0p2 rw earlyprintk rootfstype=ext4 rootwait devtmpfs.mount=1 earlycon
```

## Remove a specific line
```
% cat <path_to_xsdk_workspace>/device_tree_bsp_0/zynq-7000.dtsi
...
    operating-points = <
        /* kHz    uV */
        666667  1000000
        333334  1000000 // DELETE THIS LINE
    >;
...
```


### Generate device tree blob
```
% <path_to_linux-xlnx>/scripts/dtc/dtc -I dts -O dtb -o devicetree.dtb <source_dts_name>.dts
<source_dts_name> = <path_to_xsdk_workspace>/device_tree_bsp_0/system-top.dts
```



### When compilation was completed successfully, devicetree.dtb is located at:
- Copy it to your work directory
```
./devicetree.dtb
```
