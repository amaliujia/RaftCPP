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
      :peers_(peers_addrs), id_(peer_id), term_(-1), status_(FOLLOWER), vote_for_(-1){
    this->rpc_addr_ = addr;
    this->rpc_port_ = port;

    this->leader_active_time_ = std::chrono::duration_cast< std::chrono::milliseconds >(
    std::chrono::system_clock::now().time_since_epoch()).count();

    // start rpc service and raft service here. Do not need to join.
    rpc_thread_ = std::thread(&Raft::ThreadMain, this);
    raft_thread_ = std::thread(&Raft::LaunchRaftDemon, this);
  }

  std::string GetInfo();

  virtual void GetStatus(const Empty& e, rpcz::reply<PeerStatus> reply) {
    PeerStatus status;
    status.set_term(this->term_);
    status.set_status(StatusMapping());
    reply.send(status);
  }

  virtual void Vote(const VoteRequest& request, rpcz::reply<VoteReply> reply) {
    std::unique_lock<std::mutex> locker(lock_);

    VoteReply response;
    response.set_term(this->term_);
    if (this->status_ == LEADER || this->status_ == CANDIDATE) {
      response.set_vote(false);
    } else if (this->vote_for_ != -1){
      response.set_vote(false);
    } else {
      response.set_vote(true);
      this->vote_for_ = request.candidate_id();
      this->leader_active_time_ = std::chrono::duration_cast< std::chrono::milliseconds >(
      std::chrono::system_clock::now().time_since_epoch()).count();
    }
    locker.unlock();
    reply.send(response);
  }

  virtual void AppendMsg(const Empty& request,
                         rpcz::reply<Empty> reply) {
    LOG(INFO) << "Heartbeat";
    std::unique_lock<std::mutex> locker(lock_);

    this->leader_active_time_ = std::chrono::duration_cast< std::chrono::milliseconds >(
    std::chrono::system_clock::now().time_since_epoch()).count();

    locker.unlock();

    Empty e;
    reply.send(e);
  }

private:
  PeerStatus_Status StatusMapping();

  RaftStatus PeerStatusMapping(PeerStatus_Status status);

  void ThreadMain();

  void LaunchRaftDemon() {
    // rpcz::application application;
  // RaftService_Stub search_stub(application.create_rpc_channel(
  // "tcp://localhost:5556"), true);

  // Peer peer;
  // peer.set_id(2);
  // Null null;
  //
  // cout << "Sending request." << endl;
  // try {
  //  search_stub.Hello(peer, &null, 1000);
  //  cout << null.DebugString() << endl;
  // } catch (rpcz::rpc_error &e) {
  //  cout << "Error: " << e.what() << endl;;
  // }

    std::this_thread::sleep_for (std::chrono::seconds(1));

    rpcz::application application;
    std::vector<std::unique_ptr<RaftService_Stub>> channels;
    for (const auto& s : peers_) {
       channels.push_back(std::unique_ptr<RaftService_Stub>(
       new RaftService_Stub(application.create_rpc_channel(s), true)));
    }

    std::random_device rd;
    auto int_dist_ = std::uniform_int_distribution<int>(200, 350);
    std::mt19937 mt = std::mt19937(rd());

    while (true) {
      if (this->status_ == FOLLOWER) {
        int patience = int_dist_(mt);
        long long int cur_milli = std::chrono::duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch()).count();

        if (abs(leader_active_time_ - cur_milli) < patience) {
          std::this_thread::sleep_for (std::chrono::milliseconds(50));
        } else {
          // this->leader_active_time = cur_milli;
          std::unique_lock<std::mutex> locker(lock_);

          if (this->vote_for_ == -1) {
            this->status_ = CANDIDATE;
          }

          locker.unlock();
        }
      } else if (this->status_ == CANDIDATE) {
          LOG(INFO) << this->id_ << " becomes a candidate";
          this->term_ += 1;
          this->vote_for_ = this->id_;
          // send requests to peers

          int vote_count = 1;
          VoteRequest request;
          request.set_candidate_id(this->id_);
          request.set_last_log_index(-1);
          request.set_last_term_index(-1);
          request.set_term(this->term_);


          for (int i = 0; i < peers_.size(); i++) {
            VoteReply reply;
            if (i != this->id_) {
               channels[i]->Vote(request, &reply, 1000);
               if (reply.vote() == true) {
                 vote_count += 1;
               }
            }
          }

          if (vote_count >= (peers_.size() + 1) / 2) {
            this->status_ = LEADER;
            LOG(INFO) << this->id_ << " becomes a leader";
          } else {
            this->leader_active_time_ = std::chrono::duration_cast< std::chrono::milliseconds >(
            std::chrono::system_clock::now().time_since_epoch()).count();
            this->status_ = FOLLOWER;
            this->vote_for_ = -1;
          }
      } else { // in leader status
        std::this_thread::sleep_for (std::chrono::milliseconds(50));
      }
    }
  }

private:
  std::string rpc_addr_;
  int rpc_port_;

  std::thread rpc_thread_;
  std::thread raft_thread_;

  int id_;
  int vote_for_;
  int term_;
  RaftStatus status_;
  std::vector<std::string> peers_;

  long long int leader_active_time_;

  std::mutex lock_;
};

}


#endif //RAFT_RAFT_H
