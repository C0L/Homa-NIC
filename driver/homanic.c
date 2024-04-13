#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/printk.h>
#include <linux/mman.h>
#include <linux/dma-mapping.h>
#include <linux/pci.h>
#include <linux/set_memory.h>
#include <asm/fpu/api.h>
#include <linux/delay.h>

#define _MM_MALLOC_H_INCLUDED
#include <x86intrin.h>

#include <linux/io.h>

#define MAX_PORTS 16
#define METADATA_SIZE 16384

#define H2C_MSGBUFF_MAP  0
#define C2H_MSGBUFF_MAP  1
#define C2H_METADATA_MAP 2

#define AXI_CMAC_1 0x30000

#define MINOR_H2C_METADATA 0
#define MINOR_C2H_METADATA 1
#define MINOR_H2C_MSGBUFF  2
#define MINOR_C2H_MSGBUFF  3

#define MAX_MINOR 4

// The maximum number of bytes in a single message
#define HOMA_MAX_MESSAGE_LENGTH 1000000 

struct pci_dev * pdev;
uint32_t BAR_0;

// NIC device registers
void __iomem * io_regs;

// State associated with a user session
struct session {
    bool assigned;
    uint16_t port;

    void * h2c_msgbuff_cpu_addr;
    dma_addr_t h2c_msgbuff_dma_handle;

    void * c2h_msgbuff_cpu_addr;
    dma_addr_t c2h_msgbuff_dma_handle;

    void * c2h_metadata_cpu_addr;
    dma_addr_t c2h_metadata_dma_handle;
};

// state associated with a single NIC user
struct session sessions[MAX_PORTS];

// Structure passed to NIC for configuration
struct cfg {
    char unused1[62]; 
    uint8_t writeBufferSize;
    uint8_t fetchRequestSize;
}__attribute__((packed));

// Structure passed to NIC for DMA address mappings
struct port_to_phys {
    uint64_t phys_addr;
    uint16_t port;
    uint8_t  type;
    char     pad[53];
}__attribute__((packed));

// Aligned configuration declaration
struct cfg cfg __attribute__((aligned(64)));
struct port_to_phys dma_map __attribute__((aligned(64)));

/* Kernel Module Functions */
int     homanic_open(struct inode *, struct file *);
ssize_t homanic_read(struct file *, char *, size_t, loff_t *);
int     homanic_close(struct inode *, struct file *);
int     homanic_mmap(struct file * fp, struct vm_area_struct * vma);

void init_eth(void);

struct file_operations homanic_fops = {
    .owner          = THIS_MODULE,
    .open           = homanic_open,
    .release        = homanic_close,
    .mmap           = homanic_mmap,

};

struct device_data {
    struct cdev cdev;
};

struct device_data devs[4];

struct class * cls;

int dev_major = 0;

// https://lore.kernel.org/lkml/157428502934.36836.8119026517510193201.stgit@djiang5-desk3.ch.intel.com/
// https://stackoverflow.com/questions/51918804/generating-a-64-byte-read-pcie-tlp-from-an-x86-cpu
void iomov64B(__m256i * dst, __m256i * src) {
    __m256i ymm0;
    __m256i ymm1;

    /* avx registers are not protected.
     * will corrupt user state without this?
     */
    kernel_fpu_begin();

    ymm0 = _mm256_stream_load_si256(src);
    ymm1 = _mm256_stream_load_si256(src+1);

    _mm256_store_si256(dst, ymm0);
    _mm256_store_si256(dst+1, ymm1);
    _mm_mfence();

    kernel_fpu_end();
}

// https://docs.xilinx.com/r/en-US/pg203-cmac-usplus/Without-AXI4-Lite-Interface
void init_eth() {
    iowrite32(0x00000001, io_regs + AXI_CMAC_1 + 0x00000);

    iowrite32(0x00000001, io_regs + AXI_CMAC_1 + 0x00014);
    iowrite32(0x00000010, io_regs + AXI_CMAC_1 + 0x0000C);

    iowrite32(0x00000001, io_regs + AXI_CMAC_1 + 0x0000C);
    while((ioread32(io_regs + AXI_CMAC_1 + 0x0204) & 0x2) != 0x2);

    pr_alert("CMAC stat %x\n", ioread32(io_regs + AXI_CMAC_1 + 0x0204));
    pr_alert("CMAC stat %x\n", ioread32(io_regs + AXI_CMAC_1 + 0x0200));
} 

