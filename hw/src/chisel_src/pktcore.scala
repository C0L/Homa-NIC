package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util.Decoupled
import chisel3.util._

// TODO turn flow mode off



class pp_egress_stages extends Module {
  val io = IO(new Bundle {
    val trigger = Flipped(Decoupled(new queue_entry_t)) // Input from sendmsg priority queue
    val egress  = new axis(512, false, 0, false, 0, true, 64, true) // Output to ethernet

    val cb_ram_read_desc         = Decoupled(new ram_read_desc_t)
    val cb_ram_read_desc_status  = Flipped(Decoupled(new ram_read_desc_status_t))
    val cb_ram_read_data         = Flipped(Decoupled(new ram_read_data_t))

    val payload_ram_read_desc        = Decoupled(new ram_read_desc_t)
    val payload_ram_read_desc_status = Flipped(Decoupled(new ram_read_desc_status_t))
    val payload_ram_read_data        = Flipped(Decoupled(new ram_read_data_t))
  })

  val pp_lookup  = Module(new pp_egress_lookup)
  val pp_payload = Module(new pp_egress_payload)
  val pp_ctor    = Module(new pp_egress_ctor)
  val pp_xmit    = Module(new pp_egress_xmit)

  pp_lookup.io.packet_in  <> io.trigger
  pp_lookup.io.packet_out <> pp_payload.io.packet_in
  pp_payload.io.packet_out <> pp_ctor.io.packet_in
  pp_ctor.io.packet_out    <> pp_xmit.io.packet_in
  pp_xmit.io.egress <> io.egress
}

class pp_egress_lookup extends Module {
  val io = IO(new Bundle {
    val ram_read_desc         = Decoupled(new ram_read_desc_t)
    val ram_read_desc_status  = Flipped(Decoupled(new ram_read_desc_status_t))
    val ram_read_data         = Flipped(Decoupled(new ram_read_data_t))

    val packet_in  = Flipped(Decoupled(new queue_entry_t))
    val packet_out = Decoupled(new PacketFactory)
  })

  val frame = RegInit(0.U(32.W))

  io.ram_read_desc_status := DontCare

  val pending = Module(new Queue(new PacketFactory, 6, true, true))
  // Accept a new packet factory if the input is valid and we have room
  pending.io.enq.valid        := io.packet_in.valid
  io.packet_in.ready          := pending.io.enq.ready

  // Convert the enqueue data to a PacketFactory while storing queue output
  pending.io.enq.bits         := 0.U.asTypeOf(new PacketFactory)
  pending.io.enq.bits.trigger := io.packet_in.bits

  // Construct a read descriptor for this piece of data
  io.ram_read_desc.valid         := io.packet_in.valid
  io.ram_read_desc.bits.ram_addr := 64.U * io.packet_in.bits.rpc_id
  io.ram_read_desc.bits.len      := 64.U
  io.ram_read_desc.bits.tag      := 0.U // Everything is ordered
  io.ram_read_desc.bits.id       := 0.U
  io.ram_read_desc.bits.dest     := 0.U
  io.ram_read_desc.bits.user     := 0.U

  // Dequeue if the downstream can accept and we have ram data
  // We are ready if there are no more chunks to dequeue
  pending.io.deq.ready := io.packet_out.ready && io.ram_read_data.valid && (frame >= (1386 - 64).U)

  // If the RAM read has data there is always data in queue. That will
  // be emptied when the downstream is availible
  io.ram_read_data.ready := io.packet_out.ready

  // Write to packet output if there is ram read data and elements in the queue
  io.packet_out.valid  := pending.io.deq.valid && io.ram_read_data.valid

  /* When a new frame is accepted then reset frame offset When a new
   * frame is removed then increment the frame offset
   */
  when (pending.io.enq.fire) {
    frame := 0.U
  }.elsewhen (io.packet_out.fire) {
    frame := frame + 64.U
  }

  io.packet_out.bits    := pending.io.deq.bits
  io.packet_out.bits.frame := frame
  io.packet_out.bits.cb := io.ram_read_data.bits.asTypeOf(new msghdr_t)
}

class pp_egress_payload extends Module {
  val io = IO(new Bundle {
    val ram_read_desc        = Decoupled(new ram_read_desc_t)
    val ram_read_desc_status = Flipped(Decoupled(new ram_read_desc_status_t))
    val ram_read_data        = Flipped(Decoupled(new ram_read_data_t))

    val packet_in  = Flipped(Decoupled(new PacketFactory))
    val packet_out = Decoupled(new PacketFactory)
  })

  io.ram_read_desc_status := DontCare

  val pending = Module(new Queue(new PacketFactory, 6, true, true))
  // Accept a new packet factory if the input is valid and we have room
  pending.io.enq.valid        := io.packet_in.valid
  io.packet_in.ready          := pending.io.enq.ready

