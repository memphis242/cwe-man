cmake_minimum_required(VERSION 3.20)
project(cwe-man VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# System dependencies
find_package(SQLite3 REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

# ftxui via FetchContent (not packaged on Fedora)
include(FetchContent)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG        v6.1.9
)
FetchContent_MakeAvailable(ftxui)

add_executable(cwe-man
  src/main.cpp
  src/app/App.cpp
  src/app/State.cpp
  src/ui/Layout.cpp
  src/ui/TreePane.cpp
  src/ui/DetailPane.cpp
  src/api/CweClient.cpp
  src/api/Sync.cpp
  src/data/Database.cpp
  src/data/Repository.cpp
  src/logging/Logger.cpp
  src/logging/FileSink.cpp
)

target_include_directories(cwe-man PRIVATE src)

target_link_libraries(cwe-man PRIVATE
  SQLite::SQLite3
  CURL::libcurl
  nlohmann_json::nlohmann_json
  ftxui::screen
  ftxui::dom
  ftxui::component
)
