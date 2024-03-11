#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/printk.h>
#include <linux/dma-mapping.h>
#include <linux/pci.h>
#include <linux/set_memory.h>
#include <asm/fpu/api.h>
#include <linux/delay.h>

#define _MM_MALLOC_H_INCLUDED
#include <x86intrin.h>

#include <linux/io.h>

#define AXI_CMAC_1 0x30000

#define MINOR_H2C_METADATA 0
#define MINOR_C2H_METADATA 1
#define MINOR_H2C_MSGBUFF  2
#define MINOR_C2H_MSGBUFF  3

#define MAX_MINOR 4

#define HOMA_MAX_MESSAGE_LENGTH 1000000 // The maximum number of bytes in a single message

#define IOCTL_DMA_DUMP 0

#define LOG_RECORD 0
#define LOG_DRAIN  1

struct pci_dev * pdev;
uint32_t BAR_0;

/* device registers for physical address map onboarding AXI-Stream FIFO */
void __iomem * io_regs; 

/* single user address for DMA and for CPU */
// TODO eventually this should be an array indexed by port
void * h2c_msgbuff_cpu_addr;
dma_addr_t h2c_msgbuff_dma_handle;

void * c2h_msgbuff_cpu_addr;
dma_addr_t c2h_msgbuff_dma_handle;

void * c2h_metadata_cpu_addr;
dma_addr_t c2h_metadata_dma_handle;

// TODO this will just cycle around and eventually reallocate ports
uint16_t ports = 1;

struct cfg_t {
    uint32_t unused0; // 4
    char unused1[16]; // 16
    char unused2[16]; // 16
    uint16_t unused3; // 2
    uint16_t unused4; // 2
    uint32_t unused5; // 4
    uint32_t unused6; // 4
    uint64_t unused8; // 8
    uint32_t unused9; // 4
    uint16_t writeBufferSize; // 2
    uint16_t fetchRequestSize; // 2
}__attribute__((packed));

struct port_to_phys_t {
    uint64_t phys_addr;
    uint16_t port;
    uint8_t  type;
    char     pad[53];
}__attribute__((packed));

struct cfg_t cfg __attribute__((aligned(64)));

struct port_to_phys_t h2c_port_to_msgbuff __attribute__((aligned(64)));
struct port_to_phys_t c2h_port_to_msgbuff __attribute__((aligned(64)));
struct port_to_phys_t c2h_port_to_metadata __attribute__((aligned(64)));

// struct log_control_t log_control __attribute__((aligned(64)));
// struct log_entry_t log_entry __attribute__((aligned(64)));

extern char _binary_firmware_start[];
extern char _binary_firmware_end[];

/* Kernel Module Functions */
int     homanic_open(struct inode *, struct file *);
ssize_t homanic_read(struct file *, char *, size_t, loff_t *);
int     homanic_close(struct inode *, struct file *);
int     homanic_mmap(struct file * fp, struct vm_area_struct * vma);
// long    homanic_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);

/* Helper Functions */
void dump_log(void);
void init_eth(void);
void init_mb(void);
void dump_mb(void);
void h2c_new_msgbuff(struct port_to_phys_t * portmap);
void c2h_new_msgbuff(struct port_to_phys_t * portmap);
void c2h_new_metadata(struct port_to_phys_t * portmap);

// https://lore.kernel.org/lkml/157428502934.36836.8119026517510193201.stgit@djiang5-desk3.ch.intel.com/
// https://stackoverflow.com/questions/51918804/generating-a-64-byte-read-pcie-tlp-from-an-x86-cpu
// inline void iomov64B(__m256i * dst, __m256i * src) {
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

struct file_operations homanic_fops = {
    .owner          = THIS_MODULE,
    .open           = homanic_open,
    .release        = homanic_close,
    .mmap           = homanic_mmap,
    // .unlocked_ioctl = homanic_ioctl
};

struct device_data {
    struct cdev cdev;
};

