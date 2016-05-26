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

struct RWLock {
public:
  RWLock(RWLock const &) = delete;
  RWLock &operator=(RWLock const &) = delete;

  RWLock() { pthread_rwlock_init(&rw_lock, nullptr); }

  ~RWLock() { pthread_rwlock_destroy(&rw_lock); }

  void ReadLock() const { pthread_rwlock_rdlock(&rw_lock); }

  void WriteLock() const { pthread_rwlock_wrlock(&rw_lock); }

  void Unlock() const { pthread_rwlock_unlock(&rw_lock); }

private:
  // can only be moved, not copied
  mutable pthread_rwlock_t rw_lock;
};

typedef struct Raftcommand Raftcommand;

class Raft : public RaftService {

public:
  Raft(const Raft&) = delete;
  Raft &operator=(const Raft&) = delete;
  Raft(const Raft &&) = delete;
  Raft &operator=(const Raft &&) = delete;

  Raft(std::string addr, int port, int peer_id, const std::vector<std::string>& peers_addrs);

  ~Raft();

  // Kill Raft peer node.
  void Kill();

  // Get description of Raft peer node
  std::string GetInfo();

  virtual void GetStatus(const Empty& e, rpcz::reply<PeerStatus> reply);

  virtual void Op(const OpRequest& e, rpcz::reply<OpReply> reply) {
    OpReply rep;
    rep.set_success(false);

    if(this->status_ != LEADER) {
      reply.send(rep);
      return;
    }

    switch (e.op()) {
      case EntryOp::DELETE:
      case EntryOp::INSERT:
        log_lock_.WriteLock();

        log_lock_.Unlock();
        break;
      case EntryOp::QUERY:
        log_lock_.ReadLock();

        log_lock_.Unlock();
        break;
      default:
        break;
    }

    rep.set_success(true);
    reply.send(rep);
    return;
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
            auto channel_ptr = channels[this->vote_for_].release();
            channels.erase(channels.find(this->vote_for_));
            peers_.erase(this->vote_for_);
            this->vote_for_ = "";
            delete channel_ptr;
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
  // Network
  std::string rpc_addr_;
  int rpc_port_;
  rpcz::application application_;

  // Background threads
  std::thread rpc_thread_;
  std::thread raft_thread_;

  // Node status
  std::string self_id_;
  std::string vote_for_;
  uint64_t term_;
  RaftStatus status_;
  long long int leader_active_time_;
  bool is_dead_;

  // Volatile state of leader
  // Index of highest log entry known to be commited. Initialized to 0.
  uint64_t commit_index_;
  // Index of highest log entry applied to state machine. Initialized to 0.
  uint64_t last_applied_;

  // peers
  //std::vector<std::string> peers_;
  std::unordered_set<std::string> peers_;
  // used for leaders. index of next log entry to send to server.
  // Initialized to leader's last log index + 1.
  std::unordered_map<std::string, uint64_t> peers_next_index_;

  // Utils
  std::mutex lock_;
  RWLock log_lock_;

  // logs
  std::vector<LogEntry> logs_;

};
}

#endif //RAFT_RAFT_H
