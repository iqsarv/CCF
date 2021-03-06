// Copyright (c) Microsoft Corporation.
// Copyright (c) 1999 Miguel Castro, Barbara Liskov.
// Copyright (c) 2000, 2001 Miguel Castro, Rodrigo Rodrigues, Barbara Liskov.
// Licensed under the MIT license.

#pragma once

/*
 * Definitions of various types.
 */

#include "consensus/pbft/pbfttypes.h"
#include "parameters.h"

#include <array>
#include <cstdint>
#include <functional>

using Long = int64_t;
using ULong = uint64_t;

using Index = Long;
using Seqno = Long;
using View = Long;
using Request_id = ULong;

typedef struct sockaddr_in Addr;

static constexpr Long Long_max = std::numeric_limits<Long>::max();
static constexpr View View_max = std::numeric_limits<View>::max();
static constexpr Seqno Seqno_max = std::numeric_limits<Seqno>::max();

#include <bitset>
typedef std::bitset<Max_requests_in_batch> BR_map;

struct _Byz_buffer
{
  int size;
  char* contents;
  void* opaque;
};

typedef struct _Byz_buffer Byz_buffer;
typedef struct _Byz_buffer Byz_req;
typedef struct _Byz_buffer Byz_rep;

static const uint32_t MERKLE_ROOT_SIZE = 32;
struct ByzInfo
{
  std::array<uint8_t, MERKLE_ROOT_SIZE> full_state_merkle_root;
  std::array<uint8_t, MERKLE_ROOT_SIZE> replicated_state_merkle_root;
  int64_t ctx;
  bool include_merkle_roots;
};

using ExecCommand = std::function<int(
  Byz_req*,
  Byz_rep&,
  Byz_buffer*,
  // client id
  int,
  Request_id,
  // read only
  bool,
  // start of the pbft Request contents
  uint8_t* req_start,
  // pbft Request contents size
  size_t req_size,
  Seqno,
  ByzInfo&,
  // if tx is nullptr we are in normal execution, otherwise we are in playback
  // mode
  ccf::Store::Tx*)>;