struct device_data devs[4];

struct class * cls;

int dev_major = 0;

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

void onboard_dma_map(struct port_to_phys_t * port_to_phys) {
    iomov64B((void*) io_regs, (void*) port_to_phys);
}

int homanic_open(struct inode * inode, struct file * file) {
    memset(&h2c_port_to_msgbuff, 0xffffffff, 64);
    memset(&c2h_port_to_msgbuff, 0xffffffff, 64);
    memset(&c2h_port_to_metadata, 0xffffffff, 64);

    pr_alert("homanic_open");

    switch(iminor(file->f_inode)) {
	case MINOR_H2C_METADATA:
	    pr_alert("device register open");

	    break;
	case MINOR_C2H_METADATA: {
	    c2h_metadata_cpu_addr = dma_alloc_coherent(&(pdev->dev), 16384, &c2h_metadata_dma_handle, GFP_KERNEL);

	    pr_alert("cpu address:%llx\n", (uint64_t) c2h_metadata_cpu_addr);
	    pr_alert("dma address:%llx\n", (uint64_t) c2h_metadata_dma_handle);

	    c2h_port_to_metadata.phys_addr = ((uint64_t) c2h_metadata_dma_handle);
	    c2h_port_to_metadata.port      = ports;
	    c2h_port_to_metadata.type 	   = 2;

	    onboard_dma_map(&c2h_port_to_metadata);

	    break;
	}
	case MINOR_H2C_MSGBUFF:
	    h2c_msgbuff_cpu_addr  = dma_alloc_coherent(&(pdev->dev), 1 * HOMA_MAX_MESSAGE_LENGTH, &h2c_msgbuff_dma_handle, GFP_KERNEL);
	    pr_alert("cpu address:%llx\n", (uint64_t) h2c_msgbuff_cpu_addr);
	    pr_alert("dma address:%llx\n", (uint64_t) h2c_msgbuff_dma_handle);

	    h2c_port_to_msgbuff.phys_addr = ((uint64_t) h2c_msgbuff_dma_handle);
	    h2c_port_to_msgbuff.port      = ports;
	    h2c_port_to_msgbuff.type 	  = 0;

	    onboard_dma_map(&h2c_port_to_msgbuff);

	    break;

	case MINOR_C2H_MSGBUFF:
	    c2h_msgbuff_cpu_addr  = dma_alloc_coherent(&(pdev->dev), 1 * HOMA_MAX_MESSAGE_LENGTH, &c2h_msgbuff_dma_handle, GFP_KERNEL);

	    pr_alert("cpu address:%llx\n", (uint64_t) c2h_msgbuff_cpu_addr);
	    pr_alert("dma address:%llx\n", (uint64_t) c2h_msgbuff_dma_handle);

	    c2h_port_to_msgbuff.phys_addr = ((uint64_t) c2h_msgbuff_dma_handle);
	    c2h_port_to_msgbuff.port      = ports;
	    c2h_port_to_msgbuff.type      = 1;

	    onboard_dma_map(&c2h_port_to_msgbuff);

	    break;
    }

    return 0;
}

