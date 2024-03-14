package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util.Decoupled
import chisel3.util._

// TODO if the DMA write logic is moved to the pipeline we can size
// the writes based on the number of incoming data as provided by the
// segment length

/* PPegressStages - combine the egress (out to network) packet
 * processing stages. This involves a lookup to find the data
 * associated with the outgoing RPC (addresses, ports, etc), get the
 * payload data, construct the packet header, and break the packet up
 * into 512 bit chunks to go onto the network. Pipe togethter
 */
class PPegressStages extends Module {
  val io = IO(new Bundle {
    val trigger = Flipped(Decoupled(new QueueEntry)) // Input from sendmsg priority queue
    val egress  = new axis(512, false, 0, false, 0, true, 64, true) // Output to ethernet

    val cb_ram_read_desc      = Decoupled(new RamReadReq) // Read descriptors for sendmsg control blocks
    val cb_ram_read_data      = Flipped(Decoupled(new RamReadResp)) // Control block returned from read

    val payload_ram_read_desc = Decoupled(new RamReadReq) // Read descriptors for packet payload data
    val payload_ram_read_data = Flipped(Decoupled(new RamReadResp)) // Payload data returned from read

    val newCacheFree = Decoupled(UInt(10.W)) // Opens the cache window by freeing one packet chunk for buffer ID
  })

  val pp_lookup  = Module(new PPegressLookup)  // Lookup the control block
  val pp_ctor    = Module(new PPegressCtor) // Construct the packet header from control block data // TODO Move this after the lookup
  val pp_dupe    = Module(new PPegressDupe) // Generate a packet factory for each 64 bytes of packet
  val pp_payload = Module(new PPegressPayload) // Grab the payload data for this packet factory
  val pp_xmit    = Module(new PPegressXmit) // Contruct 64B chunks from packet factory for network

  // Link all the units together
  pp_lookup.io.packet_in  <> io.trigger
  pp_ctor.io.packet_in    <> pp_lookup.io.packet_out
  pp_dupe.io.packet_in    <> pp_ctor.io.packet_out
  pp_payload.io.packet_in <> pp_dupe.io.packet_out
  pp_xmit.io.packet_in    <> pp_payload.io.packet_out
  io.egress               <> pp_xmit.io.egress

  // Hook control block core up to the control block RAM
  pp_lookup.io.ram_read_desc        <> io.cb_ram_read_desc
  pp_lookup.io.ram_read_data        <> io.cb_ram_read_data

  // Hook payload core up to the payload RAM managed by fetch queue
  pp_payload.io.ramReadReq  <> io.payload_ram_read_desc
  pp_payload.io.ramReadData <> io.payload_ram_read_data
  pp_payload.io.newCacheFree <> io.newCacheFree

  // val pp_egress_ila = Module(new ILA(new axis(512, false, 0, false, 0, true, 64, true)))
  // pp_egress_ila.io.ila_data := io.egress

  // val pp_egress_trigger_ila = Module(new ILA(Decoupled(new QueueEntry)))
  // pp_egress_trigger_ila.io.ila_data := io.trigger

  val pp_egress_lookup_ila = Module(new ILA(Decoupled(new PacketFrameFactory)))
  pp_egress_lookup_ila.io.ila_data := pp_lookup.io.packet_out

  val pp_egress_dupe_ila = Module(new ILA(Decoupled(new PacketFrameFactory)))
  pp_egress_dupe_ila.io.ila_data := pp_dupe.io.packet_out

  // val pp_egress_payload_ila = Module(new ILA(Decoupled(new PacketFrameFactory)))
  // pp_egress_payload_ila.io.ila_data := pp_payload.io.packet_out

  // val pp_egress_ctor_ila = Module(new ILA(Decoupled(new PacketFrameFactory)))
  // pp_egress_ctor_ila.io.ila_data := pp_ctor.io.packet_out
}

/* PPegressLookup - take an input trigger from the sendmsg queue and
 * lookup the associated control block. 
 */ 
