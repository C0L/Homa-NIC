#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/printk.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/pci.h>
#include <linux/set_memory.h>
#include <linux/ioctl.h>

#define _MM_MALLOC_H_INCLUDED
#include <immintrin.h>

#define AXI_STREAM_FIFO_ISR  0x00 // Interrupt Status Register (r/clear on w)
#define AXI_STREAM_FIFO_IER  0x04 // Interrupt Enable Register (r/w)
#define AXI_STREAM_FIFO_TDFR 0x08 // Transmit Data FIFO Reset (w)
#define AXI_STREAM_FIFO_TDFV 0x0C // Transmit Data FIFO Vacancy (r)
#define AXI_STREAM_FIFO_TDFD 0x10 // Transmit Data FIFO Write (w)
#define AXI_STREAM_FIFO_TLR  0x14 // Transmit Length Register (w)
#define AXI_STREAM_FIFO_RDFR 0x18 // Receive Data FIFO Reset (w)
#define AXI_STREAM_FIFO_RDFO 0x1C // Receive Data FIFO Occupancy (r)
#define AXI_STREAM_FIFO_RDFD 0x20 // Receive Data FIFO Read (r)
#define AXI_STREAM_FIFO_RLR  0x24 // Receive Length Register (r)
#define AXI_STREAM_FIFO_SRR  0x28 // AXI4-Stream Reset (w)
#define AXI_STREAM_FIFO_TDR  0x2C // Transmit Destination Register (w)
#define AXI_STREAM_FIFO_RDR  0x30 // Receive Destination Register (r)

#define AXI_STREAM_FIFO_AXIL_SIZE 0x7C // Number of bytes for AXIL Interface
#define AXI_STREAM_FIFO_AXIF_SIZE 0x4  // Number of bytes for AXIF Interface

#define AXI_STREAM_AXIL 0x00011000
#define AXI_STREAM_AXIF 0x00012000

#define SENDMSG_DEST     0
#define RECVMSG_DEST     1
#define H2C_P2MSG_DEST   2
#define C2H_P2MSG_DEST   3
#define C2H_P2META_DEST  4
#define LOG_CONTROL_DEST 5

#define BAR_0 0xf4000000

#define MINOR_H2C_METADATA 0
#define MINOR_C2H_METADATA 1
#define MINOR_H2C_MSGBUFF  2
#define MINOR_C2H_MSGBUFF  3

#define MAX_MINOR 4

#define HOMA_MAX_MESSAGE_LENGTH 1000000 // The maximum number of bytes in a single message

#define IOCTL_DMA_DUMP 0

struct pci_dev * pdev;



/* device registers for physical address map onboarding AXI-Stream FIFO */
void __iomem * axi_stream_regs; 
void __iomem * axi_stream_write;
void __iomem * axi_stream_read;

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

struct msghdr_send_t {
    char saddr[16];
    char daddr[16];
    uint16_t sport;
    uint16_t dport;
    uint64_t buff_addr;
    uint32_t buff_size;
    uint64_t id;
    uint64_t cc;
}__attribute__((packed));

struct log_entry_t {
    uint8_t  dma_w_req_log;
    uint8_t  dma_w_stat_log;
    uint8_t  dma_r_req_log;
    uint8_t  dma_r_resp_log;
    uint8_t  dma_r_stat_log;
    uint8_t  h2c_pkt_log;
    uint8_t  c2h_pkt_log;
    char     empty[5];
    uint32_t timer;
}__attribute__((packed));

struct port_to_phys_t {
    uint64_t phys_addr;
    uint32_t port;
    char     pad[52];
}__attribute__((packed));

/* Kernel Module Functions */
int     homanic_open(struct inode *, struct file *);
ssize_t homanic_read(struct file *, char *, size_t, loff_t *);
int     homanic_close(struct inode *, struct file *);
int     homanic_mmap(struct file * fp, struct vm_area_struct * vma);
long    homanic_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);

/* Helper Functions */
void dump_log(void);
void h2c_new_msgbuff(struct port_to_phys_t * portmap);
void c2h_new_msgbuff(struct port_to_phys_t * portmap);
void c2h_new_metadata(struct port_to_phys_t * portmap);

// https://lore.kernel.org/lkml/157428502934.36836.8119026517510193201.stgit@djiang5-desk3.ch.intel.com/
// https://stackoverflow.com/questions/51918804/generating-a-64-byte-read-pcie-tlp-from-an-x86-cpu
inline void iomov64B(__m256i * dst, __m256i * src) {
    __m256i ymm0;
    __m256i ymm1; 

    ymm0 = _mm256_stream_load_si256(src);
    ymm1 = _mm256_stream_load_si256(src+1);

    _mm256_store_si256(dst, ymm0);
    _mm256_store_si256(dst+1, ymm1);
    _mm_mfence();
}