// TODO can multiple threads call open simultanously
int homanic_open(struct inode * inode, struct file * file) {
    struct session * session;
    size_t s;

    pr_alert("homanic_open");

    // TODO find the open port
    for (s = 0; s < MAX_PORTS; ++s) {
	if (sessions[s].assigned == false) {
	    session = sessions + s;
	    break;
	}

	// TODO better error handling
	if (s == MAX_PORTS-1) {
	    return 1;
	}
    }

    // Ensure that this session is not allocated to anyone else
    session->assigned = true;

    // Associate this session with this file descriptor
    file->private_data = session;

    // printk(KERN_ALERT "port %d: ", session->port);
    pr_alert("port %d: ", session->port);

    // Allocate a buffer for metadata written from card to host
    session->c2h_metadata_cpu_addr = dma_alloc_coherent(&(pdev->dev), 16384, &(session->c2h_metadata_dma_handle), GFP_KERNEL);

    // Create an on-chip mapping to this buffer
    dma_map.phys_addr = ((uint64_t) session->c2h_metadata_dma_handle);
    dma_map.port      = 1; // session->port; TODO
    dma_map.type      = C2H_METADATA_MAP;
    iomov64B((void*) io_regs, (void*) &dma_map);

    // Allocate a buffer for metadata written from card to host
    session->h2c_msgbuff_cpu_addr = dma_alloc_coherent(&(pdev->dev), 1 * HOMA_MAX_MESSAGE_LENGTH, &(session->h2c_msgbuff_dma_handle), GFP_KERNEL);

    // Create an on-chip mapping to this buffer
    dma_map.phys_addr = ((uint64_t) session->h2c_msgbuff_dma_handle);
    dma_map.port      = 1; // session->port; TODO
    dma_map.type      = H2C_MSGBUFF_MAP;
    iomov64B((void*) io_regs, (void*) &dma_map);

    // Allocate a buffer for metadata written from card to host
    session->c2h_msgbuff_cpu_addr  = dma_alloc_coherent(&(pdev->dev), 1 * HOMA_MAX_MESSAGE_LENGTH, &(session->c2h_msgbuff_dma_handle), GFP_KERNEL);

    // Create an on-chip mapping to this buffer
    dma_map.phys_addr = ((uint64_t) session->c2h_msgbuff_dma_handle);
    dma_map.port      = 1; // session->port;
    dma_map.type      = C2H_MSGBUFF_MAP;
    iomov64B((void*) io_regs, (void*) &dma_map);

    return 0;
}

// TODO should implement munmap?
int homanic_close(struct inode * inode, struct file * file) {
    struct session * session = file->private_data;

    pr_alert("homanic_close\n");

    // TODO no write back?
    // Free the DMA buffer
    dma_free_coherent(&(pdev->dev), 16384, session->c2h_metadata_cpu_addr, session->c2h_metadata_dma_handle);

    // Reset memory in page attribute table to write back, free the DMA buffer
    set_memory_wb((uint64_t) session->h2c_msgbuff_cpu_addr, (1 * HOMA_MAX_MESSAGE_LENGTH) / PAGE_SIZE);
    dma_free_coherent(&(pdev->dev), 1 * HOMA_MAX_MESSAGE_LENGTH, session->h2c_msgbuff_cpu_addr, session->h2c_msgbuff_dma_handle);

    // Reset memory in page attribute table to write back, free the DMA buffer
    set_memory_wb((uint64_t) session->c2h_msgbuff_cpu_addr, (1 * HOMA_MAX_MESSAGE_LENGTH) / PAGE_SIZE);
    dma_free_coherent(&(pdev->dev), 1 * HOMA_MAX_MESSAGE_LENGTH, session->c2h_msgbuff_cpu_addr, session->c2h_msgbuff_dma_handle);

    return 0;
}

