cmake_minimum_required(VERSION 3.16)
project(AioServer)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# find boost
find_package(Boost 1.71.0 REQUIRED COMPONENTS system)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()


add_executable(server_app 
    src/main.cpp 
    src/server.cpp 
    src/session.cpp
)

target_include_directories(server_app PRIVATE include)

if(Boost_FOUND)
    target_link_libraries(server_app PRIVATE Boost::system)
endif()