class PPegressLookup extends Module {
  val io = IO(new Bundle {
    val ram_read_desc = Decoupled(new RamReadReq) // Read descriptors for sendmsg control blocks
    val ram_read_data = Flipped(Decoupled(new RamReadResp)) // Control block returned from read

    val packet_in  = Flipped(Decoupled(new QueueEntry)) // Output from sendmsg queue
    val packet_out = Decoupled(new PacketFrameFactory) // Packet factory associated with this queue output 
  })
  /* Computation occurs between register stage 0 and 1
   */
  val packet_reg_0 = Module(new Queue(new QueueEntry, 1, true, false))
  val pending      = Module(new Queue(new PacketFrameFactory, 12, true, false))
  val packet_reg_1 = Module(new Queue(new PacketFrameFactory, 1, true, false))

  // Tie register stages to input and output 
  packet_reg_0.io.enq <> io.packet_in
  io.packet_out       <> packet_reg_1.io.deq

  /*
   *  Tie stage 0 to delay queue
   *  Dispatch the ram read request
   */ 
  pending.io.enq.bits         := 0.U.asTypeOf(new PacketFrameFactory)
  pending.io.enq.bits.trigger := packet_reg_0.io.deq.bits
  pending.io.enq.valid        := packet_reg_0.io.deq.valid
  packet_reg_0.io.deq.ready   := pending.io.enq.ready

  io.ram_read_desc.bits      := 0.U.asTypeOf(new RamReadReq)
  io.ram_read_desc.valid     := packet_reg_0.io.deq.fire
  io.ram_read_desc.bits.addr := 64.U * packet_reg_0.io.deq.bits.rpc_id
  io.ram_read_desc.bits.len  := 64.U

  /*
   * Tie delay queue to stage 1
   * Wait for ram request to return (acts as enable signal for transaction)
   * Dequeue if the downstream can accept and we have ram data
   * We are ready if there are no more chunks to dequeue
   */
  pending.io.deq.ready      := packet_reg_1.io.enq.ready && io.ram_read_data.valid
  packet_reg_1.io.enq.valid := pending.io.deq.valid && io.ram_read_data.valid

  // If the RAM read has data there is always data in queue. That will
  // be emptied when the downstream is availible
  io.ram_read_data.ready := packet_reg_1.io.enq.ready

  packet_reg_1.io.enq.bits    := pending.io.deq.bits
  packet_reg_1.io.enq.bits.cb := io.ram_read_data.bits.data.asTypeOf(new msghdr_t)
}

/* PPegressDupe - convert a single outgoing packet factory into one
 * for each 64B of packet data. 
 */
class PPegressDupe extends Module {
  val io = IO(new Bundle {
    val packet_in  = Flipped(Decoupled(new PacketFrameFactory)) // Input packet factory
    val packet_out = Decoupled(new PacketFrameFactory) // Output duplicated packet factories
  })

  // Tag each duplicated packet factory out with 64 byte increments
  val frame_off = RegInit(0.U(32.W))

  /* Computation occurs between register stage 0 and 1
   */
  val packet_reg_0 = Module(new Queue(new PacketFrameFactory, 1, true, false))
  val packet_reg_1 = Module(new Queue(new PacketFrameFactory, 1, true, false))

  // Tie register stages to input and output 
  packet_reg_0.io.enq <> io.packet_in
  io.packet_out       <> packet_reg_1.io.deq

  /*
   * Only remove the element from the first stage if we have sent all
   * the frames and receiver is ready
   */
  // TODO Move some of this logic out

  packet_reg_0.io.deq.ready          := packet_reg_1.io.enq.ready && (frame_off >= (packet_reg_0.io.deq.bits.packetBytes() - 64.U))
  packet_reg_1.io.enq.valid          := packet_reg_0.io.deq.valid
  packet_reg_1.io.enq.bits           := packet_reg_0.io.deq.bits
  packet_reg_1.io.enq.bits.frame_off := frame_off

  /* When a new frame is accepted then reset frame offset When a new
   * frame is removed then increment the frame offset
   */
  when (io.packet_in.fire) { 
    frame_off := 0.U
  }.elsewhen (packet_reg_1.io.enq.ready && packet_reg_0.io.deq.valid) {
    frame_off := frame_off + 64.U
  }
}

/* PPegressPayload - takes an input packet factory, locates the
 * associated payload chunk associated with the segment offset and
 * frame and emits the packet factory to the next stage.
 */
