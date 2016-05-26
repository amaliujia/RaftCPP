//
// Created by 王 瑞 on 16-4-17.
//

#include "raft.h"

namespace raft {

Raft::Raft(std::string addr, int port, int peer_id, const std::vector<std::string>& peers_addrs)
    :self_id_(peers_addrs[peer_id]), term_(-1), status_(FOLLOWER), vote_for_(""), is_dead_(false){
  this->rpc_addr_ = addr;
  this->rpc_port_ = port;

  this->peers_.insert(peers_addrs.begin(), peers_addrs.end());

  this->leader_active_time_ = std::chrono::duration_cast< std::chrono::milliseconds >(
  std::chrono::system_clock::now().time_since_epoch()).count();

  // start rpc service and raft service here. Do not need to join.
  rpc_thread_ = std::thread(&Raft::ThreadMain, this);
  raft_thread_ = std::thread(&Raft::LaunchRaftDemon, this);
}

Raft::~Raft() {
  LOG(INFO) << "Destroy raft " << this->rpc_addr_ << ":" << this->rpc_port_;
  this->Kill();
}

void Raft::Kill() {
  if (this->is_dead_ == false) {
    this->is_dead_ = true;
    this->raft_thread_.join();
    this->application_.terminate();
    this->rpc_thread_.join();
    this->status_ = DEAD;
  } else {
    LOG(INFO) << "Killing a raft node who has been killed";
  }
}

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

void Raft::GetStatus(const Empty& e, rpcz::reply<PeerStatus> reply) {
  PeerStatus status;
  status.set_term(this->term_);
  status.set_status(StatusMapping());
  reply.send(status);
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