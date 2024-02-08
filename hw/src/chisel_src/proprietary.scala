// Xilinx IP wrapper

package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._
import chisel3.experimental.prefix
import chisel3.experimental.noPrefix

// TODO maybe specify the type to the clock conveter through a module on top that handles bit conversion

class AXISClockConverterRaw(
  DATA_WIDTH: Int,
  ENABLE_ID: Boolean,
  ID_WIDTH: Int,
  ENABLE_USER: Boolean,
  USER_WIDTH: Int,
  ENABLE_LAST: Boolean) extends BlackBox {
  val io = IO(new Bundle {
    val s_axis         = Flipped(new axis(DATA_WIDTH, ENABLE_ID, ID_WIDTH, ENABLE_USER, USER_WIDTH, ENABLE_LAST))
    val s_axis_aresetn = Input(Reset())
    val s_axis_aclk    = Input(Clock())
    val m_axis         = new axis(DATA_WIDTH, ENABLE_ID, ID_WIDTH, ENABLE_USER, USER_WIDTH, ENABLE_LAST)
    val m_axis_aresetn = Input(Reset())
    val m_axis_aclk    = Input(Clock())
  })

  // TODO why is this needed
  override def desiredName = s"AXISClockConverter_$DATA_WIDTH"
}

// Polymorphic clock converter
// RawModule means no clock and reset
class AXISClockConverter[T <: Data](gen: T) extends RawModule {
  val io = IO(new Bundle {
    val s_axis         = Flipped(Decoupled(gen))
    val s_axis_aresetn = Input(Reset())
    val s_axis_aclk    = Input(Clock())
    val m_axis         = Decoupled(gen)
    val m_axis_aresetn = Input(Reset())
    val m_axis_aclk    = Input(Clock())
  })

  val DATA_WIDTH = gen.getWidth

  // TODO all other signals disabled for now
  val axis_cc_raw = Module(new AXISClockConverterRaw(DATA_WIDTH, false, 0, false, 0, false))

  axis_cc_raw.io.s_axis_aresetn := io.s_axis_aresetn
  axis_cc_raw.io.s_axis_aclk    := io.s_axis_aclk
  axis_cc_raw.io.m_axis_aresetn := io.m_axis_aresetn
  axis_cc_raw.io.m_axis_aclk    := io.m_axis_aclk

  axis_cc_raw.io.s_axis.tdata   := io.s_axis.bits.asTypeOf(UInt(DATA_WIDTH.W))
  axis_cc_raw.io.s_axis.tvalid  := io.s_axis.valid
  io.s_axis.ready               := axis_cc_raw.io.s_axis.tready

  io.m_axis.bits                := axis_cc_raw.io.m_axis.tdata.asTypeOf(gen)
  io.m_axis.valid               := axis_cc_raw.io.m_axis.tvalid
  axis_cc_raw.io.m_axis.tready  := io.m_axis.ready
}

// TODO axi4 needs to be more parmeterizable. Or compute most of the values manually

// Pass the desired type (preconfigured into the class)
// TODO can this be handle polymorphically?

class AXIClockConverter(
  DATA_WIDTH: Int,
  ENABLE_ID: Boolean,
  ID_WIDTH: Int,
  ENABLE_USER: Boolean,
  USER_WIDTH: Int,
  ENABLE_LAST: Boolean) extends BlackBox {
  val io = IO(new Bundle {
    val s_axi         = Flipped(new axi(DATA_WIDTH, 0, 0))
    val s_axi_aresetn = Input(Reset())
    val s_axi_aclk    = Input(Clock())
    val m_axi         = new axi(DATA_WIDTH, 0, 0)
    val m_axi_aresetn = Input(Reset())
    val m_axi_aclk    = Input(Clock())
  })
}

class SystemILA[T <: Bundle](gen: T) extends BlackBox {
  val io = IO(new Bundle {
    val SLOT_0_AXIS = Flipped(new axis(gen.getWidth, false, 0, false, 0, false))
    val clk         = Input(Clock())
    val resetn      = Input(Reset())
  })
  // Input(MixedVec(gen.getElements)).suggestName("blah")
}

class ClockWizard extends BlackBox {
  val io = IO(new Bundle {
    val clk_in1_n = Input(Clock())
    val clk_in1_p = Input(Clock())
    val reset     = Input(Reset())

    val clk_out1  = Output(Clock())
    val locked    = Output(UInt(1.W))
  })
}

class ProcessorSystemReset extends BlackBox {
  val io = IO(new Bundle {
    val slowest_sync_clk   = Input(Clock())
    val ext_reset_in       = Input(Bool())
    val aux_reset_in       = Input(Bool())
    val dcm_locked         = Input(UInt(1.W))
    val peripheral_reset   = Output(Bool())
    // val peripheral_areset  = Output(Reset())
    // val peripheral_aresetn = Output(Reset())
  })
}