class PPegressPayload extends Module {
  val io = IO(new Bundle {
    val ramReadReq  = Decoupled(new RamReadReq) // Read descriptors for payload data
    val ramReadData = Flipped(Decoupled(new RamReadResp)) // Payload data returned

    val packet_in  = Flipped(Decoupled(new PacketFrameFactory)) // Input packet factory
    val packet_out = Decoupled(new PacketFrameFactory) // Output packet factory data with 64B of payload

    val newCacheFree = Decoupled(UInt(10.W)) // Opens the cache window by freeing one packet chunk for buffer ID
  })

  /* Computation occurs between register stage 0 and 1
   */
  val packet_reg_0 = Module(new Queue(new PacketFrameFactory, 1, true, false))
  val pending = Module(new Queue(new PacketFrameFactory, 18, true, false)) // TODO cross domain crossing heavy penatly
  val packet_reg_1 = Module(new Queue(new PacketFrameFactory, 1, true, false))

  // Tie register stages to input and output 
  packet_reg_0.io.enq <> io.packet_in
  io.packet_out       <> packet_reg_1.io.deq 

  /*
   *  Tie stage 0 to delay queue
   *  Dispatch the ram read request
   */ 
  pending.io.enq <> packet_reg_0.io.deq

  // TODO no flow control on this path. Frees 1 payload worth of bytes
  io.newCacheFree.bits  := packet_reg_0.io.deq.bits.trigger.dbuff_id
  // io.newCacheFree.valid := packet_reg_0.io.deq.valid && packet_reg_0.io.deq.bits.lastFrame() === 1.U
  io.newCacheFree.valid := false.B // TODO

  io.ramReadReq.bits := 0.U.asTypeOf(new RamReadReq)

  // The cache line offset for this packet is determined by the cache ID times line size
  val cacheOffset = (packet_reg_0.io.deq.bits.trigger.dbuff_id * CacheCfg.lineSize.U)

  /* The offset of the data segment for this packet
   * NOTE: The cache is a circular buffer within individual cache lines
   */
  val segmentOffset = (packet_reg_0.io.deq.bits.cb.buff_size - packet_reg_0.io.deq.bits.trigger.remaining)

  // The offset of the data within the segment to be stored in this current frame
  val payloadOffset = packet_reg_0.io.deq.bits.payloadOffset()

  /* The final offset of the payload is the cache line offset plus the
   * offset of the currently desired payload. Each cacheline is a
   * circular buffer so we take the payload offset modulo the size of
   * the cache line.
   */
  val finalOffset = cacheOffset + ((segmentOffset + payloadOffset) % CacheCfg.lineSize.U)

  /* The ram read operation is dispatched when the packet factory moves from initial register stage to pending. 
   * NOTE: This assumes the ram read core is always ready
   */
  io.ramReadReq.valid     := packet_reg_0.io.deq.fire
  io.ramReadReq.bits.addr := finalOffset
  io.ramReadReq.bits.len  := packet_reg_0.io.deq.bits.payloadBytes()

  /*
   * Tie delay queue to stage 1
   * Wait for ram request to return (acts as enable signal for transaction)
   * Dequeue if the downstream can accept and we have ram data
   * We are ready if there are no more chunks to dequeue
   */
  pending.io.deq.ready      := packet_reg_1.io.enq.ready && io.ramReadData.valid 
  packet_reg_1.io.enq.valid := pending.io.deq.valid && io.ramReadData.valid

  // If the RAM read has data there is always data in queue. That will
  // be emptied when the downstream is availible
  io.ramReadData.ready := packet_reg_1.io.enq.ready

  packet_reg_1.io.enq.bits         := pending.io.deq.bits
  packet_reg_1.io.enq.bits.payload := io.ramReadData.bits.data
}

/* PPegressCtor - takes an input packet factory and computes/fills
 * the packet header fields based on control block data
 */
class PPegressCtor extends Module {
  val io = IO(new Bundle {
     val packet_in  = Flipped(Decoupled(new PacketFrameFactory)) // Input packet factory
     val packet_out = Decoupled(new PacketFrameFactory) // Output packet factory with header fields filled
  })

  /* Computation occurs between register stage 0 and 1
   */
  val packet_reg_0 = Module(new Queue(new PacketFrameFactory, 1, true, false))
  val packet_reg_1 = Module(new Queue(new PacketFrameFactory, 1, true, false))

  /* If there is more than 1 packet remaining, send 1 packet's worth
   * Otherwise send the remainder (remaining)
   */
  val seg_len = Wire(UInt(32.W))

