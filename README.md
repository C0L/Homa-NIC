# Homa-NIC

## Software Flow
### Dependencies
- Linux Kernel 4.15
### Kernel Module
#### Build
```
cd driver
make
```
#### Install
```
sudo insmod homanic.ko
```

### Remove
```
sudo rmmod homanic.ko
```

### Applications
#### Build
```
cd app
make apps
```

#### Tests
##### Perf Testing
Stresses the NIC with 64B echo requests, 64B device register reads, 64B packet loopback.
```
make parse
```

## HW-Dev Dependencies
- Vivado 2023.2