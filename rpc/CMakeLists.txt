
set(RPC_SOURCES
        ${PROJECT_SOURCE_DIR}/rpc/rpc.cpp
        ${PROJECT_SOURCE_DIR}/include
)

# 创建库
add_library(rpc STATIC ${RPC_SOURCES})
add_library(httpserver INTERFACE)
add_library(localregister INTERFACE)
add_library(requesthandler INTERFACE)
add_library(rpcclient INTERFACE)
add_library(rpcserver INTERFACE)
# 添加头文件目录
target_include_directories(rpc PUBLIC ${PROJECT_SOURCE_DIR}/rpc)
target_include_directories(requesthandler INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(localregister INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(requesthandler INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(rpcserver INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(requesthandler INTERFACE ws2_32)
target_link_libraries(rpcclient INTERFACE ws2_32)