  when (packet_reg_0.io.deq.bits.trigger.remaining > NetworkCfg.payloadBytes.U) {
    seg_len := NetworkCfg.payloadBytes.U
  }.otherwise {
    seg_len := packet_reg_0.io.deq.bits.trigger.remaining
  }
   
  val offset  = packet_reg_0.io.deq.bits.cb.buff_size - packet_reg_0.io.deq.bits.trigger.remaining

  // Tie register stages to input and output 
  packet_reg_0.io.enq <> io.packet_in
  io.packet_out       <> packet_reg_1.io.deq

  packet_reg_0.io.deq <> packet_reg_1.io.enq

  packet_reg_1.io.enq.bits.eth.mac_dest  := MAC.dst
  packet_reg_1.io.enq.bits.eth.mac_src   := MAC.src
  packet_reg_1.io.enq.bits.eth.ethertype := IPV6.ethertype

  packet_reg_1.io.enq.bits.ipv6.version     := IPV6.version
  packet_reg_1.io.enq.bits.ipv6.traffic     := IPV6.traffic
  packet_reg_1.io.enq.bits.ipv6.flow        := IPV6.flow
  packet_reg_1.io.enq.bits.ipv6.payload_len := 0.U // TODO 
  packet_reg_1.io.enq.bits.ipv6.next_header := IPV6.ipproto_homa
  packet_reg_1.io.enq.bits.ipv6.hop_limit   := IPV6.hop_limit
  packet_reg_1.io.enq.bits.ipv6.saddr       := packet_reg_0.io.deq.bits.cb.saddr
  packet_reg_1.io.enq.bits.ipv6.daddr       := packet_reg_0.io.deq.bits.cb.daddr

  packet_reg_1.io.enq.bits.common.sport     := packet_reg_0.io.deq.bits.cb.sport
  packet_reg_1.io.enq.bits.common.dport     := packet_reg_0.io.deq.bits.cb.dport
  packet_reg_1.io.enq.bits.common.doff      := HOMA.doff
  packet_reg_1.io.enq.bits.common.homatype  := HOMA.DATA
  packet_reg_1.io.enq.bits.common.checksum  := 0.U // TODO 
  packet_reg_1.io.enq.bits.common.sender_id := packet_reg_0.io.deq.bits.cb.id

  packet_reg_1.io.enq.bits.data.data.offset    := offset
  packet_reg_1.io.enq.bits.data.data.seg_len   := seg_len
  packet_reg_1.io.enq.bits.data.data.ack.id    := 0.U
  packet_reg_1.io.enq.bits.data.data.ack.sport := 0.U
  packet_reg_1.io.enq.bits.data.data.ack.dport := 0.U
}

/* PPegressXmit - Generate 64 byte chunks for the ethernet core from
 * a packet factory. The frame value inside the packet factory
 * determines which chunk will be sent.
 */
class PPegressXmit extends Module {
  val io = IO(new Bundle {
    val packet_in  = Flipped(Decoupled(new PacketFrameFactory))
    val egress  = new axis(512, false, 0, false, 0, true, 64, true) // Output to ethernet
  })

  val packet_reg = Module(new Queue(new PacketFrameFactory, 1, true, false))
  packet_reg.io.enq <> io.packet_in
  packet_reg.io.deq.ready := io.egress.tready
  io.egress.tvalid := packet_reg.io.deq.valid

  io.egress.tdata     := packet_reg.io.deq.bits.wireFrame()
  io.egress.tlast.get := packet_reg.io.deq.bits.lastFrame()
  io.egress.tkeep.get := packet_reg.io.deq.bits.keepBytes()
}

/* PPingressStages - combine the ingress (coming from network)
 * stages of the packet processing pipeline. This involves parsing the
 * incoming 64 byte frames into a packet factory, mapping the received
 * packeted to a local ID, using that local ID to look up the control
 * block associated, and storing the associated payload via DMA
 *
 * TODO final output to packet map stage?
 */
class PPingressStages extends Module {
  val io = IO(new Bundle {
    val dynamicConfiguration = Input(new DynamicConfiguration)

    val cb_ram_read_desc = Decoupled(new RamReadReq) // Read descriptors for recvmsg control blocks
    val cb_ram_read_data = Flipped(Decoupled(new RamReadResp)) // Data read from descriptor request

    val c2hPayloadRamReq = Decoupled(new RamWriteReq) // Data to write to BRAM

    val c2hPayloadDmaReq  = Decoupled(new DmaReq) // Descriptors to pcie to init request

    val ingress    = Flipped(new axis(512, false, 0, false, 0, true, 64, true)) // Input from ethernet
    // val dma_w_data = Decoupled(new dma_write_t) // Writes to the DMA core
  })

