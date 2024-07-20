cmake_minimum_required(VERSION 3.28)

# 设置源文件
set(CLIENT_SOURCES ${PROJECT_SOURCE_DIR}/client/client.cpp)
add_executable(client ${CLIENT_SOURCES})

# 包含头文件路径
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/lib)
include_directories(${PROJECT_SOURCE_DIR}/rpc)


# 链接库文件
target_link_libraries(client PRIVATE ws2_32)

