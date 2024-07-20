cmake_minimum_required(VERSION 3.28)
project(RPCdemo)

set(CMAKE_CXX_STANDARD 17)


include_directories(${PROJECT_SOURCE_DIR}/include)


# 添加子目录
add_subdirectory(rpc)
add_subdirectory(server)
add_subdirectory(registry)
add_subdirectory(client)

target_link_libraries(rpc ws2_32)