  val pp_dtor    = Module(new PPingressDtor) // Convert 64 byte network chunks into packet factories
  val pp_map     = Module(new PPingressMap) // Map the unique local ID associated with packet
  val pp_lookup  = Module(new PPingressLookup) // Lookup the associated data of that local ID
  val pp_payload = Module(new PPingressPayload) // Send the data from that packet factory over DMA

  // val lookup_desc_ila = Module(new ILA(new RamReadReq))
  // lookup_desc_ila.io.ila_data := io.cb_ram_read_desc.bits

  // val lookup_data_ila = Module(new ILA(new RamReadResp))
  // lookup_data_ila.io.ila_data := io.cb_ram_read_data.bits

  // Link all the units together
  pp_dtor.io.ingress      <> io.ingress
  pp_map.io.packet_in     <> pp_dtor.io.packet_out
  pp_lookup.io.packet_in  <> pp_map.io.packet_out
  pp_payload.io.packet_in <> pp_lookup.io.packet_out

  pp_payload.io.dynamicConfiguration <> io.dynamicConfiguration
  pp_payload.io.c2hPayloadDmaReq  <> io.c2hPayloadDmaReq
  // pp_payload.io.c2hPayloadDmaStat <> io.c2hPayloadDmaStat
  pp_payload.io.c2hPayloadRamReq <> io.c2hPayloadRamReq 

  // Conncet to DMA core
  // pp_payload.io.dma_w_data <> io.dma_w_data

  // Connect control block lookup descriptors
  pp_lookup.io.ram_read_desc        <> io.cb_ram_read_desc
  pp_lookup.io.ram_read_data        <> io.cb_ram_read_data

  val pp_ingress_ila = Module(new ILA(new axis(512, false, 0, false, 0, true, 64, true)))
  pp_ingress_ila.io.ila_data := io.ingress

  // val pp_dtor_ila = Module(new ILA(Decoupled(new PacketFrameFactory)))
  // pp_dtor_ila.io.ila_data := pp_dtor.io.packet_out

  val pp_lookup_ila = Module(new ILA(Decoupled(new PacketFrameFactory)))
  pp_lookup_ila.io.ila_data := pp_lookup.io.packet_out
}

/* PPingressDtor - take 64 byte chunks from off the network and
 * convert them into packet factories. For each block of payload we
 * emit another packet factory
 */
class PPingressDtor extends Module {
  val io = IO(new Bundle {
    val ingress    = Flipped(new axis(512, false, 0, false, 0, true, 64, true)) // Input from ethernet
    val packet_out = Decoupled(new PacketFrameFactory) // Constructed packet factory to the next stage
  })

  val processed = RegInit(0.U(32.W)) // Number of bytes we have received so far for this packet
  val pkt       = Reg(new PacketFrameFactory) // Packet factory we are building as data arrives
  val pktvalid  = RegInit(false.B) // Whether the packet factory should progress to the next stage

  // The output to the next stage is a registed packet factory and registered valid signal
  io.packet_out.valid := pktvalid
  io.packet_out.bits  := pkt

  // TODO this is probably not good
  io.ingress.tready := io.packet_out.ready

  val length = (new PacketFrameFactory).getWidth

  /* State machine for parsing packets
   * If there is data from ingress (64 byte chunks coming from the
   * network), we make a decision as to what to do with the data based
   * on how much data we have buffered already which is stored in the
   * processed register. If there are 0 bytes processed, this is the
   * first 64 bytes of the header and we drop that in the first 64
   * bytes of the packet factory. If there are 64 bytes processed,
   * this is the second 64 bytes of the header and we drop that in the
   * second 64 bytes of the packet factory. Otherwise, this is purely
   * data payload and we just store the entire thing in the payload
   * section of the packet factory. When we receive a tlast signal
   * from ingress that is the completition of a packet and we can
   * clear the processed register to prepare for the next packet. We
   * also register the valid signal to delay it by one cycle to match
   * up with the data being added to the packet factory we are
   * constructing.
   */

