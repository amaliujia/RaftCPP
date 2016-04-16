cmake_minimum_required(VERSION 3.5)
project(Raft)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

#set(glog_ROOT )
#find_package (glog REQUIRED)
find_library(GLOG_LIB glog build/lib/glog)

set(SOURCE_FILES main.cpp)
add_executable(Raft ${SOURCE_FILES})
target_link_libraries (Raft glog)

################################
# GTest
################################
ADD_SUBDIRECTORY (lib/googlemock)
enable_testing()
include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})

## Add test cpp file
#add_executable( runUnitTests testgtest.cpp )
## Link test executable against gtest & gtest_main
#target_link_libraries(runUnitTests gtest gtest_main)
#add_test( runUnitTests runUnitTests )

################################
# GTest
################################
ADD_SUBDIRECTORY (lib/glog)
