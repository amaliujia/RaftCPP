//
// Created by 王 瑞 on 16-5-27.
//

#ifndef RAFT_CONSENSUS_LOG_H
#define RAFT_CONSENSUS_LOG_H

#include "build/protofiles/raft.pb.h"
#include "common.h"
#include "struct.h"

class ConsensusLog {
public:
  ConsensusLog(const ConsensusLog&) = delete;
  ConsensusLog &operator=(const ConsensusLog&) = delete;
  ConsensusLog(const ConsensusLog &&) = delete;
  ConsensusLog &operator=(const ConsensusLog &&) = delete;

  ConsensusLog() {

  }

  size_t size() {
    return logs_.size();
  }

private:
  std::vector<LogEntry> logs_;
};


#endif //RAFT_CONSENSUS_LOG_H
