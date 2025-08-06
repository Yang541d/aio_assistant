cmake_minimum_required(VERSION 3.16)
project(AioServer)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


include(FetchContent)
FetchContent_Declare(
    SQLiteCpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG        3.3.1 
)
FetchContent_MakeAvailable(SQLiteCpp)

# find boost
find_package(Boost 1.71.0 REQUIRED COMPONENTS system)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()


add_executable(server_app 
    src/main.cpp 
    src/server.cpp 
    src/session.cpp
    src/database.cpp 
)

target_include_directories(server_app PRIVATE include)

target_link_libraries(server_app 
    PRIVATE 
    Boost::system 
    SQLiteCpp
)


