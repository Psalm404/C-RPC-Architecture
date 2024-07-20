set(SERVER_SOURCES
        ${PROJECT_SOURCE_DIR}/registry/registry.cpp
        ${PROJECT_SOURCE_DIR}/include
)



add_executable(registry ${SERVER_SOURCES})


add_library(dispatcherservlet INTERFACE)

target_include_directories(registry PUBLIC ${PROJECT_SOURCE_DIR}/rpc)
target_include_directories(dispatcherservlet INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(dispatcherservlet INTERFACE ws2_32)
target_link_libraries(registry PUBLIC ws2_32)