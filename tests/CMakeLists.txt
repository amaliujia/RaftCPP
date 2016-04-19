include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR})

add_executable(runBasicTests
                basic_test.cpp)

target_link_libraries(runBasicTests gtest gtest_main)
target_link_libraries (runBasicTests Raft_lib)
target_link_libraries (runBasicTests glog)
target_link_libraries (runBasicTests proto ${PROTOBUF_LIBRARY})

if (APPLE)
    add_definitions(-DGTEST_USE_OWN_TR1_TUPLE)
    add_definitions(-D__GLIBCXX__)
endif (APPLE)