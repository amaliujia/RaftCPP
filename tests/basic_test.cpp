//
// Created by 王 瑞 on 16-4-18.
//

#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <thread>
#include <chrono>

#include <rpcz/rpcz.hpp>

#include "src/raft.h"
#include "src/raft.rpcz.h"
#include "protofiles/raft.pb.h"

class RaftTest : public ::testing::Test { };

std::string BuildRPCAddr(std::string addr, int port) {
  return "tcp://" + addr + ":" + std::to_string(port);
}

TEST_F(RaftTest, BasicTest) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  int port = 5555;
  size_t peer_num = 3;

  std::vector<std::string> service;
  for (size_t i = 0; i < peer_num; i++) {
    service.emplace_back(BuildRPCAddr("127.0.0.1", port + i));
  }

  std::vector<std::unique_ptr<raft::Raft>> peers;
  for (size_t i = 0; i < peer_num; i++) {
    peers.emplace_back(std::unique_ptr<raft::Raft>(new raft::Raft("localhost",port + i, i, service)));
  }

  rpcz::application application;
  std::vector<std::unique_ptr<RaftService_Stub>> channels;
  for (const auto& s : service) {
    channels.push_back(std::unique_ptr<RaftService_Stub>(
    new RaftService_Stub(application.create_rpc_channel(s), true)));
  }


  int leader_count = 0,
      follower_count = 0,
      candidate_count = 0,
      unknown_count = 0;

  for (int i = 0; i < peer_num; i++) {
    PeerStatus reply;
    Empty e;
    channels[i]->GetStatus(e, &reply, 1000);
    if (reply.status() == PeerStatus_Status_FOLLOWER) {
      follower_count++;
    } else if(reply.status() == PeerStatus_Status_CANDIDATE) {
      candidate_count++;
    } else if(reply.status() == PeerStatus_Status_LEADER) {
      leader_count++;
    } else {
      unknown_count++;
    }
  }

  EXPECT_EQ(follower_count, 2);
  EXPECT_EQ(leader_count, 1);
  EXPECT_EQ(candidate_count, 0);
  EXPECT_EQ(unknown_count, 0);

}