  // TODO can some of this be moved into packet factory?
  when ((pktvalid === false.B || io.packet_out.ready === 1.U) && io.ingress.tvalid) {
    when (processed === 0.U) {
      pkt := Cat(io.ingress.tdata, 0.U(((new PacketFrameFactory).getWidth - 512).W)).asTypeOf(new PacketFrameFactory)
      pkt.frame_off := processed
      pktvalid := false.B
    }.elsewhen (processed === 64.U) {
      // We know that packet_out was ready so this will send data
      pkt := Cat(pkt.asUInt(length-1,length-512), io.ingress.tdata, 0.U(((new PacketFrameFactory).getWidth - 1024).W)).asTypeOf(new PacketFrameFactory)

      pkt.frame_off := processed
      pktvalid  := true.B
    }.otherwise {
      // Fill the payload section
      pktvalid := true.B
      pkt.payload := io.ingress.tdata
      pkt.frame_off := processed
    }

    when (io.ingress.tlast.get) {
      processed := 0.U 
    }.otherwise {
      processed := processed + 64.U
    }
  }.elsewhen (io.packet_out.fire) {
    pktvalid := false.B
  }
}

/* PPingressMap - Take an input packet and either find its mapping
 * to a local ID or create a mapping.
 * TODO: unimplemented
 */
class PPingressMap extends Module {
  val io = IO(new Bundle {
    val packet_in  = Flipped(Decoupled(new PacketFrameFactory)) // Input packet factory
    val packet_out = Decoupled(new PacketFrameFactory) // Output packet factory with local ID
  })

  io.packet_in <> io.packet_out

  // TODO temp
  // val id = RegInit(0.U(16.W))

  // 128 bits source address + 16 bit source port + 64 bit RPC ID
  // val cam = Module(new CAM(128 + 16 + 64, 16))

  // val searchInputReg  = Module(new Queue(new PacketFrameFactory, 1, true, false))
  // val searchPending   = Module(new Queue(new PacketFrameFactory, 6, true, false))
  // val searchOutputReg = Module(new Queue(new PacketFrameFactory, 1, true, false))

  // // Tie register stages to input and output 
  // searchInputReg.io.enq <> io.packet_in
  // io.packet_out         <> searchOutputReg.io.deq

  // /*
  //  *  Tie stage 0 to delay queue
  //  *  Dispatch the ram read request
  //  */ 
  // searchPending.io.enq <> searchInputReg.io.deq

  // cam.io.search.bits.key   := Cat(searchInputReg.io.deq.bits.ipv6.saddr, searchInputReg.io.deq.bits.common.sport, searchInputReg.io.deq.bits.common.sender_id)
  // cam.io.search.bits.value := id
  // cam.io.search.bits.set   := 1.U
  // cam.io.search.valid      := searchInputReg.io.deq.fire

  // /*
  //  * Tie delay queue to stage 1
  //  * Wait for ram request to return (acts as enable signal for transaction)
  //  * Dequeue if the downstream can accept and we have ram data
  //  * We are ready if there are no more chunks to dequeue
  //  */
  // searchPending.io.deq.ready   := searchOutputReg.io.enq.ready && cam.io.result.valid
  // cam.io.result.ready          := searchOutputReg.io.enq.ready
  // searchOutputReg.io.enq.valid := cam.io.result.valid
  // searchOutputReg.io.enq.bits  := searchPending.io.deq.bits

  // cam.io.insert.bits  := 0.U.asTypeOf(new CAMEntry(128 + 16 + 64, 16))
  // cam.io.insert.valid := false.B

  // // When the CAM lookup has completed check if the search was sucessful
  // when (searchPending.io.deq.fire) {
  //   when (cam.io.result.bits.set =/= 0.U) {
  //     // Fill in the ID
  //     searchOutputReg.io.enq.bits.cb.id := cam.io.result.bits.value
  //   }.otherwise {
  //     // TODO and insert
  //     // Generate the ID and increment
  //     searchOutputReg.io.enq.bits.cb.id := id

  //     cam.io.insert.bits.key   := Cat(searchPending.io.deq.bits.ipv6.saddr, searchPending.io.deq.bits.common.sport, searchPending.io.deq.bits.common.sender_id)
  //     cam.io.insert.bits.value := id
  //     cam.io.insert.bits.set   := 1.U
  //     cam.io.insert.valid := true.B

  //     id := id + 1.U
  //   }
  // }
}

