include_directories(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/protofiles)

set(SOURCE_FILES main.cpp
                raft.rpcz.cc
                raft.rpcz.h
                raft.cpp
                raft.h
                common.h)

add_executable(Raft ${SOURCE_FILES})
target_link_libraries (Raft glog)
target_link_libraries(Raft proto ${PROTOBUF_LIBRARY})
target_link_libraries(Raft  ${Boost_LIBRARIES})