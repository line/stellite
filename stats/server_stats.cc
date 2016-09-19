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

#include "stellite/stats/server_stats.h"

#include "base/process/process.h"

namespace net {

QuicStats::QuicStats()
    : estimated_bandwidth(0),
      bytes_received(0),
      bytes_retransmitted(0),
      bytes_sent(0),
      bytes_spuriously_retransmitted(0),
      max_packet_size(0),
      max_received_packet_size(0),
      stream_bytes_received(0),
      stream_bytes_sent(0),
      packets_discarded(0),
      packets_dropped(0),
      packets_lost(0),
      packets_processed(0),
      packets_received(0),
      packets_reordered(0),
      packets_retransmitted(0),
      packets_sent(0),
      packets_spuriously_retransmitted(0),
      slowstart_packets_lost(0),
      slowstart_packets_sent(0),
      max_sequence_reordering(0),
      max_time_reordering_us(0),
      min_rtt_us(0),
      srtt_us(0),
      crypto_retransmit_count(0),
      loss_timeout_count(0),
      rto_count(0),
      tlp_count(0),
      tcp_loss_events(0),
      connection_count(0) {
}

QuicStats::QuicStats(const QuicStats& other)
    : QuicStats() {
  Add(other);
}

QuicStats::~QuicStats() {}

void QuicStats::AddSample(const QuicConnectionStats& sample) {
  estimated_bandwidth += sample.estimated_bandwidth.ToBytesPerSecond();

  bytes_received                   += sample.bytes_received;
  bytes_retransmitted              += sample.bytes_retransmitted;
  bytes_sent                       += sample.bytes_sent;
  bytes_spuriously_retransmitted   += sample.bytes_spuriously_retransmitted;
  max_packet_size                  += sample.max_packet_size;
  max_received_packet_size         += sample.max_received_packet_size;
  stream_bytes_received            += sample.stream_bytes_received;
  stream_bytes_sent                += sample.stream_bytes_sent;
  packets_discarded                += sample.packets_discarded;
  packets_dropped                  += sample.packets_dropped;
  packets_lost                     += sample.packets_lost;
  packets_processed                += sample.packets_processed;
  packets_received                 += sample.packets_received;
  packets_reordered                += sample.packets_reordered;
  packets_retransmitted            += sample.packets_retransmitted;
  packets_sent                     += sample.packets_sent;
  packets_spuriously_retransmitted += sample.packets_spuriously_retransmitted;
  slowstart_packets_lost           += sample.slowstart_packets_lost;
  slowstart_packets_sent           += sample.slowstart_packets_sent;
  max_sequence_reordering          += sample.max_sequence_reordering;
  max_time_reordering_us           += sample.max_time_reordering_us;
  min_rtt_us                       += sample.min_rtt_us;
  srtt_us                          += sample.srtt_us;
  crypto_retransmit_count          += sample.crypto_retransmit_count;
  loss_timeout_count               += sample.loss_timeout_count;
  rto_count                        += sample.rto_count;
  tlp_count                        += sample.tlp_count;
  tcp_loss_events                  += sample.tcp_loss_events;
  connection_count                 += 1;
}

void QuicStats::Add(const QuicStats& other) {
  estimated_bandwidth              += other.estimated_bandwidth;
  bytes_received                   += other.bytes_received;
  bytes_retransmitted              += other.bytes_retransmitted;
  bytes_sent                       += other.bytes_sent;
  bytes_spuriously_retransmitted   += other.bytes_spuriously_retransmitted;
  max_packet_size                  += other.max_packet_size;
  max_received_packet_size         += other.max_received_packet_size;
  stream_bytes_received            += other.stream_bytes_received;
  stream_bytes_sent                += other.stream_bytes_sent;
  packets_discarded                += other.packets_discarded;
  packets_dropped                  += other.packets_dropped;
  packets_lost                     += other.packets_lost;
  packets_processed                += other.packets_processed;
  packets_received                 += other.packets_received;
  packets_reordered                += other.packets_reordered;
  packets_retransmitted            += other.packets_retransmitted;
  packets_sent                     += other.packets_sent;
  packets_spuriously_retransmitted += other.packets_spuriously_retransmitted;
  slowstart_packets_lost           += other.slowstart_packets_lost;
  slowstart_packets_sent           += other.slowstart_packets_sent;
  max_sequence_reordering          += other.max_sequence_reordering;
  max_time_reordering_us           += other.max_time_reordering_us;
  min_rtt_us                       += other.min_rtt_us;
  srtt_us                          += other.srtt_us;
  crypto_retransmit_count          += other.crypto_retransmit_count;
  loss_timeout_count               += other.loss_timeout_count;
  rto_count                        += other.rto_count;
  tlp_count                        += other.tlp_count;
  tcp_loss_events                  += other.tcp_loss_events;
  connection_count                 += other.connection_count;
}

void QuicStats::Sub(const QuicStats& other) {
  estimated_bandwidth              -= other.estimated_bandwidth;
  bytes_received                   -= other.bytes_received;
  bytes_retransmitted              -= other.bytes_retransmitted;
  bytes_sent                       -= other.bytes_sent;
  bytes_spuriously_retransmitted   -= other.bytes_spuriously_retransmitted;
  max_packet_size                  -= other.max_packet_size;
  max_received_packet_size         -= other.max_received_packet_size;
  stream_bytes_received            -= other.stream_bytes_received;
  stream_bytes_sent                -= other.stream_bytes_sent;
  packets_discarded                -= other.packets_discarded;
  packets_dropped                  -= other.packets_dropped;
  packets_lost                     -= other.packets_lost;
  packets_processed                -= other.packets_processed;
  packets_received                 -= other.packets_received;
  packets_reordered                -= other.packets_reordered;
  packets_retransmitted            -= other.packets_retransmitted;
  packets_sent                     -= other.packets_sent;
  packets_spuriously_retransmitted -= other.packets_spuriously_retransmitted;
  slowstart_packets_lost           -= other.slowstart_packets_lost;
  slowstart_packets_sent           -= other.slowstart_packets_sent;
  max_sequence_reordering          -= other.max_sequence_reordering;
  max_time_reordering_us           -= other.max_time_reordering_us;
  min_rtt_us                       -= other.min_rtt_us;
  srtt_us                          -= other.srtt_us;
  crypto_retransmit_count          -= other.crypto_retransmit_count;
  loss_timeout_count               -= other.loss_timeout_count;
  rto_count                        -= other.rto_count;
  tlp_count                        -= other.tlp_count;
  tcp_loss_events                  -= other.tcp_loss_events;
  connection_count                 -= other.connection_count;
}

void QuicStats::Reset() {
  estimated_bandwidth              = 0;
  bytes_received                   = 0;
  bytes_retransmitted              = 0;
  bytes_sent                       = 0;
  bytes_spuriously_retransmitted   = 0;
  max_packet_size                  = 0;
  max_received_packet_size         = 0;
  stream_bytes_received            = 0;
  stream_bytes_sent                = 0;
  packets_discarded                = 0;
  packets_dropped                  = 0;
  packets_lost                     = 0;
  packets_processed                = 0;
  packets_received                 = 0;
  packets_reordered                = 0;
  packets_retransmitted            = 0;
  packets_sent                     = 0;
  packets_spuriously_retransmitted = 0;
  slowstart_packets_lost           = 0;
  slowstart_packets_sent           = 0;
  max_sequence_reordering          = 0;
  max_time_reordering_us           = 0;
  min_rtt_us                       = 0;
  srtt_us                          = 0;
  crypto_retransmit_count          = 0;
  loss_timeout_count               = 0;
  rto_count                        = 0;
  tlp_count                        = 0;
  tcp_loss_events                  = 0;
  connection_count                 = 0;
}

HttpStats::HttpStats()
  : http_sent(0),
    http_timeout(0),
    http_connection_failed(0),
    http_received(0) {
}

HttpStats::HttpStats(const HttpStats& other)
  : HttpStats() {
  Add(other);
}

HttpStats::~HttpStats() {}

void HttpStats::Add(const HttpStats& other) {
  http_sent              += other.http_sent;
  http_timeout           += other.http_timeout;
  http_connection_failed += other.http_connection_failed;
  http_received          += other.http_received;
}

void HttpStats::Sub(const HttpStats& other) {
  http_sent              -= other.http_sent;
  http_timeout           -= other.http_timeout;
  http_connection_failed -= other.http_connection_failed;
  http_received          -= other.http_received;
}

void HttpStats::Reset() {
  http_sent              = 0;
  http_timeout           = 0;
  http_connection_failed = 0;
  http_received          = 0;
}

} // namespace net