/* PPingressLookup - Look up the associated recv control block with
 * this local ID to know where to direct packet data.
 */
class PPingressLookup extends Module {
  val io = IO(new Bundle {
    val ram_read_desc = Decoupled(new RamReadReq) // Read descriptors for recvmsg control blocks
    val ram_read_data = Flipped(Decoupled(new RamReadResp)) // Control block returned from read

    val packet_in  = Flipped(Decoupled(new PacketFrameFactory)) // Packet factory input 
    val packet_out = Decoupled(new PacketFrameFactory) // Packet factory output with control block added
  })

  // println((io.packet_in.bits.eth.getWidth + io.packet_in.bits.ipv6.getWidth + io.packet_in.bits.common.getWidth + io.packet_in.bits.data.getWidth)/8);

  /* Computation occurs between register stage 0 and 1
   */
  val packet_reg_0 = Module(new Queue(new PacketFrameFactory, 1, true, false))
  val pending = Module(new Queue(new PacketFrameFactory, 6, true, false))
  val packet_reg_1 = Module(new Queue(new PacketFrameFactory, 1, true, false))

  // Tie register stages to input and output 
  packet_reg_0.io.enq <> io.packet_in
  io.packet_out       <> packet_reg_1.io.deq

  /*
   *  Tie stage 0 to delay queue
   *  Dispatch the ram read request
   */ 
  pending.io.enq <> packet_reg_0.io.deq

  io.ram_read_desc.bits     := 0.U.asTypeOf(new RamReadReq)

  // Construct a read descriptor for this piece of data
  io.ram_read_desc.valid     := packet_reg_0.io.deq.fire
  io.ram_read_desc.bits.addr := 64.U * packet_reg_0.io.deq.bits.cb.id // TODO this is not defined!
  io.ram_read_desc.bits.len  := 64.U

  /*
   * Tie delay queue to stage 1
   * Wait for ram request to return (acts as enable signal for transaction)
   * Dequeue if the downstream can accept and we have ram data
   * We are ready if there are no more chunks to dequeue
   */
  pending.io.deq.ready      := packet_reg_1.io.enq.ready && io.ram_read_data.valid
  io.ram_read_data.ready    := packet_reg_1.io.enq.ready
  packet_reg_1.io.enq.valid := io.ram_read_data.valid

  // If the RAM read has data there is always data in queue. That will
  // be emptied when the downstream is availible
  packet_reg_1.io.enq.bits    := pending.io.deq.bits
  packet_reg_1.io.enq.bits.cb := io.ram_read_data.bits.data.asTypeOf(new msghdr_t)
}

/* PPingressPaylaod - take an input packet factory and extract the
 * payload data, determine the destination for the payload data, send
 * a dma write request.
 *
 * TODO: eventually expose more of the RAM interface to this core
 */
class PPingressPayload extends Module {
  val io = IO(new Bundle {
    val dynamicConfiguration = Input(new DynamicConfiguration)

    val c2hPayloadRamReq = Decoupled(new RamWriteReq) // Data to write to BRAM
    val c2hPayloadDmaReq  = Decoupled(new DmaReq) // Descriptors to pcie to init request

    val packet_in  = Flipped(Decoupled(new PacketFrameFactory)) // Input packet factory with data to write
  })

  val packet_reg_0 = Module(new Queue(new PacketFrameFactory, 1, true, false))

  packet_reg_0.io.enq <> io.packet_in

  /* The RAM core is always ready = True, so we only perform
   * a transaction when there is valid data in queue and
   * the DAM core is ready to accept a request
   */
  packet_reg_0.io.deq.ready := io.c2hPayloadDmaReq.ready

  io.c2hPayloadRamReq.bits  := 0.U.asTypeOf((new RamWriteReq))
  io.c2hPayloadRamReq.valid := false.B

  io.c2hPayloadDmaReq.bits  := 0.U.asTypeOf((new DmaReq))
  io.c2hPayloadDmaReq.valid := false.B

  // !! TODO maybe need to realign on 64 byte boundaries!!!!

  /* Two events trigger a DMA write request
   *   1) Last frame of packet
   *   2) Reach io.dynamicConfiguration.writeBuffer offset
   */
  val ramHead       = RegInit(0.U(16.W))
  val buffBytesReg  = RegInit(0.U(16.W))
  val buffBytesCurr = Wire(UInt(16.W))

