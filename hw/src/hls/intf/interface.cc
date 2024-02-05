#include "interface.hh"

void interface(
    hls::stream<ap_axiu<512, 32, 0, 0>> & addr_in,
    hls::stream<ap_uint<MSGHDR_SEND_SIZE>> & sendmsg,
    hls::stream<ap_uint<MSGHDR_RECV_SIZE>> & recvmsg,
    hls::stream<ap_uint<512>> & log_control 
    ) {

#pragma HLS interface axis port=addr_in
#pragma HLS interface axis port=sendmsg
#pragma HLS interface axis port=recvmsg
#pragma HLS interface axis port=log_control

#pragma HLS interface mode=ap_ctrl_none port=return
#pragma HLS pipeline II=1

    //ap_uint<64> off = addr_in.read();

    ap_axiu<512, 32, 0, 0> cmd = addr_in.read();
    // ap_uint<512> cmd = *(infmem + (off / 64));

    switch(cmd.user) {
	case 0: {
	    ap_uint<MSGHDR_SEND_SIZE> msghdr_send = cmd.data;
	    sendmsg.write(msghdr_send);
	    break;
	}
	case 64: {
	    ap_uint<MSGHDR_RECV_SIZE> msghdr_recv = cmd.data;
	    recvmsg.write(msghdr_recv);
	    break;
	}
	    // case 128: {
	    //     h2c_port_to_msgbuff.write(cmd.data);
	    //     break;
	    // }
	    // case 192: {
	    //     c2h_port_to_msgbuff.write(cmd.data);
	    //     break;
	    // }
	    // case 256: {
	    //     c2h_port_to_metadata.write(cmd.data);
	    //     break;
	    // }
	case 320: {
	    log_control.write(cmd.data);
	    break;
	}
    }
}
