# Overview
- Utitlity for configuring memory access latency to NVMM on our NVMM emulator

# LICENSE
- This utility is released under the MIT License.

# Usage
- You can set ADDITIONAL LATENCY through this command
```
// rlat : NVMM  READ latency, in other words, "tRCD" in DDR protocol
// wlat : NVMM WRITE latency, in other words, "tRP"  in DDR protocol
% latset <rlat> <wlat>
```

**NOTICE**
- latset uses /dev/mem, so latset must be ran by priviledged user (root, or sudo)
- To memory modules work correctly, **tRCD** and **tRP** must be bigger than them in spec sheet
  - "tRCD" and "tRP" of memory module on our emulator are 15 [ns] and 15 [ns]
- when you set rlat=100 and wlat=200,
  - additional 100 [ns] is inserted into **tRCD**
  - additional 200 [ns] is inserted into **tRP**
  - In other words, **tRCD** will be 115 [ns] and **tRP** will be 215 [ns]
- <rlat> and <wlat> should be multiplier of 5 (restriction on our emulator)
  - If they are NOT multiplier of 5, they will be rounded down, for example, 112 will be 110.

## Example
```
% latset 100 123
-> tRCD is 115 [ns], tRP is 135 [ns]
```