// TODO store data in struct file?
int homanic_mmap(struct file * file, struct vm_area_struct * vma) {
    int ret = 0;
    int map = vma->vm_pgoff;
    struct session * session = file->private_data;

    pr_alert("homanic_mmap\n");

    vma->vm_pgoff = 0;

    printk(KERN_ALERT "port %d: ", session->port);
    printk(KERN_ALERT "Chunk %d: ", vma->vm_pgoff);

    switch(map) {
	case 0:
	    // C2H Metadata
	    set_memory_uc((uint64_t) session->c2h_metadata_cpu_addr, 16384 / PAGE_SIZE);
	    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	    ret = dma_mmap_coherent(&(pdev->dev), vma, session->c2h_metadata_cpu_addr, session->c2h_metadata_dma_handle, 16384);
	    pr_alert("0\n");
	    break;

	case 1:
	    pr_alert("1\n");
	    // H2C Metadata
	    // TODO should shift
	    ret = remap_pfn_range(vma, vma->vm_start, (BAR_0 + 4096) >> PAGE_SHIFT, 4096, vma->vm_page_prot);
	    break;

	case 2:
	    pr_alert("2\n");

	    pr_alert("%d\n", vma->vm_end - vma->vm_start);
	    // H2C Msgbuff
	    set_memory_uc((uint64_t) session->h2c_msgbuff_cpu_addr, (1 * HOMA_MAX_MESSAGE_LENGTH) / PAGE_SIZE);
	    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	    ret = dma_mmap_coherent(&(pdev->dev), vma, session->h2c_msgbuff_cpu_addr, session->h2c_msgbuff_dma_handle, 1 * HOMA_MAX_MESSAGE_LENGTH);
	    break;

	case 3:
	    pr_alert("3\n");

	    pr_alert("%d\n", vma->vm_end - vma->vm_start);
	    // C2H Msgbuff
	    set_memory_uc((uint64_t) session->c2h_msgbuff_cpu_addr, (1 * HOMA_MAX_MESSAGE_LENGTH) / PAGE_SIZE);
	    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	    ret = dma_mmap_coherent(&(pdev->dev), vma, session->c2h_msgbuff_cpu_addr, session->c2h_msgbuff_dma_handle, 1 * HOMA_MAX_MESSAGE_LENGTH);
	    break;
    }

exit:
    return ret;
}

static void pci_enable_capability(struct pci_dev *pdev, int cap) {
    uint16_t v;
    int pos;
    pos = pci_pcie_cap(pdev);
    if (pos > 0) {
	pci_read_config_word(pdev, pos + PCI_EXP_DEVCTL, &v);
	v |= cap;
	pci_write_config_word(pdev, pos + PCI_EXP_DEVCTL, v);
    }
}

