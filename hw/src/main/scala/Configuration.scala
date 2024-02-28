package gpnic

import chisel3._
import chisel3.util._



class DynamicConfiguration extends Bundle {
  val fetchSize = UInt(16.W)       // TODO this was cc
  val unused    = UInt((512-64).W)
}


/* CACHE - cache system constant configuration
 */
object CACHE {
  val line_size = 16384.U
  val lines = 
}


// TODO configure clock frequencies here??
