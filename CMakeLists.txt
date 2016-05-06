cmake_minimum_required(VERSION 3.5)
project(Raft)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lboost_system -lrpcz")

find_package(BOOST COMPONENTS program_options REQUIRED)

find_library(GLOG_LIB glog build/lib/glog)
find_library(PROTO_LIB proto build/lib/protofiles)

set_source_files_properties(${PROTO_SRC} ${PROTO_HEADER} PROPERTIES
        GENERATED TRUE)

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

################################
# Tests
################################
add_subdirectory(tests)

################################
# SRC
################################
add_subdirectory(src)

################################
# Proto
################################
add_subdirectory(protofiles)

################################
# GTest
################################
ADD_SUBDIRECTORY (lib/googlemock)
#enable_testing()
include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})

## Add test cpp file
#add_executable( runUnitTests testgtest.cpp )
## Link test executable against gtest & gtest_main
#target_link_libraries(runUnitTests gtest gtest_main)
#add_test( runUnitTests runUnitTests )

################################
# Glog
################################
ADD_SUBDIRECTORY (lib/glog)