int homanic_init(void) {
    dev_t dev;
    int err;
    size_t s;

    pr_info("homanic_init\n");

    // Ports go from 1-MAX_PORTS
    for (s = 0; s < MAX_PORTS; ++s) {
	sessions[s].port = s+1;
	sessions[s].assigned = false;
    }

    pdev = pci_get_device(0x1234, 0x0001, NULL);
     
    pr_info("device %x.\n", pdev);

    /* Wake up the device if suspended and allocate IO and mem regions of the device if BIOS did not */
    pci_enable_device(pdev);
    
    pci_enable_capability(pdev, PCI_EXP_DEVCTL_EXT_TAG);

    /* Enables DMA by setting the bus master bit in PCI_COMMAND register */
    pci_set_master(pdev);

    pr_alert("bar: %llx\n", (uint64_t) pci_resource_start(pdev, 0));

    BAR_0 = pci_resource_start(pdev, 0);

    dma_set_mask_and_coherent(&(pdev->dev), DMA_BIT_MASK(48));

    err = alloc_chrdev_region(&dev, 0, 1, "homanic");
    // err = alloc_chrdev_region(&dev, 0, MAX_MINOR, "homanic");

    if (err != 0) {
	return err;
    }

    dev_major = MAJOR(dev);

    cls = class_create(THIS_MODULE, "homanic");
    device_create(cls, NULL, MKDEV(dev_major, 0), NULL, "homanic");
    cdev_init(&devs[0].cdev, &homanic_fops);
    cdev_add(&devs[0].cdev, MKDEV(dev_major, 0), 1);

    // device_create(cls, NULL, MKDEV(dev_major, MINOR_H2C_METADATA), NULL, "homa_nic_h2c_metadata");
    // device_create(cls, NULL, MKDEV(dev_major, MINOR_C2H_METADATA), NULL, "homa_nic_c2h_metadata");
    // device_create(cls, NULL, MKDEV(dev_major, MINOR_H2C_MSGBUFF),  NULL, "homa_nic_h2c_msgbuff");
    // device_create(cls, NULL, MKDEV(dev_major, MINOR_C2H_MSGBUFF),  NULL, "homa_nic_c2h_msgbuff");

    // cdev_init(&devs[MINOR_H2C_METADATA].cdev, &homanic_fops);
    // cdev_init(&devs[MINOR_C2H_METADATA].cdev, &homanic_fops);
    // cdev_init(&devs[MINOR_H2C_MSGBUFF].cdev,  &homanic_fops);
    // cdev_init(&devs[MINOR_C2H_MSGBUFF].cdev,  &homanic_fops);

    // cdev_add(&devs[MINOR_H2C_METADATA].cdev, MKDEV(dev_major, MINOR_H2C_METADATA), 1);
    // cdev_add(&devs[MINOR_C2H_METADATA].cdev, MKDEV(dev_major, MINOR_C2H_METADATA), 1);
    // cdev_add(&devs[MINOR_H2C_MSGBUFF].cdev,  MKDEV(dev_major, MINOR_H2C_MSGBUFF),  1);
    // cdev_add(&devs[MINOR_C2H_MSGBUFF].cdev,  MKDEV(dev_major, MINOR_C2H_MSGBUFF),  1);

    io_regs = ioremap_wc(BAR_0, 0xA0000);

    // TODO currently this is limited by the fetch core hardcoded for 256 byte
    // cfg.fetchRequestSize = 256;
    // cfg.writeBufferSize  = 256;
    
    // cfg.fetchRequestSize = 256;
    // cfg.writeBufferSize  = 1024;
    

    // 100.76 GBps
    cfg.fetchRequestSize = 10;
    cfg.writeBufferSize  = 10;

    iomov64B((void*) io_regs + 64, (void *) &cfg);

    // init_eth();

    return 0;
}

void homanic_exit(void) {
    pr_info("homanic_exit\n");

    device_destroy(cls, MKDEV(dev_major, 0));
    // device_destroy(cls, MKDEV(dev_major, MINOR_H2C_METADATA));
    // device_destroy(cls, MKDEV(dev_major, MINOR_C2H_METADATA));
    // device_destroy(cls, MKDEV(dev_major, MINOR_H2C_MSGBUFF));
    // device_destroy(cls, MKDEV(dev_major, MINOR_C2H_MSGBUFF));
    class_destroy(cls);

    cdev_del(&devs[0].cdev);
    // cdev_del(&devs[MINOR_H2C_METADATA].cdev);
    // cdev_del(&devs[MINOR_C2H_METADATA].cdev);
    // cdev_del(&devs[MINOR_H2C_MSGBUFF].cdev);
    // cdev_del(&devs[MINOR_C2H_MSGBUFF].cdev);
    // unregister_chrdev_region(MKDEV(dev_major, 0), MAX_MINOR);

    unregister_chrdev_region(MKDEV(dev_major, 0), 1);

    iounmap(io_regs);
}

module_init(homanic_init)
module_exit(homanic_exit)
MODULE_LICENSE("GPL");



