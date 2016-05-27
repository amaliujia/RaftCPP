//
// Created by 王 瑞 on 16-5-27.
//

#ifndef RAFT_STRUCT_H
#define RAFT_STRUCT_H

#include "common.h"

namespace raft {

enum RaftStatus {
  UNKNOWN,
  DEAD,
  LEADER,
  FOLLOWER,
  CANDIDATE
};

enum RaftOp {
  INSERT,
  DELETE,
  QUERY,
  INVALID
};

struct LogEntry {
  uint32_t term;
  uint32_t index;

  RaftOp op;
  std::string key;
  std::string val;
};

}

#endif //RAFT_STRUCT_H
