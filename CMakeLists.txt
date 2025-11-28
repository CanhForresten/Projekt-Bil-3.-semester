cmake_minimum_required(VERSION 3.10)

project(AGV_Project)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/obj)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/obj)

include_directories(include)

file(GLOB SRC_FILES src/*.cpp)

add_executable(agv ${SRC_FILES})

# Link mosquitto (and optionally mosquittopp)
target_link_libraries(agv PRIVATE mosquitto)
# target_link_libraries(agv PRIVATE mosquittopp)  # Uncomment if needed