  // printf("bbreg: %d, %d\n", buffBytesReg, buffBytesCurr)

  buffBytesCurr := buffBytesReg + packet_reg_0.io.deq.bits.payloadBytes()

  // Always write the current payload data to RAM at the RAM head
  io.c2hPayloadRamReq.bits.addr := ramHead
  io.c2hPayloadRamReq.bits.len  := packet_reg_0.io.deq.bits.payloadBytes()

  /* Packet frames arrive LSB (bit 512) -> MSB (bit 0). So, when we
   * extract the payload it starts at bit 512. This is no issue if we
   * are wriing 64 bytes chunks, but if we are writing partial chunks
   * we need to shift that data so that its high bit (512 - size of
   * payload chunk) is byte zero of our write request
   */

  // TODO can this be made more generic? Shifting should happen somewhere else maybe?
  // The first frame is pure header data
  when (packet_reg_0.io.deq.bits.frame_off =/= 0.U) {
    // Always issue a RAM write request if possible
    io.c2hPayloadRamReq.valid     := packet_reg_0.io.deq.valid
  }

  when (packet_reg_0.io.deq.bits.frame_off === 64.U) {
    io.c2hPayloadRamReq.bits.data := packet_reg_0.io.deq.bits.payload >> (512.U - (packet_reg_0.io.deq.bits.payloadBytes() * 8.U))
  }.otherwise {
    io.c2hPayloadRamReq.bits.data := packet_reg_0.io.deq.bits.payload
  }

  // When the RAM request is eventually made, we can increment the RAM pointer offset 
  when(packet_reg_0.io.deq.fire) {
    // TODO how does this handle wrapping around the size of the RAM!?!?!?!
    ramHead := ramHead + packet_reg_0.io.deq.bits.payloadBytes()
    buffBytesReg := buffBytesReg + packet_reg_0.io.deq.bits.payloadBytes()
  }

  val buffAddr = Wire(UInt(16.W))
  buffAddr := ramHead - buffBytesReg

  io.c2hPayloadDmaReq.bits.pcie_addr := (packet_reg_0.io.deq.bits.data.data.offset + packet_reg_0.io.deq.bits.payloadOffset() - buffBytesReg).asTypeOf(UInt(64.W))
  io.c2hPayloadDmaReq.bits.ram_sel   := 0.U
  io.c2hPayloadDmaReq.bits.ram_addr  := buffAddr
  io.c2hPayloadDmaReq.bits.len       := buffBytesCurr
  io.c2hPayloadDmaReq.bits.tag       := 0.U
  io.c2hPayloadDmaReq.bits.port      := packet_reg_0.io.deq.bits.common.dport // TODO unsafe

  // Will this frame cause us to reach our write buffer limit?
  when (buffBytesCurr >= io.dynamicConfiguration.writeBufferSize || packet_reg_0.io.deq.bits.lastFrame() === 1.U) {
    io.c2hPayloadDmaReq.valid := packet_reg_0.io.deq.valid

    when(packet_reg_0.io.deq.fire) {
      buffBytesReg := 0.U
    }
  }

  packet_reg_0.io.deq.ready := true.B
  packet_reg_0.io.deq.bits  := DontCare
  packet_reg_0.io.deq.valid := DontCare
}

// class pp_ingress_bitmap extends Module {
//   val io = IO(new Bundle {
//     val packet_in  = Flipped(Decoupled(new PacketFrameFactory)) // Input packet factory 
//     // val packet_out = Decoupled(new PacketFrameFactory) // 
//   })
// 
//   /* Computation occurs between register stage 0 and 1
//    */
//   val packet_reg_0 = Module(new Queue(new PacketFrameFactory, 1, true, false))
//   val pending = Module(new Queue(new PacketFrameFactory, 6, true, false))
//   // val packet_reg_1 = Module(new Queue(new PacketFrameFactory, 1, true, false))
// 
//   // TODO create bundle of count offset + 64 bits of map
//   // TODO how do we know if the first packet in sequence? Can init the RAM?
//   // Would be better if we just had a unique local identifier?
//   // This can be provided in an initial request for a subsequent response
//   // But this only works for the requestor. 
// 
//   packet_reg_0.io.enq <> io.packet_in
// 
//   // Tie register stages to input and output 
//   // packet_reg_0.io.enq <> packet_reg_1.io.deq
// }