struct file_operations homanic_fops = {
    .owner          = THIS_MODULE,
    .open           = homanic_open,
    .release        = homanic_close,
    .mmap           = homanic_mmap,
    .unlocked_ioctl = homanic_ioctl
};

struct device_data {
    struct cdev cdev;
};

struct device_data devs[4];

struct class * cls;

int dev_major = 0;

void dump_log(void) {
    int i;

    uint32_t rlr;
    uint32_t rdfo;

    struct log_entry_t log_entry;

    pr_alert("Dump Log\n");

    rdfo = ioread32(axi_stream_regs + AXI_STREAM_FIFO_RDFO);
    rlr  = ioread32(axi_stream_regs + AXI_STREAM_FIFO_RLR);

    while (rdfo != 0) {
	// Read 16 bytes of data
	for (i = 0; i < 4; ++i) {
	    *(((unsigned int*) &log_entry) + i) = ioread32(axi_stream_read);
	}
	
	iowrite32(0xffffffff, axi_stream_regs + AXI_STREAM_FIFO_ISR);
	iowrite32(0x0C000000, axi_stream_regs + AXI_STREAM_FIFO_IER);

	rdfo = ioread32(axi_stream_regs + AXI_STREAM_FIFO_RDFO);
	rlr  = ioread32(axi_stream_regs + AXI_STREAM_FIFO_RLR);

	pr_alert("Log Entry: ");
	pr_alert("  DMA Write Req  - %02hhX", log_entry.dma_w_req_log);
	pr_alert("  DMA Write Stat - %02hhX", log_entry.dma_w_stat_log);
	pr_alert("  DMA Read Req   - %02hhX", log_entry.dma_r_req_log);
	pr_alert("  DMA Read Resp  - %02hhX", log_entry.dma_r_resp_log);
	pr_alert("  DMA Read Stat  - %02hhX", log_entry.dma_r_stat_log);
	pr_alert("  H2C Packet     - %02hhX", log_entry.h2c_pkt_log);
	pr_alert("  C2H Packet     - %02hhX", log_entry.c2h_pkt_log);
	pr_alert("  Timer          - %u", log_entry.timer);
    }
}

void c2h_new_metadata(struct port_to_phys_t * port_to_phys) {
    iomov64B((void*) axi_stream_write, (void*) port_to_phys);

    printk(KERN_ALERT "FIFO DEPTH %d\n", ioread32(axi_stream_regs + AXI_STREAM_FIFO_TDFV));

    iowrite32(C2H_P2META_DEST, axi_stream_regs + AXI_STREAM_FIFO_TDR);
    iowrite32(64, axi_stream_regs + AXI_STREAM_FIFO_TLR);

    printk(KERN_ALERT "FIFO DEPTH %d\n", ioread32(axi_stream_regs + AXI_STREAM_FIFO_TDFV));
}

void h2c_new_msgbuff(struct port_to_phys_t * port_to_phys) {
    iomov64B((void*) axi_stream_write, (void*) port_to_phys);

    printk(KERN_ALERT "FIFO DEPTH %d\n", ioread32(axi_stream_regs + AXI_STREAM_FIFO_TDFV));

    iowrite32(H2C_P2MSG_DEST, axi_stream_regs + AXI_STREAM_FIFO_TDR);
    iowrite32(64, axi_stream_regs + AXI_STREAM_FIFO_TLR);

    // printk(KERN_ALERT "ISR: %d\n", ioread32(axi_stream_regs + AXI_STREAM_FIFO_ISR));

    printk(KERN_ALERT "FIFO DEPTH %d\n", ioread32(axi_stream_regs + AXI_STREAM_FIFO_TDFV));
}

void c2h_new_msgbuff(struct port_to_phys_t * port_to_phys) {
    iomov64B((void*) axi_stream_write, (void*) port_to_phys);

    printk(KERN_ALERT "FIFO DEPTH %d\n", ioread32(axi_stream_regs + AXI_STREAM_FIFO_TDFV));

    iowrite32(C2H_P2MSG_DEST, axi_stream_regs + AXI_STREAM_FIFO_TDR);
    iowrite32(64, axi_stream_regs + AXI_STREAM_FIFO_TLR);

    printk(KERN_ALERT "FIFO DEPTH %d\n", ioread32(axi_stream_regs + AXI_STREAM_FIFO_TDFV));
}

