package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._

/* delegate - 512 bit MMIO writes are converter to AXI Streams where
 * the ID field carries the destination address. The destination
 * address determines both the destination user and the desired
 * functionality. Currently that functionality is:
 *     Address 0                  -> New DMA Map
 *     Address 0 + 64             -> Config
 *     Address 0  + (port * 4096) -> sendmsg
 *     Address 64 + (port * 4096) -> recvmsg 
 */
class Delegate extends Module {
  val io = IO(new Bundle {
    val function_i   = Flipped(new axis(512, false, 0, true, 32, false, 0, false)) // 512 bit blocks from user
    val newSendmsg   = Decoupled(new QueueEntry) // Output to sendmsg priority queue
    val newFetchdata = Decoupled(new PrefetcherState) // Output to datafetch queue
    val newDmaMap    = Decoupled(new DmaMap)     // Output to add new DMA map

    val ppEgressSendmsgRamReq  = Decoupled(new RamWriteReq) // Data to write to pp egress RAM
    val ppIngressRecvmsgRamReq = Decoupled(new RamWriteReq) // Data to write to pp ingress RAM

    val c2hMetadataRamReq = Decoupled(new RamWriteReq) // Metadata to write to pcie RAM
    val c2hMetadataDmaReq = Decoupled(new DmaReq) // Data to to write to DMA from metadata RAM

    val dynamicConfiguration = Output(new DynamicConfiguration)
  })

  val dynamicConfiguration = RegInit(0.U.asTypeOf(new DynamicConfiguration))
  io.dynamicConfiguration := dynamicConfiguration

  io.newSendmsg.bits  := 0.U.asTypeOf(new QueueEntry)
  io.newSendmsg.valid := false.B

  io.newFetchdata.bits  := 0.U.asTypeOf(new PrefetcherState)
  io.newFetchdata.valid := false.B

  // By default all output streams are 0 initialized and inactive
  io.ppEgressSendmsgRamReq.bits  := 0.U.asTypeOf(new RamWriteReq)
  io.ppEgressSendmsgRamReq.valid := false.B

  io.ppIngressRecvmsgRamReq.bits  := 0.U.asTypeOf(new RamWriteReq)
  io.ppIngressRecvmsgRamReq.valid := false.B

  io.c2hMetadataRamReq.bits  := 0.U.asTypeOf(new RamWriteReq)
  io.c2hMetadataRamReq.valid := false.B

  io.c2hMetadataDmaReq.bits  := 0.U.asTypeOf(new DmaReq)
  io.c2hMetadataDmaReq.valid := false.B

  io.newDmaMap.bits  := 0.U.asTypeOf(new DmaMap)
  io.newDmaMap.valid := false.B

  // We wish to onboard a new RPC as one operation
  val batchSendmsgReady = io.c2hMetadataDmaReq.ready && io.newSendmsg.ready && io.newFetchdata.ready

  // Data stored in the queue
  class encaps extends Bundle {
    val data = UInt(512.W)
    val user = UInt(32.W)
  }

  /* We want our AXI stream interface to be registered but still
   * operate at full throughput An easy way to do this seems to
   * generate a queue with depth one with "pipe" and "flow" enabled.
   */
  val function_queue = Module(new Queue(new encaps, 1, true, true))
  function_queue.io.enq.bits.data := io.function_i.tdata
  function_queue.io.enq.bits.user := io.function_i.tuser.get
  function_queue.io.enq.valid     := io.function_i.tvalid
  io.function_i.tready            := function_queue.io.enq.ready

  // Be default nothing will exit our queue
  function_queue.io.deq.ready     := false.B

  // The user ID is the page allocated to this user
  val user_id   = function_queue.io.deq.bits.user >> 12
  // The user func is the ofset within that page
  val user_func = function_queue.io.deq.bits.user(11,0)

  val send_id  = RegInit(0.asUInt(16.W))
  val dbuff_id = RegInit(0.asUInt(4.W))

