# Overview
- The Vivado project for our NVMM emulator.

# Requirements
- Xilinx Vivado v2018.2
- Xilinx SDK v2018.2

# Procedure
### Extract nvmtest.tar.gz
```
% tar zxvf <path_to_nvmtest.tar.gz>
```

### Run Vivado and Open project
- From top menu bar,
  - File -> Project -> Open
  - Select nvmtest.xpr in <path_to_extracted_nvmtest>

### Generate Bitstream
- From top menu bar,
  - Flow -> Generate Bitstream

### When compilation was completed successfully, bitstream (nvmtest_top.bit) is located at:
- Copy it to your work directory
```
<path_to_extracted_nvmtest>/nvmtest.runs/impl_1/nvmtest_top.bit
```
