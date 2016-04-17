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
  Raft(std::string addr, int port) {
    this->rpc_addr = addr;
    this->rpc_port = port;

    // start rpc service here. Do not need to join
    rpc_thread = std::thread(&Raft::ThreadMain, this);
  }

  std::string GetInfo() {
    return this->rpc_addr + std::to_string(this->rpc_port);
  }

  virtual void Vote(const VoteRequest& request, rpcz::reply<VoteReply> reply) {
      VoteReply response;
      reply.send(response);
  }

private:
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

  int term;
  RaftStatus status;

  // std::unordered_map<RaftCommand> command_log;
  // std::vector<RaftCommand> logs;
};

}


#endif //RAFT_RAFT_H
