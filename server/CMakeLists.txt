cmake_minimum_required(VERSION 3.16)
project(PersonalKnowledgeBase CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# Boost.System
find_package(Boost 1.71.0 REQUIRED COMPONENTS system)

# nlohmann_json
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# pthread
find_package(Threads REQUIRED)

# Poppler-cpp via pkg-config（用导入目标）
find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER_CPP REQUIRED IMPORTED_TARGET poppler-cpp)

add_executable(server_app
  src/main.cpp
  src/server.cpp
  src/session.cpp
  src/document_processor.cpp
)

target_include_directories(server_app PRIVATE
  include
  ${Boost_INCLUDE_DIRS}                     
)

target_link_libraries(server_app PRIVATE
  Boost::system
  Threads::Threads
  PkgConfig::POPPLER_CPP
  nlohmann_json::nlohmann_json
)
