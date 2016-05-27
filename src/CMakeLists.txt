include_directories(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/protofiles)

ADD_LIBRARY(Raft_lib
            common.h
            raft.cpp
            raft.h
            raft.rpcz.h
            raft.rpcz.cc
            consensus_log.cpp
            consensus_log.h
            struct.h)

set(SOURCE_FILES main.cpp
                common.h)

add_executable(Raft ${SOURCE_FILES})
target_link_libraries(Raft proto ${PROTOBUF_LIBRARY})
target_link_libraries(Raft  ${Boost_LIBRARIES})
target_link_libraries (Raft glog)
target_link_libraries (Raft Raft_lib)