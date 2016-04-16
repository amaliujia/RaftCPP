#include <iostream>
#include <glog/logging.h>

using namespace std;

int main(int argc, char* argv[]) {
 google::InitGoogleLogging(argv[0]);
 FLAGS_logtostderr = 1;
 LOG(INFO) << "Hello world!";
 return 0;
}