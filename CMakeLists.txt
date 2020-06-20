cmake_minimum_required(VERSION 3.13)

project(hmq VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

aux_source_directory(. DIR_SRCS)

add_subdirectory(lib)

include_directories(
	${PROJECT_SOURCE_DIR}
	#	${PROJECT_SOURCE_DIR}/util
)


include_directories(${GLOG_INCLUDE_DIR})
find_package(glog REQUIRED)

add_executable(hmq src/hmq.cpp)
target_link_libraries(hmq glog::glog transdi)