int homanic_close(struct inode * inode, struct file * file) {
    // TODO free up the ID
    // Free the buffers
    // Wipe user data from the device

    switch(iminor(file->f_inode)) {
	case MINOR_H2C_METADATA:
	    printk(KERN_ALERT "h2c metadata close()");
	    // set_memory_wb((uint64_t) h2c_metadata_cpu_addr, 16384 / PAGE_SIZE);
	    // TODO also need to cleanup remap pages?

	    break;

	case MINOR_C2H_METADATA:
	    printk(KERN_ALERT "c2h metadata close()");
	    // set_memory_wb((uint64_t) c2h_metadata_cpu_addr, 16384 / PAGE_SIZE);
	    dma_free_coherent(&(pdev->dev), 16384, c2h_metadata_cpu_addr, c2h_metadata_dma_handle);
	    break;

	case MINOR_H2C_MSGBUFF:
	    printk(KERN_ALERT "h2c msgbuff close()");
	    set_memory_wb((uint64_t) h2c_msgbuff_cpu_addr, (1 * HOMA_MAX_MESSAGE_LENGTH) / PAGE_SIZE);
	    dma_free_coherent(&(pdev->dev), 1 * HOMA_MAX_MESSAGE_LENGTH, h2c_msgbuff_cpu_addr,  h2c_msgbuff_dma_handle);
	    break;

	case MINOR_C2H_MSGBUFF:
	    printk(KERN_ALERT "c2h msgbuff close()");
	    set_memory_wb((uint64_t) c2h_msgbuff_cpu_addr, (1 * HOMA_MAX_MESSAGE_LENGTH) / PAGE_SIZE);
	    dma_free_coherent(&(pdev->dev), 1 * HOMA_MAX_MESSAGE_LENGTH, c2h_msgbuff_cpu_addr,  c2h_msgbuff_dma_handle);
	    break;
    }
    pr_info("homanic_close\n");

    return 0;
}

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


