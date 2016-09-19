// Copyright 2016 LINE Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef STELLITE_STATS_SERVER_STATS_H_
#define STELLITE_STATS_SERVER_STATS_H_

#include "net/quic/quic_connection_stats.h"

namespace net {
typedef uint64_t HttpPacketCount;
typedef uint32_t StatTag;

#define STAT_TAG(a, b, c, d) \
    static_cast<StatTag>(d << 24 | c << 16 | b << 8 | a)

struct QuicStats {
  QuicStats();
  QuicStats(const QuicStats& other);
  ~QuicStats();

  void AddSample(const QuicConnectionStats& connection_stats);
  void Add(const QuicStats& other);
  void Sub(const QuicStats& other);
  void Reset();

  QuicByteCount estimated_bandwidth;
  QuicByteCount bytes_received;
  QuicByteCount bytes_retransmitted;
  QuicByteCount bytes_sent;
  QuicByteCount bytes_spuriously_retransmitted;
  QuicByteCount max_packet_size;
  QuicByteCount max_received_packet_size;
  QuicByteCount stream_bytes_received;
  QuicByteCount stream_bytes_sent;
  QuicPacketCount packets_discarded;
  QuicPacketCount packets_dropped;
  QuicPacketCount packets_lost;
  QuicPacketCount packets_processed;
  QuicPacketCount packets_received;
  QuicPacketCount packets_reordered;
  QuicPacketCount packets_retransmitted;
  QuicPacketCount packets_sent;
  QuicPacketCount packets_spuriously_retransmitted;
  QuicPacketCount slowstart_packets_lost;
  QuicPacketCount slowstart_packets_sent;
  QuicPacketNumber max_sequence_reordering;
  int64_t max_time_reordering_us;
  int64_t min_rtt_us;
  int64_t srtt_us;
  size_t crypto_retransmit_count;
  size_t loss_timeout_count;
  size_t rto_count;
  size_t tlp_count;
  uint32_t tcp_loss_events;
  uint32_t connection_count;
};

struct HttpStats {
  HttpStats();
  HttpStats(const HttpStats& other);
  ~HttpStats();

  void Add(const HttpStats& other);
  void Sub(const HttpStats& other);
  void Reset();

  HttpPacketCount http_sent;
  HttpPacketCount http_timeout;
  HttpPacketCount http_connection_failed;
  HttpPacketCount http_received;
};

const StatTag kHttpSent     = STAT_TAG('H', 'S', 'E', 'N'); // HSEN
const StatTag kHttpTimeout  = STAT_TAG('H', 'T', 'I', 'O'); // HTIO
const StatTag kHttpFailed   = STAT_TAG('H', 'C', 'F', 'A'); // HCFA
const StatTag kHttpReceived = STAT_TAG('H', 'R', 'E', 'C'); // HREC

} // namespace net

#endif // STELLITE_STATS_SERVER_STATS_H_
