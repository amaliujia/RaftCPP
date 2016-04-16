#include <iostream>
#include <glog/logging.h>

#include "protofiles/address.pb.h"

using namespace std;

int main(int argc, char* argv[]) {
 google::InitGoogleLogging(argv[0]);
 GOOGLE_PROTOBUF_VERIFY_VERSION;

 FLAGS_logtostderr = 1;
 tutorial::Person person;
 person.set_id(1);
 person.set_name("Linda");
 string code;
 person.SerializeToString(&code);

 tutorial::Person anther_person;
 anther_person.ParseFromString(code);

 LOG(INFO)  << "id: " << anther_person.id() << "  name " << anther_person.name();

 return 0;
}