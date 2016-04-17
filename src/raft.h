//
// Created by 王 瑞 on 16-4-17.
//

#ifndef RAFT_RAFT_H
#define RAFT_RAFT_H

#include "raft.pb.h"
#include "raft.rpcz.h"
#include "common.h"

namespace raft {


enum RaftStatus {
  UNKNOWN,
  LEADER,
  FOLLOWER,
  CANDIDATE
};

struct RaftCommand {
  int term;
};

typedef struct Raftcommand Raftcommand;

class Raft : public RaftService {

public:
  Raft(std::string addr, int port, int peer_id,
       const std::vector<std::string>& peers_addrs)
      :peers(peers_addrs), id(peer_id), term(-1), status(FOLLOWER){
    this->rpc_addr = addr;
    this->rpc_port = port;

    // start rpc service here. Do not need to join
    rpc_thread = std::thread(&Raft::ThreadMain, this);
  }

  std::string GetInfo() {
    return this->rpc_addr + std::to_string(this->rpc_port);
  }

  virtual void GetStatus(const Empty& e, rpcz::reply<PeerStatus> reply) {
    PeerStatus status;
    status.set_term(this->term);
    status.set_status(StatusMapping());
    reply.send(status);
  }

  virtual void Vote(const VoteRequest& request, rpcz::reply<VoteReply> reply) {
      VoteReply response;
      reply.send(response);
  }

private:
  PeerStatus_Status StatusMapping() {
    switch(this->status) {
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

  RaftStatus PeerStatusMapping(int i) {
    switch(i) {
      case 1:
        return FOLLOWER;
      case 2:
        return CANDIDATE;
      case 3:
        return LEADER;
      default:
        return UNKNOWN;
    }
  }

  void ThreadMain() {
    rpcz::application application;
    rpcz::server server(application);
    server.register_service(this);
    std::cout << "Serving requests on port " << this->rpc_port << std::endl;
    server.bind("tcp://" + this->rpc_addr + ":" + std::to_string(this->rpc_port));
    application.run();
  }

private:
  std::string rpc_addr;
  int rpc_port;

  std::thread rpc_thread;

  int id;
  std::vector<std::string> peers;
  int term;
  RaftStatus status;

  // std::unordered_map<RaftCommand> command_log;
  // std::vector<RaftCommand> logs;
};

}


#endif //RAFT_RAFT_H