  // Construct a read descriptor for this piece of data
  io.ram_read_desc.valid         := io.packet_in.valid
  // TODO should clean this up
  io.ram_read_desc.bits.ram_addr := (io.packet_in.bits.trigger.dbuff_id * 16384.U) + (io.packet_in.bits.cb.buff_size - io.packet_in.bits.trigger.remaining) + io.packet_in.bits.frame 
  io.ram_read_desc.bits.len      := 64.U
  io.ram_read_desc.bits.tag      := 0.U // Everything is ordered
  io.ram_read_desc.bits.id       := 0.U
  io.ram_read_desc.bits.dest     := 0.U
  io.ram_read_desc.bits.user     := 0.U

  // TODO this logic is not very clean

  // Dequeue if the downstream can accept and we have ram data
  pending.io.deq.ready   := io.packet_out.ready && io.ram_read_data.valid

  // If the RAM read has data there is always data in queue. That will
  // be emptied when the downstream is availible
  io.ram_read_data.ready := io.packet_out.ready

  // Write to packet output if there is ram read data and elements in the queue
  io.packet_out.valid  := pending.io.deq.valid && io.ram_read_data.valid

  io.packet_out.bits         := pending.io.deq.bits
  io.packet_out.bits.payload := io.ram_read_data.bits.data
}

class pp_egress_ctor extends Module {
  val io = IO(new Bundle {
     val packet_in  = Flipped(Decoupled(new PacketFactory))
     val packet_out = Decoupled(new PacketFactory)
  })

  val packet_reg = Module(new Queue(new PacketFactory, 1, true, true))
  packet_reg.io.enq <> io.packet_in
  packet_reg.io.deq <> io.packet_out

  io.packet_out.bits.eth.mac_dest  := MAC.dst
  io.packet_out.bits.eth.mac_src   := MAC.src
  io.packet_out.bits.eth.ethertype := IPV6.ethertype

  io.packet_out.bits.ipv6.version     := IPV6.version
  io.packet_out.bits.ipv6.traffic     := IPV6.traffic
  io.packet_out.bits.ipv6.flow        := IPV6.flow
  io.packet_out.bits.ipv6.payload_len := 0.U // TODO 
  io.packet_out.bits.ipv6.next_header := IPV6.ipproto_homa
  io.packet_out.bits.ipv6.hop_limit   := IPV6.hop_limit
  io.packet_out.bits.ipv6.saddr       := packet_reg.io.deq.bits.cb.saddr
  io.packet_out.bits.ipv6.daddr       := packet_reg.io.deq.bits.cb.daddr

  io.packet_out.bits.common.sport     := packet_reg.io.deq.bits.cb.saddr
  io.packet_out.bits.common.dport     := packet_reg.io.deq.bits.cb.daddr
  io.packet_out.bits.common.doff      := HOMA.doff
  io.packet_out.bits.common.homatype  := HOMA.DATA
  io.packet_out.bits.common.checksum  := 0.U // TODO 
  io.packet_out.bits.common.sender_id := packet_reg.io.deq.bits.cb.id

  io.packet_out.bits.data.data.offset      := packet_reg.io.deq.bits.cb.buff_size - packet_reg.io.deq.bits.trigger.remaining
  io.packet_out.bits.data.data.seg_len     := 1386.U // packet_reg.io.deq.bits.trigger TODO 1386 or remaining
  io.packet_out.bits.data.data.ack.id      := 0.U
  io.packet_out.bits.data.data.ack.sport   := 0.U
  io.packet_out.bits.data.data.ack.dport   := 0.U
}
 
class pp_egress_xmit extends Module {
  val io = IO(new Bundle {
    val packet_in  = Flipped(Decoupled(new PacketFactory))
    val egress  = new axis(512, false, 0, false, 0, true, 64, true) // Output to ethernet
  })

  val packet_reg = Module(new Queue(new PacketFactory, 1, true, true))
  packet_reg.io.enq <> io.packet_in
  packet_reg.io.deq.ready := io.egress.tready
  io.egress.tvalid := packet_reg.io.deq.valid

  when (packet_reg.io.deq.bits.frame === 0.U) {
    // First 64 bytes
    io.egress.tdata := packet_reg.io.deq.bits.asUInt(512,0) // TODO this is from the wrong dir?
  }.elsewhen (packet_reg.io.deq.bits.frame === 64.U) {
    // Second 64 bytes
    io.egress.tdata := packet_reg.io.deq.bits.asUInt(1023,512) // TODO this is from the wrong dir?
  }.otherwise {
    io.egress.tdata := packet_reg.io.deq.bits.payload
  }

  // TODO temporary full length packets only
  when (packet_reg.io.deq.bits.frame >= 1386.U) {
    io.egress.tlast.get := 1.U
  }

  // when (packet_reg.io.deq.bits.frame >= /* payload size */) {
  io.egress.tkeep.get := "hffffffffffffffff".U
  // }
}

