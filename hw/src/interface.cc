#include "homa.hh"
#include "interface.hh"

void interface(
    ap_uint<512> infmem[128],
    hls::stream<ap_uint<64>> & addr_in,
    hls::stream<msghdr_send_t> & sendmsg,
    hls::stream<msghdr_recv_t> & recvmsg,
    hls::stream<port_to_phys_t> & h2c_port_to_msgbuff,
    hls::stream<port_to_phys_t> & c2h_port_to_msgbuff,
    hls::stream<port_to_phys_t> & c2h_port_to_metadata,
    hls::stream<ap_uint<512>> & log_control 
    ) {

#pragma HLS interface bram port=infmem
#pragma HLS interface axis port=addr_in
#pragma HLS interface axis port=sendmsg
#pragma HLS interface axis port=recvmsg
#pragma HLS interface axis port=h2c_port_to_msgbuff
#pragma HLS interface axis port=c2h_port_to_msgbuff
#pragma HLS interface axis port=c2h_port_to_metadata
#pragma HLS interface axis port=log_control

#pragma HLS interface mode=ap_ctrl_none port=return
#pragma HLS pipeline II=1

    ap_uint<64> off = addr_in.read();

    ap_uint<512> cmd = *(infmem + (off / 64));

    switch(off) {
	case 0: {
	    msghdr_send_t msghdr_send;
	    msghdr_send.data = cmd;
	    sendmsg.write(msghdr_send);
	    break;
	}
	case 64: {
	    msghdr_recv_t msghdr_recv;
	    msghdr_recv.data = cmd;
	    recvmsg.write(msghdr_recv);
	    break;
	}
	case 128: {
	    h2c_port_to_msgbuff.write(cmd);
	    break;
	}
	case 192: {
	    c2h_port_to_msgbuff.write(cmd);
	    break;
	}
	case 256: {
	    c2h_port_to_metadata.write(cmd);
	    break;
	}
	case 320: {
	    log_control.write(cmd);
	    break;
	}
    }
}
