cmake_minimum_required(VERSION 3.16)
project(AioServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 引入第三方依赖 ---
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

# cpr (C++ Requests)
FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG        1.10.5
)
FetchContent_MakeAvailable(cpr)

# --- 寻找系统库 ---
# 寻找Boost system组件
find_package(Boost 1.71.0 REQUIRED COMPONENTS system)
find_package(CURL REQUIRED)

# --- 定义 server_app 目标 ---
add_executable(server_app 
    src/main.cpp 
    src/server.cpp 
    src/session.cpp
    src/database.cpp 
)

# 为 server_app 配置包含目录
target_include_directories(server_app 
    PRIVATE 
    include # 使用简洁的相对路径
    ${nlohmann_json_SOURCE_DIR}/include
)

# 为 server_app 链接库
target_link_libraries(server_app 
    PRIVATE 
    Boost::system 
    SQLiteCpp
    cpr::cpr      
    CURL::libcurl 
)

# --- 定义 http_hello_server 目标 ---
add_executable(http_hello_server src/http_server_main.cpp)

# 为 http_hello_server 配置包含目录
target_include_directories(http_hello_server
    PRIVATE 
    include
    ${Boost_INCLUDE_DIRS}
)

# 为 http_hello_server 链接库
target_link_libraries(http_hello_server 
    PRIVATE 
    Boost::system
)