class pp_ingress_stages extends Module {
  val io = IO(new Bundle {
    val igress       = Flipped(new axis(512, false, 0, false, 0, true, 64, true)) // Input from ethernet
    val dma_w_data_i = Decoupled(new dma_write_t)

    // TODO some final output channel
    // val packet_out = Decoupled(new PacketFactory)

    // val trigger = Flipped(Decoupled(new queue_entry_t)) // Input from sendmsg priority queue
    // val ingress = new axis(512, false, 0, false, 0, true, 64, true) // Output to ethernet 
  })

  val pp_dtor    = new pp_ingress_dtor
  val pp_map     = new pp_ingress_map
  val pp_lookup  = new pp_ingress_lookup
  val pp_payload = new pp_ingress_payload

  pp_dtor.io.ingress  <> io.ingress
  pp_map.io.packet_in <> pp_dtor.io.packet_out

  // pp_lookup.io.packet_out <> pp_payload.io.packet_in
  // pp_payload.io.packet_out <> pp_ctor.io.packet_in
  // pp_ctor.io.packet_out    <> pp_xmit.io.packet_in
  // pp_xmit.io.egress <> io.egress
}


class pp_ingress_dtor extends Module {
  val io = IO(new Bundle {
    val igress     = Flipped(new axis(512, false, 0, false, 0, true, 64, true)) // Input from ethernet
    val packet_out = Decoupled(new PacketFactory)
  })

  val processed = RegInit(0.U(32.W))
  val pktfac    = Reg(new PacketFactory)

  // TODO this is probably not good
  io.igress.tready := io.packet_out.ready

  // State machine for parsing packets
  when (io.igress.tready) {
    when (processed === 0.U) {
      // Process data into packet fac
      pktfac.asUInt(511,0) := io.igress.tdata
      pktfac.frame         := processed
    }.elsewhen (processed === 64.U) {
      // We know that packet_out was ready so this will send data
      pktfac.asUInt(1023,512) := io.igress.tdata
      pktfac.frame            := processed
      io.packet_out.valid     := true.B
    }.otherwise {
      // Fill the payload section
      io.packet_out.valid := true.B
      pktfac.payload := io.igress.tdata
      pktfac.frame   := processed
    }

    when (io.igress.tlast.get) {
      processed := 0.U
    }
  }
}

/* pp_ingress_map - Take an input packet and either find its mapping
 * to a local ID or create a mapping
 */
class pp_ingress_map extends Module {
  val io = IO(new Bundle {
    val packet_in  = Flipped(Decoupled(new PacketFactory))
    val packet_out = Decoupled(new PacketFactory)
  })

  // TODO 
  io.packet_in <> io.packet_out 
}

class pp_ingress_lookup extends Module {
  val io = IO(new Bundle {
    val ram_read_desc        = Decoupled(new ram_read_desc_t)
    val ram_read_desc_status = Flipped(Decoupled(new ram_read_desc_status_t))
    val ram_read_data        = Flipped(Decoupled(new ram_read_data_t))

    val packet_in  = Flipped(Decoupled(new PacketFactory))
    val packet_out = Decoupled(new PacketFactory)
  })

  io.ram_read_desc_status := DontCare

  val pending = Module(new Queue(new PacketFactory, 6, true, true))

  pending.io.enq <> io.packet_in

  // Construct a read descriptor for this piece of data
  io.ram_read_desc.valid         := io.packet_in.valid
  // TODO should clean this up
  io.ram_read_desc.bits.ram_addr := 64.U * io.packet_in.bits.cb.id
  io.ram_read_desc.bits.len      := 64.U
  io.ram_read_desc.bits.tag      := 0.U // Everything is ordered
  io.ram_read_desc.bits.id       := 0.U
  io.ram_read_desc.bits.dest     := 0.U
  io.ram_read_desc.bits.user     := 0.U

  // TODO this logic is not very clean

  // Dequeue if the downstream can accept and we have ram data
  pending.io.deq.ready   := io.packet_out.ready && io.ram_read_data.valid

  // If the RAM read has data there is always data in queue. That will
  // be emptied when the downstream is availible
  io.ram_read_data.ready := io.packet_out.ready

  // Write to packet output if there is ram read data and elements in the queue
  io.packet_out.valid  := pending.io.deq.valid && io.ram_read_data.valid

  io.packet_out.bits    := pending.io.deq.bits
  io.packet_out.bits.cb := io.ram_read_data.bits
}

class pp_ingress_payload extends Module {
  val io = IO(new Bundle {
    val dma_w_data_i = Decoupled(new dma_write_t) 

    val packet_in  = Flipped(Decoupled(new PacketFactory))
    val packet_out = Decoupled(new PacketFactory)
  })
  val pending = Module(new Queue(new PacketFactory, 6, true, true))

  pending.io.enq <> io.packet_in

  pending.io.deq <> io.packet_out
}
