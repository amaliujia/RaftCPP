//
// Created by 王 瑞 on 16-4-17.
//

#include "raft.h"

namespace raft {
PeerStatus_Status Raft::StatusMapping() {
  switch(this->status_) {
    case FOLLOWER:
      return PeerStatus_Status_FOLLOWER;
    case CANDIDATE:
      return PeerStatus_Status_CANDIDATE;
    case LEADER:
      return PeerStatus_Status_LEADER;
    default:
      return PeerStatus_Status_UNKNOWN;
  }
}

RaftStatus Raft::PeerStatusMapping(PeerStatus_Status status) {
  switch(status) {
    case PeerStatus_Status_FOLLOWER:
      return FOLLOWER;
    case PeerStatus_Status_CANDIDATE:
      return CANDIDATE;
    case PeerStatus_Status_LEADER:
      return LEADER;
    default:
      return UNKNOWN;
  }
}

void Raft::ThreadMain() {
  rpcz::application application;
  rpcz::server server(application);
  server.register_service(this);
  LOG(INFO) << "Serving requests on port " << this->rpc_port_;
  server.bind("tcp://" + this->rpc_addr_ + ":" + std::to_string(this->rpc_port_));
  application.run();
}

std::string Raft::GetInfo() {
  return this->rpc_addr_ + std::to_string(this->rpc_port_);
}

}