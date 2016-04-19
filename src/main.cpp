#include <iostream>
#include <fstream>

#include "src/raft.rpcz.h"
#include "src/raft.h"
#include "common.h"

using namespace std;


string BuildRPCAddr(string addr, int port) {
 return "tcp://" + addr + ":" + to_string(port);
}

int main(int argc, char* argv[]) {
 google::InitGoogleLogging(argv[0]);
 GOOGLE_PROTOBUF_VERIFY_VERSION;
 FLAGS_logtostderr = 1;

 assert(argc == 2);
 string config_path(argv[1]);
 std::ifstream config_file(config_path);

 std::string line;
 vector<string> addrs;
 vector<int> ports;
 while (std::getline(config_file, line)) {
  std::istringstream iss(line);
  string addr;
  int port;
  if (!(iss >> addr >> port)) { break; }
  addrs.push_back(addr);
  ports.push_back(port);
 }

 vector<string> service;
 for (size_t i = 0; i < addrs.size(); i++) {
  service.push_back(BuildRPCAddr(addrs[i], ports[i]));
 }

 vector<unique_ptr<raft::Raft>> peers;
 for (size_t i = 0; i < addrs.size(); i++) {
   peers.push_back(unique_ptr<raft::Raft>(new raft::Raft(addrs[i], ports[i], i, service)));
 }

 while (true) {
  std::this_thread::sleep_for (std::chrono::seconds(300));
 }

 std::this_thread::sleep_for (std::chrono::seconds(300));
 return 0;
}