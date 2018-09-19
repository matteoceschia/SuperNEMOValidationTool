cmake_minimum_required(VERSION 3.3)
project(ValidationParser)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(ROOT REQUIRED)
find_package(Boost REQUIRED filesystem system)

include_directories(${ROOT_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} include)

add_executable(ValidationParser ValidationParser.cxx ValidationParser.h)
target_link_libraries(ValidationParser ${ROOT_LIBRARIES} ${Boost_LIBRARIES})