int homanic_mmap(struct file * file, struct vm_area_struct * vma) {
    int ret = 0;

    pr_alert("homanic_mmap\n");

    /* Get the minor number to determine if the caller is interested in
     *     - Control page for device registers
     *     - h2c memory buffer
     *     - c2h memory buffer
     */
    switch(iminor(file->f_inode)) {
	case MINOR_H2C_METADATA:
	    pr_alert("device register remap");

	    ret = remap_pfn_range(vma, vma->vm_start, (BAR_0 + 4096) >> PAGE_SHIFT, 4096, vma->vm_page_prot);
	    // ret = remap_pfn_range(vma, vma->vm_start, BAR_0 >> PAGE_SHIFT, 16384, vma->vm_page_prot);
	    // ret = remap_pfn_range(vma, vma->vm_start, (BAR_0 + AXI_STREAM_AXIL) >> PAGE_SHIFT, 16384, vma->vm_page_prot);
	    // set_memory_wc((uint64_t) vma->vm_start, 16384 / PAGE_SIZE);

	    if (ret != 0) {
		goto exit;
	    }

	    break;

	case MINOR_C2H_METADATA:
	    pr_alert("c2h metadata buffer remap");

	    pr_alert("cpu addr:%llx\n", (uint64_t) c2h_metadata_cpu_addr);
	    pr_alert("dma handle:%llx\n", (uint64_t) c2h_metadata_dma_handle);

	    set_memory_uc((uint64_t) c2h_metadata_cpu_addr, 16384 / PAGE_SIZE);
	    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	    ret = dma_mmap_coherent(&(pdev->dev), vma, c2h_metadata_cpu_addr, c2h_metadata_dma_handle, 16384);

	    if (ret < 0) {
		pr_err("c2h metadata dma_mmap_coherent failed\n");
	    }

	    break;

	case MINOR_H2C_MSGBUFF:
	    pr_alert("h2c msgbuff buffer remap");
	    pr_alert("cpu addr:%llx\n", (uint64_t) h2c_msgbuff_cpu_addr);
	    pr_alert("dma handle:%llx\n", (uint64_t) h2c_msgbuff_dma_handle);

	    set_memory_uc((uint64_t) h2c_msgbuff_cpu_addr, (1 * HOMA_MAX_MESSAGE_LENGTH) / PAGE_SIZE);
	    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	    ret = dma_mmap_coherent(&(pdev->dev), vma, h2c_msgbuff_cpu_addr, h2c_msgbuff_dma_handle, 1 * HOMA_MAX_MESSAGE_LENGTH);

	    if (ret < 0) {
		pr_err("h2c msgbuff dma_mmap_coherent failed\n");
	    }

	    break;

	case MINOR_C2H_MSGBUFF:
	    pr_alert("c2h msgbuff buffer remap");
	    pr_alert("cpu addr:%llx\n", (uint64_t) c2h_msgbuff_cpu_addr);
	    pr_alert("dma handle:%llx\n", (uint64_t) c2h_msgbuff_dma_handle);


	    set_memory_uc((uint64_t) c2h_msgbuff_cpu_addr, (1 * HOMA_MAX_MESSAGE_LENGTH) / PAGE_SIZE);
	    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	    ret = dma_mmap_coherent(&(pdev->dev), vma, c2h_msgbuff_cpu_addr, c2h_msgbuff_dma_handle, 1 * HOMA_MAX_MESSAGE_LENGTH);

	    if (ret < 0) {
		pr_err("c2h msgbuff dma_mmap_coherent failed\n");
	    }

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

    pr_info("homanic_init\n");

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

    err = alloc_chrdev_region(&dev, 0, MAX_MINOR, "homanic");

    if (err != 0) {
	return err;
    }

    dev_major = MAJOR(dev);

    cls = class_create(THIS_MODULE, "homanic");
    device_create(cls, NULL, MKDEV(dev_major, MINOR_H2C_METADATA), NULL, "homa_nic_h2c_metadata");
    device_create(cls, NULL, MKDEV(dev_major, MINOR_C2H_METADATA), NULL, "homa_nic_c2h_metadata");
    device_create(cls, NULL, MKDEV(dev_major, MINOR_H2C_MSGBUFF),  NULL, "homa_nic_h2c_msgbuff");
    device_create(cls, NULL, MKDEV(dev_major, MINOR_C2H_MSGBUFF),  NULL, "homa_nic_c2h_msgbuff");

    cdev_init(&devs[MINOR_H2C_METADATA].cdev, &homanic_fops);
    cdev_init(&devs[MINOR_C2H_METADATA].cdev, &homanic_fops);
    cdev_init(&devs[MINOR_H2C_MSGBUFF].cdev,  &homanic_fops);
    cdev_init(&devs[MINOR_C2H_MSGBUFF].cdev,  &homanic_fops);

    cdev_add(&devs[MINOR_H2C_METADATA].cdev, MKDEV(dev_major, MINOR_H2C_METADATA), 1);
    cdev_add(&devs[MINOR_C2H_METADATA].cdev, MKDEV(dev_major, MINOR_C2H_METADATA), 1);
    cdev_add(&devs[MINOR_H2C_MSGBUFF].cdev,  MKDEV(dev_major, MINOR_H2C_MSGBUFF),  1);
    cdev_add(&devs[MINOR_C2H_MSGBUFF].cdev,  MKDEV(dev_major, MINOR_C2H_MSGBUFF),  1);

    io_regs = ioremap_wc(BAR_0, 0xA0000);

    cfg.fetchRequestSize = 256;
    cfg.writeBufferSize  = 256;

    iomov64B((void*) io_regs + 64, (void *) &cfg);

    // init_eth();

    return 0;
}

void homanic_exit(void) {
    pr_info("homanic_exit\n");

    device_destroy(cls, MKDEV(dev_major, MINOR_H2C_METADATA));
    device_destroy(cls, MKDEV(dev_major, MINOR_C2H_METADATA));
    device_destroy(cls, MKDEV(dev_major, MINOR_H2C_MSGBUFF));
    device_destroy(cls, MKDEV(dev_major, MINOR_C2H_MSGBUFF));
    class_destroy(cls);

    cdev_del(&devs[MINOR_H2C_METADATA].cdev);
    cdev_del(&devs[MINOR_C2H_METADATA].cdev);
    cdev_del(&devs[MINOR_H2C_MSGBUFF].cdev);
    cdev_del(&devs[MINOR_C2H_MSGBUFF].cdev);
    unregister_chrdev_region(MKDEV(dev_major, 0), MAX_MINOR);

    iounmap(io_regs);
}

module_init(homanic_init)
module_exit(homanic_exit)
MODULE_LICENSE("GPL");
