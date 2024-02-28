package gpnic

import chisel3._
import chisel3.util._

class DynamicConfiguration extends Bundle {
  val fetchRequstSize = UInt(16.W) // TODO this was cc
  val writeBufferSize = UInt(16.W) // 
  val unused          = UInt((512-32).W)
}

/* CACHE - cache system constant configuration
 */
object CACHE {
  val line_size = 16384.U
  // val lines = number of dbuffs
}


// TODO configure clock frequencies here??
