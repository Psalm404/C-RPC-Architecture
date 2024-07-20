set(SERVER_SOURCES
        ${PROJECT_SOURCE_DIR}/server/server.cpp
)


# 创建可执行文件
add_executable(server ${SERVER_SOURCES})

target_include_directories(server PRIVATE ${PROJECT_SOURCE_DIR}/rpc)
target_include_directories(server PRIVATE ${PROJECT_SOURCE_DIR}/registry)

# 链接 httpserver 库
target_link_libraries(server PRIVATE httpserver)
target_link_libraries(server PRIVATE localregister)

# 链接所需的库
target_link_libraries(server PRIVATE ws2_32)
