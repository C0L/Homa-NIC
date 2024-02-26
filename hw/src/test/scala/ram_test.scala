package gpnic 

import chisel3._
import chisel3.experimental.BundleLiterals._
import chisel3.simulator.EphemeralSimulator._
import org.scalatest.freespec.AnyFreeSpec
import org.scalatest.matchers.must.Matchers

import circt.stage.ChiselStage
import chisel3.util.Decoupled
import chisel3.util._

class simple_ram extends Module {
  val io = IO(new Bundle {
      val ram_read_desc         = Flipped(Decoupled(new ram_read_desc_t))
      val ram_read_data         = Decoupled(new ram_read_data_t)

      val ram_write_desc        = Flipped(Decoupled(new ram_write_desc_t))
      val ram_write_data        = Flipped(Decoupled(new ram_write_data_t))
      val ram_write_desc_status = Decoupled(new ram_write_desc_status_t)
  })

  val sendmsg_cb     = Module(new dma_psdpram)
  val sendmsg_cb_wr  = Module(new dma_client_axis_sink)
  val sendmsg_cb_rd  = Module(new psdpram_rd)

  sendmsg_cb.io.clk := clock
  sendmsg_cb.io.rst := reset.asUInt

  sendmsg_cb_wr.io.clk := clock
  sendmsg_cb_wr.io.rst := reset.asUInt

  sendmsg_cb.io.ram_wr <> sendmsg_cb_wr.io.ram_wr
  sendmsg_cb.io.ram_rd <> sendmsg_cb_rd.io.ram_rd

  sendmsg_cb_wr.io.enable := 1.U
  sendmsg_cb_wr.io.abort := 0.U

  io.ram_read_desc        <> sendmsg_cb_rd.io.ram_read_desc
  io.ram_read_data        <> sendmsg_cb_rd.io.ram_read_data

  io.ram_write_desc        <> sendmsg_cb_wr.io.ram_write_desc
  io.ram_write_desc_status <> sendmsg_cb_wr.io.ram_write_desc_status
  io.ram_write_data        <> sendmsg_cb_wr.io.ram_write_data
 }

class unaligned_read_test extends AnyFreeSpec {
  "unaligned read test" in {
    simulate(new simple_ram) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.ram_write_desc.valid.poke(true.B)
      dut.io.ram_write_desc.ready.expect(true.B)
      dut.io.ram_write_desc.bits.ram_addr.poke(0.U)
      dut.io.ram_write_desc.bits.len.poke(64.U)

      dut.io.ram_write_desc_status.ready.poke(true.B)

      dut.clock.step()

      dut.io.ram_write_desc.valid.poke(false.B)

      dut.io.ram_write_data.ready.expect(true.B)
      dut.io.ram_write_data.valid.poke(true.B)
      dut.io.ram_write_data.bits.data.poke("h11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111".U(512.W))
      dut.io.ram_write_data.bits.keep.poke("hffffffffffffffff".U)
      dut.io.ram_write_data.bits.last.poke(true.B)

      dut.clock.step()

      println("1")
      dut.io.ram_write_desc.valid.poke(true.B)
      dut.io.ram_write_desc.ready.expect(true.B)
      dut.io.ram_write_desc.bits.ram_addr.poke(64.U)
      dut.io.ram_write_desc.bits.len.poke(64.U)

      dut.clock.step()

      dut.io.ram_write_desc.valid.poke(false.B)

      dut.io.ram_write_data.ready.expect(true.B)
      dut.io.ram_write_data.valid.poke(true.B)
      dut.io.ram_write_data.bits.data.poke("h22222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222".U(512.W))
      dut.io.ram_write_data.bits.keep.poke("hffffffffffffffff".U)
      dut.io.ram_write_data.bits.last.poke(true.B)

      dut.clock.step()

      dut.io.ram_write_desc.valid.poke(true.B)
      dut.io.ram_write_desc.ready.expect(true.B)
      dut.io.ram_write_desc.bits.ram_addr.poke(128.U)
      dut.io.ram_write_desc.bits.len.poke(64.U)

      dut.clock.step()

      dut.io.ram_write_data.ready.expect(true.B)
      dut.io.ram_write_data.valid.poke(true.B)
      dut.io.ram_write_data.bits.data.poke("h33333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333".U(512.W))
      dut.io.ram_write_data.bits.keep.poke("hffffffffffffffff".U)
      dut.io.ram_write_data.bits.last.poke(true.B)

      dut.clock.step()

      dut.io.ram_write_desc.valid.poke(true.B)
      dut.io.ram_write_desc.ready.expect(true.B)
      dut.io.ram_write_desc.bits.ram_addr.poke(192.U)
      dut.io.ram_write_desc.bits.len.poke(64.U)

      dut.clock.step()

      dut.io.ram_write_data.ready.expect(true.B)
      dut.io.ram_write_data.valid.poke(true.B)
      dut.io.ram_write_data.bits.data.poke("h44444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444".U(512.W))
      dut.io.ram_write_data.bits.keep.poke("hffffffffffffffff".U)
      dut.io.ram_write_data.bits.last.poke(true.B)

      dut.clock.step()

      dut.io.ram_write_desc.valid.poke(false.B)
      dut.io.ram_write_data.valid.poke(false.B)

      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()

      dut.io.ram_read_desc.ready.expect(true.B)
      dut.io.ram_read_desc.valid.poke(true.B)
      dut.io.ram_read_desc.bits.ram_addr.poke(32.U(18.W))
      dut.io.ram_read_desc.bits.len.poke(64.U)

      dut.clock.step()

      dut.io.ram_read_desc.ready.expect(true.B)
      dut.io.ram_read_desc.valid.poke(true.B)
      dut.io.ram_read_desc.bits.ram_addr.poke((128+32).U)
      dut.io.ram_read_desc.bits.len.poke(64.U)

      dut.clock.step()

      dut.io.ram_read_desc.ready.expect(true.B)
      dut.io.ram_read_desc.valid.poke(true.B)
      dut.io.ram_read_desc.bits.ram_addr.poke((64+32).U)
      dut.io.ram_read_desc.bits.len.poke(64.U)

      dut.clock.step()

      dut.io.ram_read_desc.valid.poke(false.B)
      // dut.io.ram_read_desc.valid.poke(true.B)
      // dut.io.ram_read_desc.bits.ram_addr.poke(32.U)
      // dut.io.ram_read_desc.bits.len.poke(64.U)


      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      // dut.clock.step()
      // dut.clock.step()

      println("valid data out")
      dut.io.ram_read_data.valid.expect(true.B)
      dut.io.ram_read_data.ready.poke(true.B)
      dut.io.ram_read_data.bits.data.expect("h22222222222222222222222222222222222222222222222222222222222222221111111111111111111111111111111111111111111111111111111111111111".U)

      dut.clock.step()

      println("valid data out")
      dut.io.ram_read_data.valid.expect(true.B)
      dut.io.ram_read_data.ready.poke(true.B)
      dut.io.ram_read_data.bits.data.expect("h44444444444444444444444444444444444444444444444444444444444444443333333333333333333333333333333333333333333333333333333333333333".U)

      dut.clock.step()

      println("valid data out")
      dut.io.ram_read_data.valid.expect(true.B)
      dut.io.ram_read_data.ready.poke(true.B)
      dut.io.ram_read_data.bits.data.expect("h33333333333333333333333333333333333333333333333333333333333333332222222222222222222222222222222222222222222222222222222222222222".U)
    }
  }
}



