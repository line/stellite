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

#ifndef STELLITE_STATS_SERVER_STATS_MACRO_H_
#define STELLITE_STATS_SERVER_STATS_MACRO_H_

#include "stellite/stats/server_stats.h"
#include "stellite/stats/server_stats_recorder.h"

#define STATIC_SERVER_STAT_BLOCK(stats_method_invocation)                      \
  net::ServerStatsRecorder* recorder =                                         \
      net::ServerStatsRecorder::GetInstance();                                 \
  recorder->stats_method_invocation

#define SERVER_STAT_ADD(tag, amount) \
    STATIC_SERVER_STAT_BLOCK(AddStat(tag, amount))

#define SERVER_STAT_RESET(tag) \
    STATIC_SERVER_STAT_BLOCK(name, Reset())

#endif // STELLITE_STATS_SERVER_STATS_MACRO_H_
