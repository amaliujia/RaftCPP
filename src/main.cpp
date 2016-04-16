#include <iostream>
#include <glog/logging.h>
#include <rpcz/rpcz.hpp>

#include "protofiles/address.pb.h"
#include "protofiles/search.pb.h"

#include "src/search.rpcz.h"

using namespace std;

class SearchServiceImpl : public SearchService {
    virtual void Search(
      const SearchRequest& request,
      rpcz::reply<SearchResponse> reply) {
     cout << "Got request for '" << request.query() << "'" << endl;
     SearchResponse response;
     response.add_results("result1 for " + request.query());
     response.add_results("this is result2");
     reply.send(response);
    }
};

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


 rpcz::application application;
 rpcz::server server(application);
 SearchServiceImpl search_service;
 server.register_service(&search_service);
 cout << "Serving requests on port 5555." << endl;
 server.bind("tcp://*:5555");
 application.run();

 return 0;
}