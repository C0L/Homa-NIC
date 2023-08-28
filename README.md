# Homa-NIC
## Dependencies
- Vitis HLS 2023.1
- Vivado 2023.1
- [Alveo U250 XDC File](https://www.xilinx.com/bin/public/openDownload?filename=alveo-u250-xdc_20210909.zip)
- [Alveo U250 Board File](https://www.xilinx.com/bin/public/openDownload?filename=au250_board_files_20200616.zip)
## Vivado Setup
The Alveo U250 board file needs to be added to your Vivado installation. Assuming Vivado is installed in `$XILINX_VIVADO`:
```
wget -P $XILINX_VIVADO/ https://www.xilinx.com/bin/public/openDownload?filename=au250_board_files_20200616.zip | unzip 
```
