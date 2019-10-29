# Overview
- The first stage boot loader for our NVMM emulator.

# Requirements
- Xilinx Vivado v2018.2
- Xilinx SDK v2018.2

# Procedure
### Run Vivado SDK
### Create new project
- From menu bar,
  - File -> New -> Application Project

### Configure project for FSBL
  
  | item | value |
  |:--:|:--:|
  | Project name | FSBL |
  | OS Platform | standalone |
  | Hardware Platform | ZC706_hw_platform(pre-defined) |
  | Processor | ps7_cortexa9_0 |
  | Language | C |
  | Board Support Package | Create New / FSBL_bsp |
  
### When compilation was completed successfully, FSBL (FSBL) is located at:
- Copy it to your work directory
- From left menu, 
```
Project Explorer/FSBL/Debug/FSBL.elf
```
