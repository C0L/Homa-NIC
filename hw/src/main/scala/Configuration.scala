package gpnic

import chisel3._
import chisel3.util._

object SysCfg {
  val maxRPCs = 16384 // Maximum number of assignable RPC IDs
}

// Network imposed constraints
object NetworkCfg {
  val maxEthFrameBytes = 1522 // Ethernet frame maximum 1522 bytes (non-jumbo)
  // TODO this should be more generic "encapsulation"
  // TODO also some way to account for different packet variants in a more generic way
  val headerBytes = { // Total number of frame bytes dedicated to header (TODO currently this is DATA packet specific)
    ((new EthHeader).getWidth + (new Ipv6Header).getWidth
      + (new HomaCommonHeader).getWidth + (new HomaDataHeader).getWidth) / 8
  }
  val payloadBytes = { // Number of bytes in frame that can be dedicated to payload (TODO currently DATA packet specific)
    (maxEthFrameBytes - headerBytes)
  }
}

// TODO what goes through the pipeline?

class DynamicConfiguration extends Bundle {
  val logReadSize  = UInt(8.W) // Log2 of the size of DMA read requests  was 16
  val logWriteSize = UInt(8.W) // Log2 of the size of DMA write requests was 16
  val unused       = UInt((512-16).W)
}

/* CACHE - cache system constant configuration
 */
object CacheCfg {
  val lineSize = 16384
  val logLineSize = 14 
  // val lines = number of dbuffs
}

// TODO some abstract class Encapsulation. Defines operations on the packet as a whole
// TODO some abstract class HomaPacket (DATA, GRANT). // Defines operations on Homa packet
// TODO some concrete classes for each Homa Packet

// TODO configure clock frequencies here??
