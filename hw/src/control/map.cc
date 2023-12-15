#include "map.hh"

/* Dense BRAM based hashmap:
 *   - c2h_header_cam is high performant mapping structure
 *   - c2h_header_cam migrates entries to dense c2h_header_hashmap 
 */
// void c2h_header_hashmap(
//     hls::stream<header_t> & c2h_header_i,
//     hls::stream<header_t> & c2h_header_o
//     ) {
// 
// 
//     header_t c2h_header;
//     if (c2h_header_i.read_nb(c2h_header)) {
// 	c2h_header_o.write(c2h_header);
//     }

//     static hashmap_t<rpc_hashpack_t, local_id_t> rpc_hashmap;
//     static hashmap_t<peer_hashpack_t, peer_id_t> peer_hashmap;
// 
// #pragma HLS pipeline II=1
// 
//     header_t c2h_header;
//     if (c2h_header_i.read_nb(c2h_header)) {
// 	/* Perform the peer ID lookup regardless */
// 	peer_hashpack_t peer_query = {c2h_header.saddr};
// 	c2h_header.peer_id         = peer_hashmap.search(peer_query);
// 
// 	entry_t<peer_hashpack_t, peer_id_t> peer_ins;
// 	if (!IS_CLIENT(c2h_header.id)) {
// 	    rpc_hashpack_t query = {c2h_header.saddr, c2h_header.id, c2h_header.sport, 0};
// 
// 	    c2h_header.local_id = rpc_hashmap.search(query);
// 	} else {
// 	    c2h_header.local_id = c2h_header.id;
// 	}
// 
// 	c2h_header_o.write(c2h_header);
//     }
// 
//     static entry_t<rpc_hashpack_t, local_id_t> rpc_ins;
//     if (new_rpcmap_entry.read_nb(rpc_ins)) {
// 	rpc_hashmap.insert(rpc_ins);
//     }
// 
//     static entry_t<peer_hashpack_t, local_id_t> peer_ins;
//     if (new_peermap_entry.read_nb(peer_ins)) {
// 	peer_hashmap.insert(peer_ins);
//     }
// }

/* Content addressable memory for local ID lookup:
 *   - If the mapping has migrated to c2h_header_hashmap then the local ID is set in header
 *   - If not then check the CAM, and if that fails, generate a new ID
 */


/* Dense BRAM based hashmap:
 *   - c2h_header_cam is high performant mapping structure
 *   - c2h_header_cam migrates entries to dense c2h_header_hashmap 
 */
void c2h_header_hashmap(
    hls::stream<header_t> & c2h_header_i,
    hls::stream<header_t> & c2h_header_o
    ) {

    static hashmap_t<rpc_hashpack_t, local_id_t> rpc_hashmap;
    static hashmap_t<peer_hashpack_t, peer_id_t> peer_hashmap;

    // TODO bad
    /* Unique local RPC ID assigned when this core is the server */
    static stack_t<local_id_t, MAX_RPCS> server_ids;

    /* Unique Peer IDs */
    static stack_t<peer_id_t, MAX_PEERS> peer_ids;

#pragma HLS pipeline II=2

    header_t c2h_header;
    if (c2h_header_i.read_nb(c2h_header)) {
	if (c2h_header.peer_id == 0) {

	    /* Perform the peer ID lookup regardless */
	    peer_hashpack_t peer_query = {c2h_header.saddr};
	    c2h_header.peer_id          = peer_hashmap.search(peer_query);
	    
	    // If the peer is not registered, generate new ID and register it
	    if (c2h_header.peer_id == 0) {
		c2h_header.peer_id = peer_ids.pop();

		entry_t<peer_hashpack_t, local_id_t> new_entry = {peer_query, c2h_header.peer_id};

		peer_hashmap.insert(new_entry);

		// peer_cam.insert(new_entry);
		// new_peermap_entry.write(new_entry);
	    }
	}

	if (c2h_header.local_id == 0) {
	    if (!IS_CLIENT(c2h_header.id)) {
		/* If we are the client then the entry already exists inside
		 * rpc_client. If we are the server then the entry needs to be
		 * created inside rpc_server if this is the first packet received
		 * for a message. If it is not the first message then the entry
		 * already exists in server_rpcs.
		 */
		rpc_hashpack_t rpc_query = {c2h_header.saddr, c2h_header.id, c2h_header.sport, 0};

		c2h_header.local_id = rpc_hashmap.search(rpc_query);

		if (c2h_header.local_id == 0) {
		    c2h_header.local_id = (2 * server_ids.pop()) + 1;

		    std::cerr << "ASSIGNED NEW SERVER ID " << c2h_header.local_id << std::endl;

		    entry_t<rpc_hashpack_t, local_id_t> new_entry = {rpc_query, c2h_header.local_id};
		    rpc_hashmap.insert(new_entry);
		    // If the rpc is not registered, generate a new RPC ID and register it 
		    // rpc_cam.insert(new_entry);
		    // new_rpcmap_entry.write(new_entry);
		} 
	    } else {
		c2h_header.local_id = c2h_header.id;
	    }
	}

	c2h_header_o.write(c2h_header);
    }
}
