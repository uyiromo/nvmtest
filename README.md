# Overview
- Softwares and README for NVMM Emulator on Xilinx ZC706 board.
  - Implemented by Yu OMORI and Keiji Kimura at Waseda University.

# Requirements
- Our NVMM emulator was implemented on **Xilinx ZC706 Evaluation Board (Zynq-7000 XC7Z045 SoC)**
- To run this NVMM emulator, capacity of SD card must be greater than or equls to **8-GB**


# How to clone
- This repository uses **git-lfs**. Please install it before clone
  - git lfs installation: https://github.com/git-lfs/git-lfs/wiki/Installation
- After git-lfs insallation, you can clone by ```git clone```



# Repository Structure
```
.
├── docs            # documents and some files to build our NVMM Emulator
├── latset          # source files for tool to set memory access latency
├── libnvmm         # source files for NVMM management library
├── wbmod           # source files for kernel module to flush CPU cache from user space
└── nvmtest.tar.gz  # Vivado project for our emulator
```

# CONTACT
- Yu OMORI
  - Master Course, Department of Computer Science and Engineering at Waseda University, Japan
  - oy [at] kasahara.cs.waseda.ac.jp
    - please replace **[at]** with @

- Keiji Kimura
  - Professor, Department of Computer Science and Engineering at Waseda University, Japan
  - keiji [at] waseda.jp
    - please replace **[at]** with @


# Acknowledge
This work was partly executed under the cooperation of organization between Toshiba Memory Corporation and Waseda University.

# LICENSE
- libnvmm, latset and wbmod are released under the MIT License.
- If you use our emulator, please refer to the following paper:  
>  Yu Omori, Keiji Kimura, “Performance Evaluation on NVMM Emulator Employing Fine-Grain Delay Injection”,  
>  Proc. of 2019 IEEE Non-Volatile Memory Systems and Applications Symposium (NVMSA),  Hangzhou, China, August 18-21, 2019  
>  https://ieeexplore.ieee.org/document/8863522