  // Is there a valid registered transaction?
  when(function_queue.io.deq.valid) {
    // Are we talking to the kernel (raw address 0) or a user (port * 4096)
    when (user_id === 0.U) {
      switch (user_func) {
        is (0.U) {
          function_queue.io.deq.ready := io.newDmaMap.ready
          io.newDmaMap.bits           := function_queue.io.deq.bits.data.asTypeOf(new DmaMap)
          io.newDmaMap.valid          := function_queue.io.deq.valid
        }

        is (64.U) {
          function_queue.io.deq.ready := true.B
          dynamicConfiguration        := function_queue.io.deq.bits.data.asTypeOf(new DynamicConfiguration)
        }
      }
    } otherwise {
      // User bits specify the function of this 512 bit block for a particular port
       switch (user_func) {
         // new sendmsg
         is (0.U) {
           val msghdr_send = function_queue.io.deq.bits.data.asTypeOf(new msghdr_t)

           msghdr_send.id := send_id

           io.c2hMetadataRamReq.bits.addr := 64.U * send_id
           io.c2hMetadataRamReq.bits.len  := 64.U
           io.c2hMetadataRamReq.bits.data := msghdr_send.asUInt

           io.ppEgressSendmsgRamReq.bits.addr := 64.U * send_id
           io.ppEgressSendmsgRamReq.bits.len  := 64.U
           io.ppEgressSendmsgRamReq.bits.data := msghdr_send.asUInt

           // Construct a new meta data write request with the newly assigned ID
           io.c2hMetadataDmaReq.bits.pcie_addr := 64.U * msghdr_send.ret
           io.c2hMetadataDmaReq.bits.ram_sel   := 0.U
           io.c2hMetadataDmaReq.bits.ram_addr  := 64.U * send_id
           io.c2hMetadataDmaReq.bits.len       := 64.U
           io.c2hMetadataDmaReq.bits.tag       := 0.U
           io.c2hMetadataDmaReq.bits.port      := user_id

           // Construct a new entry for the packet queue to send packets
           io.newSendmsg.bits.rpc_id    := msghdr_send.id
           io.newSendmsg.bits.dbuff_id  := dbuff_id;
           io.newSendmsg.bits.remaining := msghdr_send.buff_size
           io.newSendmsg.bits.dbuffered := msghdr_send.buff_size
           io.newSendmsg.bits.granted   := 0.U
           io.newSendmsg.bits.priority  := queue_priority.ACTIVE.asUInt;

           // Construct a new entry for the fetch queue to fetch data
           io.newFetchdata.bits.localID  := msghdr_send.id
           io.newFetchdata.bits.dbuffID   := dbuff_id 
           io.newFetchdata.bits.fetchable := msghdr_send.buff_size // TODO 
           io.newFetchdata.bits.totalRem  := msghdr_send.buff_size
           io.newFetchdata.bits.totalLen  := msghdr_send.buff_size

           // Only signal that a transaction is ready to exit our queue if all recievers are ready
           // Ram core will always be ready
           function_queue.io.deq.ready    := batchSendmsgReady

           io.newSendmsg.valid            := function_queue.io.deq.fire
           io.newFetchdata.valid          := function_queue.io.deq.fire
           io.c2hMetadataRamReq.valid     := function_queue.io.deq.fire
           io.c2hMetadataDmaReq.valid     := function_queue.io.deq.fire
           io.ppEgressSendmsgRamReq.valid := function_queue.io.deq.fire

           // Only when te transaction takes place we increment the send and dbuff IDs
           when (function_queue.io.deq.fire) {
             // send_id  := send_id + 1.U
             // dbuff_id := dbuff_id + 1.U
           }
         }

         // new recvmsg
         is (64.U) {
           val msghdr_recv = function_queue.io.deq.bits.data.asTypeOf(new msghdr_t)

           io.ppEgressSendmsgRamReq.bits.data := msghdr_recv.asUInt
           io.ppEgressSendmsgRamReq.bits.addr := 64.U * send_id
           io.ppEgressSendmsgRamReq.bits.len  := 64.U
           io.ppEgressSendmsgRamReq.valid     := true.B
         }
       }
    }
  }
}
