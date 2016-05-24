//
// Created by 王 瑞 on 16-4-17.
//

#ifndef RAFT_RAFT_H
#define RAFT_RAFT_H

#include "build/protofiles/raft.pb.h"
// #include "raft.pb.h"
#include "raft.rpcz.h"
#include "common.h"

namespace raft {

enum RaftStatus {
  UNKNOWN,
  DEAD,
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
  Raft(const Raft&) = delete;
  Raft &operator=(const Raft&) = delete;
  Raft(const Raft &&) = delete;
  Raft &operator=(const Raft &&) = delete;

  Raft(std::string addr, int port, int peer_id,
       const std::vector<std::string>& peers_addrs)
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

  ~Raft() {
    LOG(INFO) << "Destroy raft " << this->rpc_addr_ << ":" << this->rpc_port_;
    this->Kill();
  }

  void Kill() {
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
    } else if (this->vote_for_ != ""){
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

  virtual void AppendMsg(const AppendRequest& request,
                         rpcz::reply<AppendReply> reply) {
    std::unique_lock<std::mutex> locker(lock_);

    AppendReply e;
    e.set_term(this->term_);
    e.set_success(false);

    this->leader_active_time_ = std::chrono::duration_cast< std::chrono::milliseconds >(
    std::chrono::system_clock::now().time_since_epoch()).count();

    int term = request.term();
    std::string leader_id = request.candidate_id();

    if (this->term_ <= term) {
      this->term_ = term;

      if (this->status_ == CANDIDATE || this->status_ == LEADER) {
        this->status_ = FOLLOWER;
        this->vote_for_ = leader_id;
      }

      e.set_success(true);

      // should do replication, aka check log
    } else  {
      // should reject stale leader's request.
      e.set_success(false);
    }

    locker.unlock();

    reply.send(e);
  }

private:
  PeerStatus_Status StatusMapping();

  RaftStatus PeerStatusMapping(PeerStatus_Status status);

  void ThreadMain();

  void LaunchRaftDemon() {
    std::this_thread::sleep_for (std::chrono::seconds(5));

    rpcz::application application;
    std::unordered_map<std::string, std::unique_ptr<RaftService_Stub>> channels;
    for (const auto& s : peers_) {
      if (s == this->self_id_) {
        continue;
      }
      channels[s] = std::unique_ptr<RaftService_Stub>(
      new RaftService_Stub(application.create_rpc_channel(s), true));
    }

    std::random_device rd;
    auto int_dist_ = std::uniform_int_distribution<int>(200, 350);
    std::mt19937 mt = std::mt19937(rd());

    while (!this->is_dead_) {
      std::unique_lock<std::mutex> locker(lock_);

      if (this->status_ == FOLLOWER) {
        int patience = int_dist_(mt) * 25;
        long long int cur_milli = std::chrono::duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch()).count();

        if (abs(leader_active_time_ - cur_milli) < patience) {
          std::this_thread::sleep_for (std::chrono::milliseconds(50));
        } else {
          // this->leader_active_time = cur_milli;
          // std::unique_lock<std::mutex> locker(lock_);

          if (this->vote_for_ == "") {
            this->status_ = CANDIDATE;
          } else {
            channels.erase(channels.find(this->vote_for_));
            peers_.erase(this->vote_for_);
            this->vote_for_ = "";
          }
          // locker.unlock();
        }
      } else if (this->status_ == CANDIDATE) {
          LOG(INFO) << this->self_id_ << " becomes a candidate";
          this->term_ += 1;
          this->vote_for_ = this->self_id_;
          // send requests to peers

          int vote_count = 1;
          VoteRequest request;
          request.set_candidate_id(this->self_id_);
          request.set_last_log_index(-1);
          request.set_last_term_index(-1);
          request.set_term(this->term_);


          for (auto peer : this->peers_) {
            VoteReply reply;
            reply.set_vote(false);
            if (peer != this->self_id_) {
                try {
                  channels[peer]->Vote(request, &reply, 1000);
                } catch (rpcz::rpc_error &e) {
                  LOG(INFO) << "Error: " << e.what() << std::endl;;
                }
               if (reply.vote() == true) {
                 vote_count += 1;
               }
            }
          }

          if (vote_count >= (peers_.size()/ 2+1)) {
            this->status_ = LEADER;
            LOG(INFO) << this->self_id_ << " becomes a leader";
          } else {
            this->leader_active_time_ = std::chrono::duration_cast< std::chrono::milliseconds >(
            std::chrono::system_clock::now().time_since_epoch()).count();
            this->status_ = FOLLOWER;
            this->vote_for_ = "";
          }
      } else { // in leader status
        std::this_thread::sleep_for (std::chrono::milliseconds(50));
        for (auto peer : this->peers_) {
          AppendRequest request;
          AppendReply reply;

          request.set_candidate_id(this->self_id_);
          request.set_term(this->term_);

          if (peer != this->self_id_) {
            try {
              channels[peer]->AppendMsg(request, &reply, 1000);
              if (reply.success() == false) {
                this->status_ = FOLLOWER;
                this->term_ = reply.term();
                this->leader_active_time_ = std::chrono::duration_cast< std::chrono::milliseconds >(
                std::chrono::system_clock::now().time_since_epoch()).count();
                break;
              }
            } catch (rpcz::rpc_error &e) {
             LOG(INFO) << "Error: " << e.what() << std::endl;;
            }
          }
        }
      }
      locker.unlock();
    }
  }

private:
  std::string rpc_addr_;
  int rpc_port_;

  std::thread rpc_thread_;
  std::thread raft_thread_;

  std::string self_id_;
  std::string vote_for_;
  int term_;
  RaftStatus status_;

  //std::vector<std::string> peers_;
  std::unordered_set<std::string> peers_;

  long long int leader_active_time_;

  std::mutex lock_;

  bool is_dead_;

  rpcz::application application_;
};
}

#endif //RAFT_RAFT_H
