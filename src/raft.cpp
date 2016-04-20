//
// Created by 王 瑞 on 16-4-17.
//

#include "raft.h"

namespace raft {
PeerStatus::Status Raft::StatusMapping() {
  switch(this->status_) {
    case FOLLOWER:
      return PeerStatus::FOLLOWER;
    case CANDIDATE:
      return PeerStatus::CANDIDATE;
    case LEADER:
      return PeerStatus::LEADER;
    default:
      return PeerStatus::UNKNOWN;
  }
}

RaftStatus Raft::PeerStatusMapping(PeerStatus::Status status) {
  switch(status) {
    case PeerStatus::FOLLOWER:
      return FOLLOWER;
    case PeerStatus::CANDIDATE:
      return CANDIDATE;
    case PeerStatus::LEADER:
      return LEADER;
    default:
      return UNKNOWN;
  }
}

void Raft::ThreadMain() {

  rpcz::server server(this->application_);
  server.register_service(this);
  LOG(INFO) << "Serving requests on port " << this->rpc_port_;
  server.bind("tcp://" + this->rpc_addr_ + ":" + std::to_string(this->rpc_port_));
  this->application_.run();
}

std::string Raft::GetInfo() {
  return this->rpc_addr_ + std::to_string(this->rpc_port_);
}

}