int homanic_open(struct inode * inode, struct file * file) {
    // TODO eventually insert these values into an array indexed by the port
    // TODO this will allow for recovery when the mmap is called.
    // TODO How does the port reach the mmap invocation though?

    // int ret;
    return 0;
}

int homanic_close(struct inode * inode, struct file * file) {
    // TODO free up the ID
    // Free the buffers
    // Wipe user data from the device

    int i, j;

    for (i = 0; i < 2; ++i) {
    	printk(KERN_ALERT "Chunk %d: ", i);
    	for (j = 0; j < 64; ++j) printk(KERN_CONT "%02hhX", *(((unsigned char *) c2h_msgbuff_cpu_addr) + j + (i*64)));
    }

    for (i = 0; i < 2; ++i) {
    	printk(KERN_ALERT "Chunk %d: ", i);
    	for (j = 0; j < 64; ++j) printk(KERN_CONT "%02hhX", *(((unsigned char *) h2c_msgbuff_cpu_addr) + j + (i*64)));
    }

    pr_info("homanic_close\n");

    return 0;
}

long homanic_ioctl(struct file * file, unsigned int ioctl_num, unsigned long ioctl_param) {
    int i, j;

    switch(ioctl_num) {
	case IOCTL_DMA_DUMP:
	    dump_log();
	    for (i = 0; i < 8; ++i) {
		printk(KERN_ALERT "Chunk %d: ", i);
		for (j = 0; j < 64; ++j) printk(KERN_CONT "%02hhX", *(((unsigned char *) c2h_msgbuff_cpu_addr) + j + (i*64)));
	    }
	    
	    for (i = 0; i < 8; ++i) {
		printk(KERN_ALERT "Chunk %d: ", i);
		for (j = 0; j < 64; ++j) printk(KERN_CONT "%02hhX", *(((unsigned char *) h2c_msgbuff_cpu_addr) + j + (i*64)));
	    }
    }

    return 0;
}


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

	    ret = remap_pfn_range(vma, vma->vm_start, (BAR_0 + AXI_STREAM_AXIL) >> PAGE_SHIFT, 16384, vma->vm_page_prot);
	    set_memory_wc((uint64_t) vma->vm_start, 1);

	    if (ret != 0) {
		goto exit;
	    }

	    break;

	case MINOR_C2H_METADATA:
	    pr_alert("c2h metadata buffer remap");

	    set_memory_uc((uint64_t) c2h_metadata_cpu_addr, (1 * HOMA_MAX_MESSAGE_LENGTH) / PAGE_SIZE);
	    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	    ret = dma_mmap_coherent(NULL, vma, c2h_metadata_cpu_addr, c2h_metadata_dma_handle, HOMA_MAX_MESSAGE_LENGTH);

	    if (ret < 0) {
		pr_err("c2h metadata dma_mmap_coherent failed\n");
	    }

	    break;

	case MINOR_H2C_MSGBUFF:
	    pr_alert("h2c msgbuff buffer remap");

	    set_memory_uc((uint64_t) h2c_msgbuff_cpu_addr, (1 * HOMA_MAX_MESSAGE_LENGTH) / PAGE_SIZE);
	    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	    ret = dma_mmap_coherent(NULL, vma, h2c_msgbuff_cpu_addr, h2c_msgbuff_dma_handle, 1 * HOMA_MAX_MESSAGE_LENGTH);

	    if (ret < 0) {
		pr_err("h2c msgbuff dma_mmap_coherent failed\n");
	    }

	    break;

	case MINOR_C2H_MSGBUFF:
	    pr_alert("c2h msgbuff buffer remap");

	    set_memory_uc((uint64_t) c2h_msgbuff_cpu_addr, (1 * HOMA_MAX_MESSAGE_LENGTH) / PAGE_SIZE);
	    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	    ret = dma_mmap_coherent(NULL, vma, c2h_msgbuff_cpu_addr, c2h_msgbuff_dma_handle, 1 * HOMA_MAX_MESSAGE_LENGTH);

	    if (ret < 0) {
		pr_err("c2h msgbuff dma_mmap_coherent failed\n");
	    }

	    break;
    }

exit:
    return ret;
}

