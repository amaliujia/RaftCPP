#include <iostream>
#include <fstream>

#include "src/raft.rpcz.h"
#include "src/raft.h"
#include "common.h"

using namespace std;


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

//  raft::Raft raft_peer(addrs[0], ports[0]);
 vector<unique_ptr<raft::Raft>> peers;
 for (size_t i = 0; i < addrs.size(); i++) {
   peers.push_back(unique_ptr<raft::Raft>(new raft::Raft(addrs[i], ports[i])));
 }


 std::this_thread::sleep_for (std::chrono::seconds(30));
 return 0;
}