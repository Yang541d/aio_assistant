cmake_minimum_required(VERSION 3.16)
project(AioServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# SQLiteCpp
FetchContent_Declare(
    SQLiteCpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG        3.3.1
)
FetchContent_MakeAvailable(SQLiteCpp)

# nlohmann_json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# cpr
FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG        1.10.5 # 使用一个稳定版本
)
FetchContent_MakeAvailable(cpr)
# Boost
find_package(Boost 1.71.0 REQUIRED COMPONENTS system)
if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost not found")
endif()

# Executable
add_executable(server_app 
    src/main.cpp 
    src/server.cpp 
    src/session.cpp
    src/database.cpp 
)

# Include dirs
target_include_directories(server_app 
    PRIVATE 
    ${PROJECT_SOURCE_DIR}/include
    ${nlohmann_json_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(server_app 
    PRIVATE 
    Boost::system 
    SQLiteCpp
    cpr::cpr      
    CURL::libcurl 
)