// void dump_mb() {
//     int i;
// 
//     volatile uint64_t * prog_mem = (volatile uint64_t*) (io_regs + 0x40000);
//  
//     for (i = 0; i < 200; ++i) {
// 	pr_alert("mem index %x: %x\n", i * 4, *((uint32_t*) (prog_mem + i)));
//     }
// }
// 
// void init_mb() {
//     int i;
// 
//     uint32_t * prog_mem = (uint32_t*) (io_regs + 0x40000);
// 
//     for (i = 0; i < ((_binary_firmware_end - _binary_firmware_start) / 4); ++i) {
// 	iowrite32(*(((uint32_t*) _binary_firmware_start) + i), prog_mem + i);
//     }
// 
//     for (i = 0; i < 150; ++i) {
//     	pr_alert("mem index %x: %x\n", i * 4, ioread32(prog_mem + i));
//     }
// }


// long homanic_ioctl(struct file * file, unsigned int ioctl_num, unsigned long ioctl_param) {
//     printk(KERN_ALERT "homanic_ioctl\n");
// 
//     switch(ioctl_num) {
// 	case IOCTL_DMA_DUMP:
// 
// 	    // dump_log();
// 	    // for (i = 0; i < 8; ++i) {
// 	    // 	printk(KERN_ALERT "Chunk %d: ", i);
// 	    // 	for (j = 0; j < 64; ++j) printk(KERN_CONT "%02hhX", *(((unsigned char *) c2h_msgbuff_cpu_addr) + j + (i*64)));
// 	    // }
// 	    // 
// 	    // for (i = 0; i < 8; ++i) {
// 	    // 	printk(KERN_ALERT "Chunk %d: ", i);
// 	    // 	for (j = 0; j < 64; ++j) printk(KERN_CONT "%02hhX", *(((unsigned char *) h2c_msgbuff_cpu_addr) + j + (i*64)));
// 	    // }
// 	    break;
//     }
// 
//     return 0;
// }


// void dump_log() {
//      uint32_t rlr = 0;
//      uint32_t rdfo = 0;
//  
//      // iowrite32(LOG_CONTROL_DEST, axi_stream_regs + AXI_STREAM_FIFO_TDR);
//  
//      log_control.state = LOG_DRAIN;
// 
//      iomov64B((void*) io_regs + 320, (void*) &log_control);
//  
//      pr_alert("Dump Log\n");
//  
//      rdfo = ioread32(io_regs + 0x11000 + AXI_STREAM_FIFO_RDFO);
//      rlr  = ioread32(io_regs + 0x11000 + AXI_STREAM_FIFO_RLR);
//  
//      while (rdfo != 0) {
//  	iomov64B((void *) &log_entry, (void*) io_regs + 0x13000);
//  	
//  	iowrite32(0xffffffff, io_regs + 0x11000 + AXI_STREAM_FIFO_ISR);
//  	iowrite32(0x0C000000, io_regs + 0x11000 + AXI_STREAM_FIFO_IER);
//  
//  	rdfo = ioread32(io_regs + 0x11000 + AXI_STREAM_FIFO_RDFO);
//  	rlr  = ioread32(io_regs + 0x11000 + AXI_STREAM_FIFO_RLR);
//  
//  	pr_alert("Log Entry: ");
//  	pr_alert("  DMA Write Req  - %02hhX", log_entry.dma_w_req_log);
//  	pr_alert("  DMA Write Stat - %02hhX", log_entry.dma_w_stat_log);
//  	pr_alert("  DMA Read Req   - %02hhX", log_entry.dma_r_req_log);
//  	pr_alert("  DMA Read Resp  - %02hhX", log_entry.dma_r_resp_log);
//  	pr_alert("  DMA Read Stat  - %02hhX", log_entry.dma_r_stat_log);
//  	pr_alert("  H2C Packet     - %02hhX", log_entry.h2c_pkt_log);
//  	pr_alert("  C2H Packet     - %02hhX", log_entry.c2h_pkt_log);
//  	pr_alert("  Duff Notif     - %02hhX", log_entry.dbuff_notif_log);
//  	pr_alert("  Timer          - %u",     log_entry.timer);
//      }
//     
//      // iowrite32(LOG_CONTROL_DEST, axi_stream_regs + AXI_STREAM_FIFO_TDR);
//  
//      log_control.state = LOG_RECORD;
//  
//      iomov64B((void*) io_regs + 320, (void*) &log_control);
//  
//      // iowrite32(64, axi_stream_regs + AXI_STREAM_FIFO_TLR);
// }

