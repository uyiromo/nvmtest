# Overview
- The binary file to boot board

# Requirements
- Xilinx Vivado v2018.2
- Xilinx SDK v2018.2


# Procedures
### Write bootimage.bif
```
image : {
      [bootloader] FSBL.elf
                   system_top.bit
                   u-boot.elf
}
```

### Put files as below:
```
% tree
.
├── bootimage.bif
├── FSBL.elf
├── system_top.bit
└── u-boot.elf
```

### Read PATH to Xilinx tool chain
```
% source /opt/Xilinx/Vivado/2018.2/settings64.sh
% source /opt/Xilinx/SDK/2018.2/settings64.sh
```

### Generate BOOT.bin
```
% bootgen -image bootimage.bif -w -o BOOT.bin
```

### When compilation was completed successfully, BOOT.bin is located at:
- Copy it to your work directory
```
./BOOT.bin
```
