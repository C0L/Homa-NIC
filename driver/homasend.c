#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/printk.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define AXI_STREAM_FIFO_ISR  0x00 // Interrupt Status Register (r/clear on w)
#define AXI_STREAM_FIFO_IER  0x04 // Interrupt Enable Register (r/w)
#define AXI_STREAM_FIFO_TDFR 0x08 // Transmit Data FIFO Reset (w)
#define AXI_STREAM_FIFO_TDFV 0x0C // Transmit Data FIFO Vacancy (r)
#define AXI_STREAM_FIFO_TLR  0x14 // Transmit Length Register (w)
#define AXI_STREAM_FIFO_RDFR 0x18 // Receive Data FIFO Reset (w)
#define AXI_STREAM_FIFO_RDFO 0x1C // Receive Data FIFO Occupancy (r)
#define AXI_STREAM_FIFO_RLR  0x24 // Receive Length Register (r)
#define AXI_STREAM_FIFO_SRR  0x28 // AXI4-Stream Reset (w)
#define AXI_STREAM_FIFO_TDR  0x2C // Transmit Destination Register (w)
#define AXI_STREAM_FIFO_RDR  0x30 // Receive Destination Register (r)

#define AXI_STREAM_FIFO_AXIL_SIZE 0x7C  // Number of bytes for AXIL Interface
// #define AXI_STREAM_FIFO_AXIF_SIZE 0x400 // Number of bytes for AXIF Interface

#define SENDMSG_AXIL_OFFSET 0x00101000
#define SENDMSG_AXIF_OFFSET 0x00104000

#define RECVMSG_AXIL_OFFSET 0x00106000
#define RECVMSG_AXIF_OFFSET 0x00102000

#define BAR_0 0xfe800000

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


int     homasend_open(struct inode *, struct file *);
ssize_t homasend_read(struct file *, char *, size_t, loff_t *);
ssize_t homasend_write(struct file *, const  char *, size_t, loff_t *);
int     homasend_close(struct inode *, struct file *);
struct file_operations homasend_fops = {
    .owner          = THIS_MODULE,
    .open           = homasend_open,
    // .unlocked_ioctl = homasend_ioctl,
    .write          = homasend_write
    //.read           = homasend_read
};

void print_msghdr(struct msghdr_send_t *);

struct homasend_device_data {
    struct cdev cdev;
};

struct homasend_device_data homasend_dd;
struct class * cls;

int dev_major = 0;

void dump_axi_fifo_state(void) {

    /* We need a kernel virtual address as there is no direct way to access physical addresses */

    void __iomem * device_regs = ioremap(BAR_0 + SENDMSG_AXIL_OFFSET, AXI_STREAM_FIFO_AXIL_SIZE);

    // pr_info("0xdead0000: %#x\n", readl(regs));

    pr_alert("Interrupt Status Register   : %x\n", ioread32(device_regs + AXI_STREAM_FIFO_ISR));
    pr_alert("Interrupt Enable Register   : %x\n", ioread32(device_regs + AXI_STREAM_FIFO_IER));
    pr_alert("Transmit Data FIFO Vacancy  : %x\n", ioread32(device_regs + AXI_STREAM_FIFO_TDFV));
    pr_alert("Transmit Length Register    : %x\n", ioread32(device_regs + AXI_STREAM_FIFO_TLR));
    pr_alert("Receive Data FIFO Occupancy : %x\n", ioread32(device_regs + AXI_STREAM_FIFO_RDFO));
    pr_alert("Receive Length Register     : %x\n", ioread32(device_regs + AXI_STREAM_FIFO_RLR));
    pr_alert("Interrupt Status Register   : %x\n", ioread32(device_regs + AXI_STREAM_FIFO_RDR));

    iounmap(device_regs);
}

// TPDP ise GFP_DMA? dma_alloc_coherent?

/* Programming Sequence Using Direct Register Read/Write
 * https://docs.xilinx.com/v/u/4.1-English/pg080-axi-fifo-mm-s
 */
