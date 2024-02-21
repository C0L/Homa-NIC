package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._
import scala.collection.immutable.ListMap
import scala.collection.immutable.SeqMap
import scala.collection.mutable.Map

import java.nio.file.{Paths, Files}
import java.nio.charset.StandardCharsets

/* AXISClockConverterRaw - This blackbox module maps down to the
 * xilinx axis_clock_converter IP. The signal names of this module
 * need to map to the signals of the axis_clock_converter IP. This
 * generates a corresponding tcl file for instantiating the xilinx IP.
 */ 
class AXISClockConverterRaw(
  DATA_WIDTH: Int,
  ENABLE_ID: Boolean,
  ID_WIDTH: Int,
  ENABLE_USER: Boolean,
  USER_WIDTH: Int,
  ENABLE_LAST: Boolean) extends BlackBox {
  val io = IO(new Bundle {
    val s_axis         = Flipped(new axis(DATA_WIDTH, ENABLE_ID, ID_WIDTH, ENABLE_USER, USER_WIDTH, false, 0, ENABLE_LAST))
    val s_axis_aresetn = Input(Reset())
    val s_axis_aclk    = Input(Clock())
    val m_axis         = new axis(DATA_WIDTH, ENABLE_ID, ID_WIDTH, ENABLE_USER, USER_WIDTH, false, 0, ENABLE_LAST)
    val m_axis_aresetn = Input(Reset())
    val m_axis_aclk    = Input(Clock())
  })

  // Create a unique name for the axis clock converter
  override def desiredName = "AXISClockConverter" + this.hashCode()

  val ipname   = this.desiredName
  val ipgen    = new StringBuilder()

  // Roundoff to the nearest number of bytes
  val numbytes = ((io.s_axis.getWidth.toFloat / 8 ).ceil).toInt

  // Generate tcl to 1) construct the ip with the given name, 2) parameterize the module 
  ipgen ++= s"create_ip -name axis_clock_converter -vendor xilinx.com -library ip -module_name ${ipname}\n"
  ipgen ++= s"set_property CONFIG.TDATA_NUM_BYTES ${numbytes} [get_ips ${ipname}]"

  // Dump the generated tcl for build flow
  Files.write(Paths.get(s"gen/${ipname}.tcl"), ipgen.toString.getBytes(StandardCharsets.UTF_8))
}

/* AXISClockConverter - Wrapper for the AXISClockConverterRaw that
 * converts from a chisel type to a generic AXI stream type for
 * compatibility with the axis_clock_converter Xilinx IP.
 */
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

/* AXIClockConverter - blackbox module for the axi_clock_converter IP.
 * The signals of this module need to match that of the Xilinx IP. A
 * corresponding tcl file is generated which is used in the build
 * process for IP generation.
 */

// TODO should just type as input
class AXIClockConverter(
  AXI_DATA_WIDTH: Int,
  AXI_ADDR_WIDTH: Int,
  ENABLE_ID: Boolean,
  AXI_ID_WIDTH: Int,
  ENABLE_REGION: Boolean,
  AXI_REGION_WIDTH: Int,
  ENABLE_QOS: Boolean,
  AXI_QOS_WIDTH: Int
) extends BlackBox {
  val io = IO(new Bundle {
    val s_axi         = Flipped(new axi(512, 26, true, 8, true, 4, true, 4))
    val s_axi_aresetn = Input(Reset())
    val s_axi_aclk    = Input(Clock())
    val m_axi         = new axi(512, 26, true, 8, true, 4, true, 4)
    val m_axi_aresetn = Input(Reset())
    val m_axi_aclk    = Input(Clock())
  })

  // Construct a unique identifier for this module (which we will map to from tcl)
  override def desiredName = "AXIClockConverter" + this.hashCode()

  val ipname   = this.desiredName

  val ipgen    = new StringBuilder()
  val data_width = AXI_DATA_WIDTH
  val addr_width = AXI_ADDR_WIDTH
  val id_width   = AXI_ID_WIDTH

  // Generate tcl to 1) construct the ip with the given name, 2) parameterize the module 
  ipgen ++= s"create_ip -name axi_clock_converter -vendor xilinx.com -library ip -module_name ${ipname}\n"
  ipgen ++= s"set_property CONFIG.DATA_WIDTH {${data_width}} [get_ips ${ipname}]\n"
  ipgen ++= s"set_property CONFIG.ADDR_WIDTH {${addr_width}} [get_ips ${ipname}]\n"
  ipgen ++= s"set_property CONFIG.ID_WIDTH {${id_width}} [get_ips ${ipname}]\n"

  // Dump the generated tcl for build flow
  Files.write(Paths.get(s"gen/${ipname}.tcl"), ipgen.toString.getBytes(StandardCharsets.UTF_8))
}

