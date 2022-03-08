cmake_minimum_required(VERSION 3.14)
project(cpu)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O2 -g")

include_directories(.)
include_directories(Common)
include_directories(BurstDetector)
include_directories(Benchmark)
include_directories(Dataset)
include_directories(PriorWork)

add_executable(cpu main.cc Common/murmur3.cc)