int homanic_init(void) {

    struct port_to_phys_t h2c_port_to_msgbuff;
    struct port_to_phys_t c2h_port_to_msgbuff;
    struct port_to_phys_t c2h_port_to_metadata;

    dev_t dev;
    int err;

    memset(&h2c_port_to_msgbuff, 0xffffffff, 64);
    memset(&c2h_port_to_msgbuff, 0xffffffff, 64);
    memset(&c2h_port_to_metadata, 0xffffffff, 64);

    pr_info("homanic_init\n");

    pdev = pci_get_device(0x10ee, 0x903f, NULL);

    /* Wake up the device if suspended and allocate IO and mem regions of the device if BIOS did not */
    pci_enable_device(pdev);

    /* Enables DMA by setting the bus master bit in PCI_COMMAND register */
    pci_set_master(pdev);

    pci_set_mwi(pdev);

    dma_set_mask_and_coherent(&(pdev->dev), DMA_BIT_MASK(64));

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

    // TODO pass correct device create to alloc?
    h2c_msgbuff_cpu_addr  = dma_alloc_coherent(NULL, 1 * HOMA_MAX_MESSAGE_LENGTH, &h2c_msgbuff_dma_handle,  GFP_KERNEL);
    c2h_msgbuff_cpu_addr  = dma_alloc_coherent(NULL, 1 * HOMA_MAX_MESSAGE_LENGTH, &c2h_msgbuff_dma_handle,  GFP_KERNEL);
    c2h_metadata_cpu_addr = dma_alloc_coherent(NULL, 16384, &c2h_metadata_dma_handle, GFP_KERNEL);

    pr_alert("cpu address:%llx\n", (uint64_t) h2c_msgbuff_cpu_addr);
    pr_alert("dma address:%llx\n", (uint64_t) h2c_msgbuff_dma_handle);
    pr_alert("cpu address:%llx\n", (uint64_t) c2h_msgbuff_cpu_addr);
    pr_alert("dma address:%llx\n", (uint64_t) c2h_msgbuff_dma_handle);

    axi_stream_regs  = ioremap_wc(BAR_0 + AXI_STREAM_AXIL, AXI_STREAM_FIFO_AXIL_SIZE);
    axi_stream_write = ioremap_wc(BAR_0 + AXI_STREAM_AXIF, 64);
    axi_stream_read  = ioremap_wc(BAR_0 + AXI_STREAM_AXIF + 0x1000, 64);

    iowrite32(0xffffffff, axi_stream_regs + AXI_STREAM_FIFO_ISR);
    iowrite32(0x0C000000, axi_stream_regs + AXI_STREAM_FIFO_IER);

    // TODO eventually be on a per user basis
    h2c_port_to_msgbuff.phys_addr = ((uint64_t) h2c_msgbuff_dma_handle);
    h2c_port_to_msgbuff.port      = ports;

    pr_alert("set port %u\n", ports);

    h2c_new_msgbuff(&h2c_port_to_msgbuff);

    c2h_port_to_msgbuff.phys_addr = ((uint64_t) c2h_msgbuff_dma_handle);
    c2h_port_to_msgbuff.port      = ports;

    c2h_new_msgbuff(&c2h_port_to_msgbuff);

    c2h_port_to_metadata.phys_addr = ((uint64_t) c2h_metadata_dma_handle);
    c2h_port_to_metadata.port      = ports;

    c2h_new_metadata(&c2h_port_to_metadata);

    return 0;
}

void homanic_exit(void) {

    pr_info("homanic_exit\n");

    set_memory_wb((uint64_t) c2h_metadata_cpu_addr, 1);
    set_memory_wb((uint64_t) c2h_msgbuff_cpu_addr, (1 * HOMA_MAX_MESSAGE_LENGTH) / PAGE_SIZE);
    set_memory_wb((uint64_t) h2c_msgbuff_cpu_addr, (1 * HOMA_MAX_MESSAGE_LENGTH) / PAGE_SIZE);

    dma_free_coherent(NULL, 1 * HOMA_MAX_MESSAGE_LENGTH, h2c_msgbuff_cpu_addr,  h2c_msgbuff_dma_handle);
    dma_free_coherent(NULL, 1 * HOMA_MAX_MESSAGE_LENGTH, c2h_msgbuff_cpu_addr,  c2h_msgbuff_dma_handle);
    dma_free_coherent(NULL, 1, c2h_metadata_cpu_addr, c2h_metadata_dma_handle);

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

    iounmap(axi_stream_regs);
    iounmap(axi_stream_read);
    iounmap(axi_stream_write);
}

module_init(homanic_init)
module_exit(homanic_exit)
MODULE_LICENSE("GPL");