/* ClockWizard - This blackbox module maps down to the clk_wiz xilinx
 * IP. The port names of this module needs to map to the xilinx IP.
 */
class ClockWizard extends BlackBox {
  val io = IO(new Bundle {
    val clk_in1_n = Input(Clock())
    val clk_in1_p = Input(Clock())
    val reset     = Input(Reset())

    val clk_out1  = Output(Clock())
    val locked    = Output(UInt(1.W))
  })
}

/* ProcessorSystemReset - This blackbox module maps down to the
 * processor_system_reset xilinx IP. The port names of this module
 * needs to map to the xilinx IP signals
 */ 
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

/* ILARaw - This blackbox module maps to the xilinx ILA primitive IP.
 * The signal names of this module need to match that of the signals
 * in the xilinx ip. To accomplish that we take a generic bundle gen
 * and convert it to bitvectors named param0-X where X is the number
 * of signals in the bundle.
 */ 
class ILARaw[T <: Bundle](gen: T) extends BlackBox {
  // Construct the IO ports manually by interating through the elements in the input bundle
  val io = IO(new Record {
    val elements = {
      val list = gen.elements.zip(0 until gen.elements.size).map(elem => {
        val index = elem._2
        // Incremenent index and associate with bitvector to bundle entry
        (s"probe$index", Input(UInt(elem._1._2.getWidth.W)))
      })

      // Add the clk port in final port map
      (list.toMap[String, chisel3.Data]).to(SeqMap) ++ ListMap("clk" -> Input(Clock()))
    }
  })

  // Construct a unique identifier for this module (which we will map to from tcl)
  override def desiredName = "ILA_" + this.hashCode()

  val ipname   = this.desiredName
  val numprobs = gen.getElements.length

  val ipgen = new StringBuilder()

  // Generate tcl to 1) construct the ip with the given name, 2) parameterize the module 
  ipgen ++= s"create_ip -name ila -vendor xilinx.com -library ip -module_name ${ipname}\n"
  ipgen ++= s"set_property CONFIG.C_NUM_OF_PROBES {${numprobs}} [get_ips ${ipname}]\n"

  gen.elements.zip(0 until gen.elements.size).foreach(elem => {
    val index = elem._2
    val width = gen.getElements(index).getWidth
    ipgen ++= s"set_property CONFIG.C_PROBE${index}_WIDTH {${width}} [get_ips ${ipname}]\n"
  })

  // Dump the generated tcl for build flow
  Files.write(Paths.get(s"gen/${ipname}.tcl"), ipgen.toString.getBytes(StandardCharsets.UTF_8))
}

/* ILA - wrapper for raw ila module. Inverters the reset to create
 * more natural interface. Maps the bundle entries to the params of
 * the raw ila.
 */ 
class ILA[T <: Bundle](gen: T) extends Module {
  val io = IO(new Bundle {
    val ila_data = Input(gen)
  })

  // Construct the raw clock generator and map implicit clock to raw clk
  val ila = Module(new ILARaw(gen))
  ila.io.elements("clk") := clock

  // Map all of the bundle elements to the params of the raw clock generator
  io.ila_data.elements.zip(0 until gen.elements.size).foreach(elem => {
    val index = elem._2
    ila.io.elements(s"probe$index") := io.ila_data.getElements(index).asUInt
  })
}

// class CMAC extends RawModule {
// 
// }
