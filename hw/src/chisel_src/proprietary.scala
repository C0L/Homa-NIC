// Xilinx IP wrapper

package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._
// import chisel3.experimental.prefix
// import chisel3.experimental.noPrefix
// import chisel3.util.experimental.{forceName, InlineInstance}
// import chisel3.internal.plugin.autoNameRecursively
// import chisel3.internal.plugin.prefix
import scala.collection.immutable.ListMap
import scala.collection.immutable.SeqMap
import scala.collection.mutable.Map

import java.nio.file.{Paths, Files}
import java.nio.charset.StandardCharsets



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

class ILARaw[T <: Bundle](gen: T) extends BlackBox {
  val io = IO(new Record {
    val elements = {
      val list = gen.elements.zip(0 until gen.elements.size).map(elem => {
        val idx = elem._2
        (s"probe$idx", Input(UInt(elem._1._2.getWidth.W)))
      })

      println(list)
      
      (list.toMap[String, chisel3.Data]).to(SeqMap) ++ ListMap("clk" -> Input(Clock()), "resetn" -> Input(Reset()))
    }
  })
}

class ILA[T <: Bundle](gen: T) extends Module {
  val io = IO(new Bundle {
    val ila_data = Input(gen)
  })

  val ila = Module(new ILARaw(gen))
  ila.io.elements("clk")    := clock
  ila.io.elements("resetn") := !reset.asBool

  io.ila_data.elements.zip(0 until gen.elements.size).foreach(elem => {
    val idx = elem._2
    ila.io.elements(s"probe$idx") := io.ila_data.getElements(idx).asUInt
  })

  // println(this.hashCode())

  override def desiredName = "ILA_" + this.hashCode()

  // val num_probs = io.ila_data. TODO size of vector

  val ipgen   = s"create_ip -name ila -vendor xilinx.com -library ip -module_name ILA_" + this.hashCode()
  // val numprob = s"set_property CONFIG.C_NUM_OF_PROBES {$}"
  // val parms = "set_property CONFIG.TDATA_NUM_BYTES {9} [get_ips AXISClockConverter_82]"

  Files.write(Paths.get("file.txt"), ipgen.getBytes(StandardCharsets.UTF_8))

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
    val slowest_sync_clk     = Input(Clock())
    val ext_reset_in         = Input(Bool())
    val aux_reset_in         = Input(Bool())
    val dcm_locked           = Input(UInt(1.W))
    val peripheral_reset     = Output(Bool())
    val mb_reset             = Output(UInt(1.W))
    val bus_struct_reset     = Output(UInt(1.W))
    val interconnect_aresetn = Output(UInt(1.W))
    val peripheral_aresetn   = Output(UInt(1.W))
    val mb_debug_sys_rst     = Input(UInt(1.W))
  })
}
