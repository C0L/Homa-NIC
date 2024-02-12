package packetproc

import chisel3._
import circt.stage.ChiselStage
// import chisel3.util.Decoupled
import chisel3.util._

class pp_stages extends Module {

  // TODO cobble together all the stages here
  // header constructor

  // data fetch

  // transmit
}



class pp_hdr extends Module {

}

class pp_payload extends Module {

  // TODO makes request to segmented memory
  // TOOD use tag cyclic buffer to delay till return. Maybe needs to be smaller for resources?

}

class pp_xmit extends Module {

}
