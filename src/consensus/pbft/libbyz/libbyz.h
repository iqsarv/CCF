// Copyright (c) Microsoft Corporation.
// Copyright (c) 1999 Miguel Castro, Barbara Liskov.
// Copyright (c) 2000, 2001 Miguel Castro, Rodrigo Rodrigues, Barbara Liskov.
// Licensed under the MIT license.

#pragma once

#include "consensus/pbft/pbftpreprepares.h"
#include "consensus/pbft/pbftrequests.h"
#include "consensus/pbft/pbfttypes.h"
#include "nodeinfo.h"

/* Because of FILE parameter */
#include <stdio.h>

/* Should be a power of 2 less than or equal to the vm page size */
static const int Block_size = 128;

#include "types.h"

class INetwork;
class IMessageReceiveBase;

/*
 * Client
 */

int Byz_init_client(const NodeInfo& node_info, INetwork* network);
/* Effects: Initializes a libbyz client process using the information in
   node_info that includes the private key. */

int Byz_alloc_request(Byz_req* req, int size);
/* Requires: "req" points to a Byz_req structure
   Effects: If successful returns 0 and initializes "req" by allocating internal
   memory for the request, making "req->contents" point to that memory, and
   "req->size" contain the number of bytes that can be used starting from
   "req->contents". If it fails it returns -1. */

void Byz_free_request(Byz_req* req);
/* Requires: "req" points to a Byz_req structure whose "req->contents" value
   was obtained by calling Byz_alloc_req.
   Effects: Frees the internal memory associated with "req". */

void Byz_free_reply(Byz_rep* rep);
/* Requires: "rep" points to a Byz_rep structure whose "req->contents" value
   was obtained by calling Byz_recv_reply.
   Effects: Frees the internal memory associated with "rep". */

int Byz_send_request(Byz_req* req, bool ro);
/* Requires: "req" points to a Byz_req structure whose "req->contents"
   value was obtained by calling Byz_alloc_req and whose "req->size"
   value is the actual number of bytes in the request.
   "read_only" is true iff the request
   does not modify the service state. All previous request have been
   followed by an invocation of Byz_recv_reply.

   Effects: Invokes the request. If successful, returns 0.
   Otherwise returns -1. */

int Byz_recv_reply(Byz_rep* rep);
/* Requires: "rep" points to an uninitialized Byz_rep structure.
   There was a previous request for which there was not an invocation
   of Byz_recv_reply.

   If successful, initializes "rep" to
   point to the reply and returns 0. ("rep" must be deallocated by the
   caller using Byz_free_reply.) Otherwise, does not initialize "rep"
   and returns -1. */

int Byz_invoke(Byz_req* req, Byz_rep* rep, bool ro);
/* Requires: "req" points to a Byz_req structure whose "req->contents"
   value was obtained by calling Byz_alloc_req and whose "req->size"
   value is the actual number of bytes in the request.
   "read_only" is true iff the request
   does not modify the service state. All previous request have been
   followed by an invocation of Byz_recv_reply.
   "rep" points to an uninitialized Byz_rep structure.

   Effects: Invokes the request. If successful, initializes "rep" to
   point to the reply and returns 0. ("rep" must be deallocated by the
   caller using Byz_free_reply.) Otherwise, does not initialize "rep"
   and returns -1. */

/*
 * Principals
 */

void Byz_add_principal(const PrincipalInfo& principal_info);

void Byz_configure_principals();
/* Effects: principal configures all known principal info */

/*
 * Replica
 */

int Byz_init_replica(
  const NodeInfo& node_info,
  char* mem,
  unsigned int size,
  ExecCommand exec,
  void (*comp_ndet)(Seqno, Byz_buffer*),
  int ndet_max_len,
  INetwork* network,
  pbft::RequestsMap& pbft_requests_map,
  pbft::PrePreparesMap& pbft_pre_prepares_map,
  pbft::PbftStore& store_,
  IMessageReceiveBase** message_receiver = nullptr);
/* Requires: "mem" is vm page aligned and "size" is a multiple of the vm page
   size.

   Effects: Initializes a libbyz replica process using the information
   node_info, that includes its private key.
   The state managed by the replica is set to the "size" contiguous bytes
   starting at "mem", and the replica will call the "exec" upcall to execute
   requests and the "comp_ndet" upcall to compute non-deterministic choices for
   each request. "ndet_max_len" must be the maximum number of bytes comp_ndet
   places in its argument buffer. The replication code uses the beginning of
   "mem" to store protocol data. If successful, the function returns the number
   of bytes used which is guaranteed to be a multiple of the vm page size.
   Otherwise, the function returns -1.

   The specs for the upcalls are:
   int exec(Byz_req *req, Byz_rep *rep, Byz_buffer *ndet, int client, bool
   read_only);

   Effects:
   - "req->contents" is a character array with a request with
   "req->size" bytes

   - "rep->contents" is a character array where exec should place the
   reply to the request. This reply cannot excede the value of
   "rep->size" on entry to the exec. On exit from exec, "rep->size"
   must contain the actual number of bytes in the reply.

   - "ndet->contents" is a character array with non-deterministic
   choices associated with the request and is "ndet->size" bytes long

   - "client" is the identifier of the client that executed the
   request (index of client's public key in configuration file)

   - "read_only" is true iff the request should execute only if it does
   not modify the replica's state.

   If "read_only" is true "exec" should not execute the request in
   "req" unless it is in fact read only. If the request is not read
   only it should return -1 without modifying the service
   state. Except for this case exec should execute the request in req
   using the non-deterministic choices and place the replies in
   rep. The execution of the request will typically require access
   control checks using the client identifier. If the request executes
   successfully exec should return 0.


   void comp_ndet(Seqno seqno, Byz_buffer *ndet);
   Effects: "ndet->contents" is a character array where comp_ndet
   should place the non-deterministic choices (e.g., time) associated
   with the request with sequence number seqno. These choices cannot
   excede the value of "ndet->size" on entry to the comp_ndet. On exit
   from comp_ndet, "ndet->size" must contain the actual number of
   bytes in the choices.

*/

void Byz_start_replica();
/* Effects: Starts PBFT replica. This function must be called after
 * Byz_init_replica */

/*
 * The service code should call one of the following functions before
 * it modifies the state managed by the replica.
 *
 */

void Byz_modify(void* mem, int size);
/* Effects: Informs library that the bytes between "mem" and
   "mem+size" are about to be modified if "mem" and "mem+size-1" are
   within the replica's state. */

void Byz_replica_run();
/* Effects: Loops executing requests. */

void Byz_reset_stats();
/* Effects: Resets library's statistics counters */

void Byz_print_stats();
/* Effects: Print library statistics to stdout */

void Byz_reset_client();
/* Reverts client to its initial state to ensure independence of experimental
   points */