// long    homanic_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);

// TODO eventually this should be an array indexed by port
// void * h2c_msgbuff_cpu_addr;
// dma_addr_t h2c_msgbuff_dma_handle;
// 
// void * c2h_msgbuff_cpu_addr;
// dma_addr_t c2h_msgbuff_dma_handle;
// 
// void * c2h_metadata_cpu_addr;
// dma_addr_t c2h_metadata_dma_handle;


// #define IOCTL_DMA_DUMP 0

// #define LOG_RECORD 0
// #define LOG_DRAIN  1

// extern char _binary_firmware_start[];
// extern char _binary_firmware_end[];

    // .unlocked_ioctl = homanic_ioctl
// void init_mb(void);
// void dump_mb(void);

/* Helper Functions */
// void dump_log(void);






    // switch(iminor(file->f_inode)) {
    // 	case MINOR_H2C_METADATA:
    // 	    pr_alert("device register open");

    // 	    break;
    // 	case MINOR_C2H_METADATA: {
    // 	    c2h_metadata_cpu_addr = dma_alloc_coherent(&(pdev->dev), 16384, &c2h_metadata_dma_handle, GFP_KERNEL);

    // 	    pr_alert("cpu address:%llx\n", (uint64_t) c2h_metadata_cpu_addr);
    // 	    pr_alert("dma address:%llx\n", (uint64_t) c2h_metadata_dma_handle);

    // 	    c2h_port_to_metadata.phys_addr = ((uint64_t) c2h_metadata_dma_handle);
    // 	    c2h_port_to_metadata.port      = ports;
    // 	    c2h_port_to_metadata.type 	   = 2;

    // 	    // onboard_dma_map(&c2h_port_to_metadata);

    // 	    iomov64B((void*) io_regs, (void*) dma_map);

    // 	    break;
    // 	}
    // 	case MINOR_H2C_MSGBUFF:
    // 	    h2c_msgbuff_cpu_addr  = dma_alloc_coherent(&(pdev->dev), 1 * HOMA_MAX_MESSAGE_LENGTH, &h2c_msgbuff_dma_handle, GFP_KERNEL);
    // 	    pr_alert("cpu address:%llx\n", (uint64_t) h2c_msgbuff_cpu_addr);
    // 	    pr_alert("dma address:%llx\n", (uint64_t) h2c_msgbuff_dma_handle);

    // 	    h2c_port_to_msgbuff.phys_addr = ((uint64_t) h2c_msgbuff_dma_handle);
    // 	    h2c_port_to_msgbuff.port      = ports;
    // 	    h2c_port_to_msgbuff.type 	  = 0;

    // 	    onboard_dma_map(&h2c_port_to_msgbuff);

    // 	    break;

    // 	case MINOR_C2H_MSGBUFF:
    // 	    c2h_msgbuff_cpu_addr  = dma_alloc_coherent(&(pdev->dev), 1 * HOMA_MAX_MESSAGE_LENGTH, &c2h_msgbuff_dma_handle, GFP_KERNEL);

    // 	    pr_alert("cpu address:%llx\n", (uint64_t) c2h_msgbuff_cpu_addr);
    // 	    pr_alert("dma address:%llx\n", (uint64_t) c2h_msgbuff_dma_handle);

    // 	    c2h_port_to_msgbuff.phys_addr = ((uint64_t) c2h_msgbuff_dma_handle);
    // 	    c2h_port_to_msgbuff.port      = ports;
    // 	    c2h_port_to_msgbuff.type      = 1;

    // 	    onboard_dma_map(&c2h_port_to_msgbuff);

    // 	    break;
    // }

