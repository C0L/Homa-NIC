package gpnic

import chisel3._
import chisel3.util._

/* Scheduler - Responsible for deciding the next packet of data
 * to send out. Packet eligibility is determined by grant status and
 * data availability status.
 */ 
class Scheduler extends Module {
  val io = IO(new Bundle {
    // TODO update types
    val newMessage = Flipped(Decoupled(new SchedulerState)) // Messages needing to be managed by scheduler
    // val newGranted = Flipped(Decoupled(new QueueEntry)) // Arrived grant notifications
    val newCached  = Flipped(Decoupled(new SchedulerState)) // Arrived cached data notifications

    val sendPacket = Decoupled(new SchedulerEntry) // The next packet to be sent out onto the network
  })

  // Parallel compare priority queue for packet send entries
  val packetQueue = Module(new PriorityQueue(new SchedulerEntry, 32))

  val msgState = SyncReadMem(SysCfg.maxRPCs, new SchedulerState)

  val excluded = Vec(8, Reg(UInt(16.W)))

  val msgStateRd = Wire(new SchedulerState)
  val msgStateWr = Wire(new SchedulerState)

  val newMessageData  = Reg(Reg(io.newMessage.bits))
  val newMessageValid = Reg(RegInit(false.B))

  val newCachedData  = Reg(Reg(io.newCached.bits))
  val newCachedValid = Reg(RegInit(false.B))

  val blocked = Module(new Queue(new SchedulerEntry, 16, true, false))
  val blockedMessageValid = Reg(RegInit(false.B))

  // TODO should have insertion queues for new message and updates 

  io.sendPacket.bits  := packetQueue.io.dequeue.bits
  io.sendPacket.valid := packetQueue.io.dequeue.valid
  packetQueue.io.dequeue.ready := io.sendPacket.ready

  // TODO does this reach 0?
  // TODO this could potentially drop elements not being inserted into the blocked queue
  // We dequeued an entry becaues it ran out of grants or data but it is not complete
  when (packetQueue.io.dequeue.fire &&
    packetQueue.io.dequeue.bits.cycles === 0.U &&
    packetQueue.io.dequeue.bits.priority =/= 0.U) {
    blocked.io.enq.bits  := packetQueue.io.dequeue.bits
    blocked.io.enq.valid := true.B
  }

  /* Read Operations */
  when (blocked.io.deq.valid && !excluded.contains(io.newCached.bits.localID)) {
    blocked.io.deq.ready := true.B

    val exindex = excluded.indexWhere(e => e === 0.U)
    excluded(exindex) := io.newCached.bits.localID

    msgStateRd := msgState.read(blocked.io.deq.bits.localID)
    blockedMessageValid := true.B
  }.elsewhen (io.newMessage.valid) {
    io.newMessage.ready := true.B
    // Issue read request for this update
    msgStateRd := msgState.read(io.newMessage.bits.localID)
    newMessageData  := io.newMessage.bits
    newMessageValid := true.B
  }.elsewhen (io.newCached.valid && !excluded.contains(io.newCached.bits.localID)) {
    io.newCached.ready := true.B
    // Lock state updates for this RPC ID
    val exindex = excluded.indexWhere(e => e === 0.U)
    excluded(exindex) := io.newCached.bits.localID

    // Issue read request for this update
    msgStateRd := msgState.read(io.newCached.bits.localID)
    newCachedData  := io.newCached.bits
    newCachedValid := true.B
  }

  /* Modify Operations */
  when (blockedMessageValid) {
    msgStateWr := msgStateRd
    // Cached byte count is one packet or more greater than sent bytes
    msgStateWr.queued := ((msgStateRd.totalLen - msgStateRd.totalRem)
      + NetworkCfg.payloadBytes.U <= msgStateRd.cached)
  }.elsewhen (newMessageValid) {
    msgStateWr := newMessageData
    msgStateWr.queued := true.B
  }.elsewhen (newCachedValid) {
    // The state is largly unchanged, except one packet of 
    msgStateWr        := msgStateRd
    msgStateWr.cached := msgStateRd.cached - NetworkCfg.payloadBytes.U
  }

  /* Write Operations */
  when (blockedMessageValid) {
    msgState.write(msgStateWr.localID, msgStateWr)
    val exindex = excluded.indexWhere(e => e === msgStateWr.localID)
    excluded(exindex) := 0.U

    when (msgStateWr.queued) {
      packetQueue.io.enqueue.bits := msgStateWr.entry(NetworkCfg.payloadBytes.U)
    }
  }.elsewhen (newMessageValid) {

    msgState.write(msgStateWr.localID, msgStateWr)
    packetQueue.io.enqueue.bits := newMessageData.entry(NetworkCfg.payloadBytes.U)
    // Assumes that the priority queue is always willing to accept
    packetQueue.io.enqueue.valid := true.B
  }.elsewhen (newCachedValid) {
    msgState.write(msgStateWr.localID, msgStateWr)
    val exindex = excluded.indexWhere(e => e === msgStateWr.localID)
    excluded(exindex) := 0.U

    when (!msgStateWr.queued) {
      packetQueue.io.enqueue.bits := msgStateWr.entry(NetworkCfg.payloadBytes.U)
    }
  }
}