class simple_wr extends Module {
  val io = IO(new Bundle {
      val ram_read_desc         = Flipped(Decoupled(new ram_read_desc_t))
      val ram_read_data         = Decoupled(new ram_read_data_t)

      val ram_write_cmd = Flipped(Decoupled(new RamWrite))
  })

  val ram = Module(new dma_psdpram)
  val wr  = Module(new psdpram_wr)
  val rd  = Module(new psdpram_rd)

  ram.io.clk := clock
  ram.io.rst := reset.asUInt

  ram.io.ram_wr <> wr.io.ram_wr
  ram.io.ram_rd <> rd.io.ram_rd

  io.ram_read_desc        <> rd.io.ram_read_desc
  io.ram_read_data        <> rd.io.ram_read_data

  io.ram_write_cmd <> wr.io.ram_write_cmd
 }


class unaligned_write_test extends AnyFreeSpec {
  "unaligned write test" in {
    simulate(new simple_wr) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.ram_write_cmd.ready.expect(true.B)
      dut.io.ram_write_cmd.valid.poke(true.B)
      dut.io.ram_write_cmd.bits.addr.poke(32.U)
      dut.io.ram_write_cmd.bits.data.poke("h11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111".U(512.W))
      dut.io.ram_write_cmd.bits.len.poke(64.U)

      dut.clock.step()

      dut.io.ram_write_cmd.valid.poke(false.B)

      dut.clock.step()

      // dut.io.ram_write_cmd.ready.expect(true.B)
      // dut.io.ram_write_cmd.valid.poke(true.B)
      // dut.io.ram_write_cmd.bits.addr.poke(96.U)
      // dut.io.ram_write_cmd.bits.data.poke("h22222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222".U(512.W))
      // dut.io.ram_write_cmd.bits.len.poke(64.U)

      dut.clock.step()

      dut.io.ram_write_cmd.valid.poke(false.B)

      dut.clock.step()

      // dut.io.ram_write_cmd.ready.expect(true.B)
      // dut.io.ram_write_cmd.valid.poke(true.B)
      // dut.io.ram_write_cmd.bits.addr.poke(160.U)
      // dut.io.ram_write_cmd.bits.data.poke("h33333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333".U(512.W))
      // dut.io.ram_write_cmd.bits.len.poke(64.U)

      dut.clock.step()

      dut.io.ram_write_cmd.valid.poke(false.B)

      dut.clock.step()

      dut.io.ram_read_desc.ready.expect(true.B)
      dut.io.ram_read_desc.valid.poke(true.B)
      dut.io.ram_read_desc.bits.ram_addr.poke(32.U(18.W))
      dut.io.ram_read_desc.bits.len.poke(64.U)

      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()

      dut.io.ram_read_data.valid.expect(true.B)
      dut.io.ram_read_data.ready.poke(true.B)
      dut.io.ram_read_data.bits.data.expect("h11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111".U)

      dut.clock.step()

      dut.io.ram_read_desc.ready.expect(true.B)
      dut.io.ram_read_desc.valid.poke(true.B)
      dut.io.ram_read_desc.bits.ram_addr.poke(96.U(18.W))
      dut.io.ram_read_desc.bits.len.poke(64.U)

      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()

      // dut.io.ram_read_data.valid.expect(true.B)
      // dut.io.ram_read_data.ready.poke(true.B)
      // dut.io.ram_read_data.bits.data.expect("h22222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222".U)

      dut.clock.step()

    }
  }
}
