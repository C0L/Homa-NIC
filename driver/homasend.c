#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/printk.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/pci.h>

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

#define SENDMSG_AXIL_OFFSET 0x00101000
#define SENDMSG_AXIF_OFFSET 0x00104000

#define RECVMSG_AXIL_OFFSET 0x00106000
#define RECVMSG_AXIF_OFFSET 0x00102000

#define LOG_AXIL_OFFSET 0x0010D000
#define LOG_AXIF_OFFSET 0x00012000

#define H2C_PORT_TO_PHYS_AXIL_OFFSET 0x0010A000
#define C2H_PORT_TO_PHYS_AXIL_OFFSET 0x0010C000

#define BAR_0 0xf4000000

/* device registers for physical address map onboarding AXI-Stream FIFO */
void __iomem * h2c_physmap_regs;
void __iomem * c2h_physmap_regs;

/* device registers for logging AXI-Stream FIFO */
void __iomem * log_regs;
void __iomem * log_read;

/* device registers for sendmsg AXI-Stream FIFO */
void __iomem * sendmsg_regs; 
void __iomem * sendmsg_write;
void __iomem * sendmsg_read;

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
}__attribute__((packed));

int     homasend_open(struct inode *, struct file *);
ssize_t homasend_read(struct file *, char *, size_t, loff_t *);
ssize_t homasend_write(struct file *, const  char *, size_t, loff_t *);
int     homasend_close(struct inode *, struct file *);
struct file_operations homasend_fops = {
  .owner          = THIS_MODULE,
  .open           = homasend_open,
  .write          = homasend_write
};

void sendmsg(struct msghdr_send_t *);
void print_msghdr(struct msghdr_send_t *);
void dump_log(void);
void h2c_new_physmap(struct port_to_phys_t * portmap);
void c2h_new_physmap(struct port_to_phys_t * portmap);
void dump_axi_fifo_state(void __iomem * device_regs);

struct homasend_device_data {
  struct cdev cdev;
};

// This should be the values returned from lspci
// static const struct pci_device_id pci_id = PCI_DEVICE(0x10ee, 0x903f);

struct homasend_device_data homasend_dd;
struct class * cls;

int dev_major = 0;

/* Programming Sequence Using Direct Register Read/Write
 * https://docs.xilinx.com/v/u/4.1-English/pg080-axi-fifo-mm-s
 */
void sendmsg(struct msghdr_send_t * msghdr_send) {
  int i;
  uint32_t rlr;
  uint32_t rdfo;

  for (i = 0; i < 16; ++i) iowrite32(*(((unsigned int*) msghdr_send) + i), sendmsg_write);

  iowrite32(64, sendmsg_regs + AXI_STREAM_FIFO_TLR);

  rdfo = ioread32(sendmsg_regs + AXI_STREAM_FIFO_RDFO);
  rlr = ioread32(sendmsg_regs + AXI_STREAM_FIFO_RLR);

  pr_alert("sendmsg rdfo: %x\n", rdfo);
  pr_alert("sendmsg rlr: %x\n", rlr);

  // TODO should sanity check rlr and rdfo
  for (i = 0; i < 16; ++i) *(((unsigned int*) msghdr_send) + i) = ioread32(sendmsg_read);
}

