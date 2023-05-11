#include <string.h>

#include "homa.hh"
#include "link.hh"
#include "dma.hh"

/**
 * proc_link_egress() - 
 */
void proc_link_egress(hls::stream<srpt_xmit_entry_t> & srpt_xmit_queue_next,
		      hls::stream<srpt_grant_entry_t> & srpt_grant_queue_next,
		      hls::stream<xmit_req_t> & xmit_buffer_request,
		      hls::stream<xmit_mblock_t> & xmit_buffer_response,
		      hls::stream<rpc_id_t> & rpc_buffer_request,
		      hls::stream<homa_rpc_t> & rpc_buffer_response,
		      hls::stream<rpc_id_t> & rexmit_rpcs,
		      hls::stream<raw_frame_t> & link_egress) {
#pragma HLS PIPELINE II=1
 
  srpt_xmit_entry_t srpt_xmit_entry;
  srpt_xmit_queue_next.read(srpt_xmit_entry);

  srpt_grant_entry_t srpt_grant_entry;
  srpt_grant_queue_next.write(srpt_grant_entry);

  rpc_id_t rexmit;
  rexmit_rpcs.read(rexmit);

  homa_rpc_t homa_rpc;
  rpc_buffer_request.write(srpt_xmit_entry.rpc_id);
  rpc_buffer_response.read(homa_rpc);

  raw_frame_t raw_frame;
  // TODO this is temporary and wrong just to test datapath
  for (int i = 0; i < XMIT_BUFFER_SIZE; ++i) {
    xmit_mblock_t mblock;
    //xmit_req_t xmit_req = {homa_rpc.msgout.xmit_id, (xmit_offset_t) i};
    xmit_buffer_request.write((xmit_req_t) {homa_rpc.msgout.xmit_id, (xmit_offset_t) i});
    xmit_buffer_response.read(mblock);
    raw_frame.data[i%6] = mblock;
    if (i % 6 == 0) link_egress.write(raw_frame);
  }

}

/**
 * proc_link_ingress() - 
 * 
 */
void proc_link_ingress(hls::stream<raw_frame_t> & link_ingress,
		       hls::stream<srpt_grant_entry_t> & srpt_grant_queue_insert,
		       hls::stream<srpt_xmit_entry_t> & srpt_xmit_queue_update,
		       hls::stream<rpc_hashpack_t> & rpc_table_request,
		       hls::stream<rpc_id_t> & rpc_table_response,
		       hls::stream<rpc_id_t> & rpc_buffer_request,   		       
		       hls::stream<homa_rpc_t> & rpc_buffer_response,
		       hls::stream<rpc_id_t> & rexmit_touch,
		       hls::stream<dma_egress_req_t> & dma_egress_reqs) {
#pragma HLS pipeline II=1 

  /*
   * Are there incoming ethernet frames to process
   * Raw frame max size for non jumbo frames:
   *   6 + 6 + 4 + 2 + 1500 + 4 = 1522 bytes 
   */
  if (!link_ingress.empty()) {
    raw_frame_t raw_frame;
    link_ingress.read(raw_frame);

    ethernet_header_t * ethheader = (ethernet_header_t*) &(raw_frame.data);

    if (ethheader->ethertype != ETHERTYPE_IPV6) return;

    ipv6_header_t * ipv6header = (ipv6_header_t*) (((char*) ethheader) + sizeof(ethernet_header_t));

    // Version is a constant that should always be 6? 
    if (ipv6header->version != 0x6) return;

    // TODO traffic class? Nothing to do here?

    // TODO flow label? Nothing to do here?

    int payload_length = ipv6header->payload_length;

    if (ipv6header->next_header != IPPROTO_HOMA) return;

    // TODO Hop limit? Nothing to do here?

    // TODO Dest address? Nothing to do here?

    common_header_t * commonheader = (common_header_t *) (((char*) ipv6header) + sizeof(ipv6_header_t));

    rpc_hashpack_t hashpack = {
      ipv6header->src_address,
      commonheader->sender_id,
      commonheader->sport,
      0
    };

    // Grab the local RPC ID
    rpc_table_request.write(hashpack);
    rpc_id_t local_id;
    rpc_table_response.read(local_id);


    homa_rpc_t homa_rpc;
    rpc_buffer_request.write(local_id);
    rpc_buffer_response.read(homa_rpc);

    // Grab the RPC struct with local name

    switch(commonheader->doff) {
      case DATA: {
	// TODO check if we need to issue more grants
	
	rexmit_touch.write(local_id);
	break;
      }

      case GRANT: {
	// TODO send new grant to the xmit SRPT queue
	break;
      }

      case RESEND: {
	// TODO queue some xmit to the link_egress
	break;
      }

      case BUSY: {
	// TODO don't perform long time out in the timer core
	break;
      }

      case ACK: {
	// TODO update the range of received bytes?
	break;
      }

      case NEED_ACK: {
	// TODO 
	break;
      }

      default:
	break;
    }
  }

  srpt_grant_entry_t entry;
  srpt_grant_queue_insert.write(entry);

  srpt_xmit_entry_t xentry;
  srpt_xmit_queue_update.write(xentry);

  rpc_id_t touch;
  rexmit_touch.write(touch);

  // Has a full ethernet frame been buffered? If so, grab it.
  //link_ingress.read(frame);

  // TODO this needs to get the address of the output
  for (int i = 0; 6; ++i) {
    dma_egress_req_t req; //= {i, frame.data[i]};
    dma_egress_reqs.write(req);
  }
}