void sendmsg(struct msghdr_send_t * msghdr_send) {
    int i;
    void __iomem * device_regs = ioremap(BAR_0 + SENDMSG_AXIL_OFFSET, AXI_STREAM_FIFO_AXIL_SIZE);
    void __iomem * data_regs = ioremap(BAR_0 + SENDMSG_AXIF_OFFSET, 8);

    void * virt_buff = kmalloc(16384, GFP_ATOMIC);
    phys_addr_t phys_buff = virt_to_phys(virt_buff);

    char msg[13] = "Hello World!";

    memcpy(virt_buff, msg, 13);

    pr_alert("Send Buff Message Contents: %.*s\n", 13, (char *) virt_buff);

    pr_alert("Physical address:%llx\n", phys_buff);
    // pr_alert("Virtual address:%p\n", phys_to_virt(virt_buff));

    msghdr_send->buff_addr = phys_buff;

    print_msghdr(msghdr_send);

    pr_alert("sendmsg call: \n");

    pr_alert("  Resetting Interrput Status Register\n");
    iowrite32(0xffffffff, device_regs + AXI_STREAM_FIFO_ISR);

    pr_alert("  Resetting Interrput Enable Register\n");
    iowrite32(0x0C000000, device_regs + AXI_STREAM_FIFO_IER);

    pr_alert("  Writing sendmsg data\n");
    for (i = 0; i < 16; ++i) {
        iowrite32(*(((unsigned int*) msghdr_send) + i), data_regs);
    }

    pr_alert("  Current sendmsg FIFO vacancy %d\n", ioread32(device_regs + AXI_STREAM_FIFO_TDFV));
    pr_alert("  Draining sendmsg FIFO\n");

    iowrite32(64, device_regs + AXI_STREAM_FIFO_TLR);

    pr_alert("  Current sendmsg FIFO vacancy: %d\n", ioread32(device_regs + AXI_STREAM_FIFO_TDFV));
    pr_alert("  Interrupt Status Register: %d\n",   ioread32(device_regs + AXI_STREAM_FIFO_ISR));

    pr_alert("  Stalling for completed sendmsg\n");

    // TODO Could also check the status registers
    while (ioread32(device_regs + AXI_STREAM_FIFO_RDFO) != 0x00000010);

    // TODO set read size
    pr_alert("  Setting FIFO read size");
    iowrite32(0x00000040, device_regs + AXI_STREAM_FIFO_RLR);

    pr_alert("  Reading sendmsg data\n");
    for (i = 0; i < 16; ++i) {
        *(((unsigned int*) msghdr_send) + i) = ioread32(device_regs + 0x1000);
    }

    print_msghdr(msghdr_send);

    pr_alert("Recv Buff Message Contents: %.*s\n", 13, (char *) (virt_buff + 128));

    iounmap(device_regs);
    iounmap(data_regs);
}

void print_msghdr(struct msghdr_send_t * msghdr_send) {
    pr_alert("sendmsg content dump:\n");
    pr_alert("  SOURCE ADDRESS      : %.*s\n", 16, msghdr_send->saddr);
    pr_alert("  DESTINATION ADDRESS : %.*s\n", 16, msghdr_send->saddr);
    pr_alert("  SOURCE PORT         : %u\n", (unsigned int) msghdr_send->sport);
    pr_alert("  DESTINATION PORT    : %u\n", (unsigned int) msghdr_send->dport);
    pr_alert("  BUFFER ADDRESS      : %llu\n", msghdr_send->buff_addr);
    pr_alert("  BUFFER SIZE         : %u\n", msghdr_send->buff_size);
    pr_alert("  RPC ID              : %llu\n", msghdr_send->id);
    pr_alert("  COMPLETION COOKIE   : %llu\n", msghdr_send->cc);
}

ssize_t homasend_write(struct file * file, const char __user * user_buffer, size_t size, loff_t * offset) {
    struct msghdr_send_t msghdr_send;

    pr_alert("homasend_write\n");

    if (copy_from_user(&msghdr_send, user_buffer, 64))
        return -EFAULT;

    print_msghdr(&msghdr_send);

    // pr_info("virt_to_phys = 0x%llx\n", (unsigned long long) virt_to_phys((void *) msghdr_send.buff_addr));

    sendmsg(&msghdr_send);

    // TODO should use kmap onto user buffer? Then virt to phys

    // msghdr_send->buff_addr = virt_to_phys((void*) msgbuff);

    // dump_axi_fifo_state();
    
    return size;
}

int homasend_open(struct inode * inode, struct file * file) {
    pr_info("homasend_open\n");
    return 0;
}

int homasend_close(struct inode * inode, struct file * file) {
    pr_info("homasend_close\n");
    return 0;
}

int homasend_init(void) {
    dev_t dev;
    int err;

    pr_info("homasend_init\n");

    err = alloc_chrdev_region(&dev, 0, 1, "homasend");

    if (err != 0) {
	return err;
    }

    dev_major = MAJOR(dev);

    cls = class_create(THIS_MODULE, "homasend");
    device_create(cls, NULL, MKDEV(dev_major, 0), NULL, "homasend");

    cdev_init(&homasend_dd.cdev, &homasend_fops);
    cdev_add(&homasend_dd.cdev, MKDEV(dev_major, 0), 1);

    return 0;
}

void homasend_exit(void) {
    pr_info("homasend_exit\n");

    device_destroy(cls, MKDEV(dev_major, 0));
    class_destroy(cls);

    cdev_del(&homasend_dd.cdev);
    unregister_chrdev_region(MKDEV(dev_major, 0), 1);
}

module_init(homasend_init)
module_exit(homasend_exit)
MODULE_LICENSE("GPL");