void dump_log(void) {
  int i;

  uint32_t rlr;
  uint32_t rdfo;

  struct log_entry_t log_entry;

  rdfo = ioread32(log_regs + AXI_STREAM_FIFO_RDFO);
  rlr = ioread32(log_regs + AXI_STREAM_FIFO_RLR);

  while (rdfo != 0) {
    // Read 16 bytes of data
    for (i = 0; i < 4; ++i) {
      *(((unsigned int*) &log_entry) + i) = ioread32(log_read);
    }
	
    iowrite32(0xffffffff, log_regs + AXI_STREAM_FIFO_ISR);
    iowrite32(0x0C000000, log_regs + AXI_STREAM_FIFO_IER);

    rdfo = ioread32(log_regs + AXI_STREAM_FIFO_RDFO);
    rlr = ioread32(log_regs + AXI_STREAM_FIFO_RLR);

    // TODO interpret these 
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

void h2c_new_physmap(struct port_to_phys_t * port_to_phys) {
  int i;

  for (i = 0; i < 3; ++i) iowrite32(*(((unsigned int*) port_to_phys) + i), h2c_physmap_regs + AXI_STREAM_FIFO_TDFD);

  iowrite32(12, h2c_physmap_regs + AXI_STREAM_FIFO_TLR);
}

void c2h_new_physmap(struct port_to_phys_t * port_to_phys) {
  int i;

  for (i = 0; i < 3; ++i) iowrite32(*(((unsigned int*) port_to_phys) + i), c2h_physmap_regs + AXI_STREAM_FIFO_TDFD);

  iowrite32(12, c2h_physmap_regs + AXI_STREAM_FIFO_TLR);
}


void print_msghdr(struct msghdr_send_t * msghdr_send) {
  pr_alert("sendmsg content dump:\n");
  pr_alert("  - source address      : %.*s\n", 16, msghdr_send->saddr);
  pr_alert("  - destination address : %.*s\n", 16, msghdr_send->saddr);
  pr_alert("  - source port         : %u\n", (unsigned int) msghdr_send->sport);
  pr_alert("  - dest port           : %u\n", (unsigned int) msghdr_send->dport);
  pr_alert("  - buffer address      : %llx\n", msghdr_send->buff_addr);
  pr_alert("  - buffer size         : %u\n", msghdr_send->buff_size);
  pr_alert("  - rpc id              : %llu\n", msghdr_send->id);
  pr_alert("  - completion cookie   : %llu\n", msghdr_send->cc);
}

// TODO Maybe possible to do a 64 byte write from the CPU to PCIe
// https://stackoverflow.com/questions/51918804/generating-a-64-byte-read-pcie-tlp-from-an-x86-cpu

// _mm256_store_si256(pcie_memory_address, ymm0);
// _mm256_store_si256(pcie_memory_address+32, ymm1);
// _mm_mfence();

// TODO virtualize device registers with a large address space of AXI-L registers. Our core interrupters the write operation and queues the data for a particular sender based on that incoming register, then writes that data whenever it gets full. The user has no idea what these addresses are because they are just remapped by the host into the user applications address space.

ssize_t homasend_write(struct file * file, const char __user * user_buffer, size_t size, loff_t * offset) {

  // https://www.kernel.org/doc/html/latest/core-api/dma-api-howto.html

  int i;

  struct msghdr_send_t msghdr_send;
  struct port_to_phys_t port_to_phys;
  //void * h2c_cpu_addr;
  //dma_addr_t h2c_dma_handle;

  //void * c2h_cpu_addr;
  //dma_addr_t c2h_dma_handle;

  void * cpu_addr;
  dma_addr_t dma_handle;

  char pattern[4] = "\xDE\xAD\xBE\xEF";
    
  char msg[512];

  struct pci_dev * pdev = NULL;
  pdev = pci_get_device(0x10ee, 0x903f, NULL);

  // ?
  // pci_enable_capability(pdev, PCI_EXP_DEVCTL_RELAX_EN);
  // pci_enable_capability(pdev, PCI_EXP_DEVCTL_EXT_TAG);

  // pci_set_dma_mask(pdev, DMA_BIT_MASK(64));
  // pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(64));
  
  /* Wake up the device if suspended and allocate IO and mem regions of the device if BIOS did not */
  pci_enable_device(pdev);

  /* Enables DMA by setting the bus master bit in PCI_COMMAND register */
  pci_set_master(pdev);

  pci_set_mwi(pdev);

  // pci_enable_device_mem(pdev);
  dma_set_mask_and_coherent(&(pdev->dev), DMA_BIT_MASK(64));

  cpu_addr = dma_alloc_coherent(&(pdev->dev), 16384, &dma_handle, GFP_ATOMIC);
  // h2c_cpu_addr = kmalloc(16384, GFP_ATOMIC);
  // c2h_cpu_addr = kmalloc(16384, GFP_ATOMIC);

  // h2c_dma_handle = dma_map_single(&(pdev->dev), h2c_cpu_addr, 16384, DMA_TO_DEVICE);
  // c2h_dma_handle = dma_map_single(&(pdev->dev), c2h_cpu_addr, 16384, DMA_FROM_DEVICE);

  pr_alert("cpu address:%llx\n", (uint64_t) cpu_addr);
  pr_alert("dma address:%llx\n", (uint64_t) dma_handle);

  // pr_alert("cpu address:%llx\n", (uint64_t) h2c_cpu_addr);
  // pr_alert("dma address:%llx\n", (uint64_t) h2c_dma_handle);
  // pr_alert("cpu address:%llx\n", (uint64_t) c2h_cpu_addr);
  // pr_alert("dma address:%llx\n", (uint64_t) c2h_dma_handle);

  if (copy_from_user(&msghdr_send, user_buffer, 64))
    return -EFAULT;

  for (i = 0; i < (512/4); ++i) memcpy(msg + (i*4), pattern, 4);

  memcpy(cpu_addr, msg, 512);

  printk(KERN_ALERT "H2C Message Contents: ");

  for (i = 0; i < 512; ++i) printk(KERN_CONT "%02hhX", *(((unsigned char *) cpu_addr) + i));

  port_to_phys.phys_addr = ((uint64_t) dma_handle);
  port_to_phys.port = 1;

  h2c_new_physmap(&port_to_phys);
  port_to_phys.phys_addr = ((uint64_t) dma_handle + 2048);
  c2h_new_physmap(&port_to_phys);
  print_msghdr(&msghdr_send);

  sendmsg(&msghdr_send);

  print_msghdr(&msghdr_send);

  msleep(1000);

  dump_log();

  printk(KERN_ALERT "C2H Message Contents: ");

  // TODO need to do this in both places
  // TODO Occasionally a driver needs to access the contents of a streaming DMA buffer with-
// out unmapping it. A call has been provided to make this possible:
// void dma_sync_single_for_cpu(struct device *dev, dma_handle_t bus_addr,
// size_t size, enum dma_data_direction direction);

  for (i = 0; i < 512; ++i) printk(KERN_CONT "%02hhX", *(((unsigned char *) cpu_addr) + 2048 + i));

  // dma_unmap_single(&(pdev->dev), h2c_dma_handle, 16384, DMA_TO_DEVICE);
  // dma_unmap_single(&(pdev->dev), c2h_dma_handle, 16384, DMA_FROM_DEVICE);

  dma_free_coherent(&(pdev->dev), 16384, cpu_addr, dma_handle);

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

  h2c_physmap_regs = ioremap(BAR_0 + H2C_PORT_TO_PHYS_AXIL_OFFSET, AXI_STREAM_FIFO_AXIL_SIZE);
  c2h_physmap_regs = ioremap(BAR_0 + C2H_PORT_TO_PHYS_AXIL_OFFSET, AXI_STREAM_FIFO_AXIL_SIZE);

  log_regs = ioremap(BAR_0 + LOG_AXIL_OFFSET, AXI_STREAM_FIFO_AXIL_SIZE);
  log_read = ioremap(BAR_0 + LOG_AXIF_OFFSET + 0x1000, 4);

  sendmsg_regs  = ioremap(BAR_0 + SENDMSG_AXIL_OFFSET, AXI_STREAM_FIFO_AXIL_SIZE);
  sendmsg_write  = ioremap(BAR_0 + SENDMSG_AXIF_OFFSET, 4);
  sendmsg_read = ioremap(BAR_0 + SENDMSG_AXIF_OFFSET + 0x1000, 4);

  iowrite32(0xffffffff, log_regs + AXI_STREAM_FIFO_ISR);
  iowrite32(0x0C000000, log_regs + AXI_STREAM_FIFO_IER);

  iowrite32(0xffffffff, c2h_physmap_regs + AXI_STREAM_FIFO_ISR);
  iowrite32(0x0C000000, c2h_physmap_regs + AXI_STREAM_FIFO_IER);

  iowrite32(0xffffffff, h2c_physmap_regs + AXI_STREAM_FIFO_ISR);
  iowrite32(0x0C000000, h2c_physmap_regs + AXI_STREAM_FIFO_IER);

  iowrite32(0xffffffff, sendmsg_regs + AXI_STREAM_FIFO_ISR);
  iowrite32(0x0C000000, sendmsg_regs + AXI_STREAM_FIFO_IER);

  return 0;
}

void homasend_exit(void) {
  pr_info("homasend_exit\n");

  device_destroy(cls, MKDEV(dev_major, 0));
  class_destroy(cls);

  cdev_del(&homasend_dd.cdev);
  unregister_chrdev_region(MKDEV(dev_major, 0), 1);

  iounmap(sendmsg_regs);
  iounmap(sendmsg_read);
  iounmap(sendmsg_write);
  iounmap(log_regs);
  iounmap(log_read);
  iounmap(h2c_physmap_regs);
  iounmap(c2h_physmap_regs);
}

module_init(homasend_init)
module_exit(homasend_exit)
MODULE_LICENSE("GPL");
