# Homa-NIC
## Dependencies
- Vitis HLS 2023.1
- Vivado 2023.1
- [Alveo U250 XDC File](https://www.xilinx.com/bin/public/openDownload?filename=alveo-u250-xdc_20210909.zip)
- [Alveo U250 Board File](https://www.xilinx.com/bin/public/openDownload?filename=alveo-u250-xdc_20210909.zip)
## Vivado Setup
The Alveo U250 board file needs to be added to your Vivado installation. Assuming Vivado is installed in `$XILINX_VIVADO`:
```
wget "https://www.xilinx.com/bin/public/openDownload?filename=au250_board_files_20200616.zip"
unzip *au250_board_files*.zip -d $XILINX_VIVADO/data/xhub/boards/XilinxBoardStore/boards/Xilinx/
```
## Repository Setup
The XDC cannot be packaged in this repository but can downloaded from Xilinx. Assuming this repository is located in `$HOMA_NIC`:
```
wget "https://www.xilinx.com/bin/public/openDownload?filename=alveo-u250-xdc_20210909.zip"
unzip *alveo-u250-xdc*.zip -d $HOMA_NIC/xdc
```
## Vitis Flow
### Synthesis
TODO
```make synth```

### Tests
### C-simulation Tests
TODO
```make ...```

### Co-simulation Tests
TODO
```make ...```

## Vivado Flow
TODO
```make homa```
