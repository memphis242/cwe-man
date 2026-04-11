cmake_minimum_required(VERSION 3.20)
project(cwe-man VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CWE_MAN_ENABLE_WARNINGS "Enable strict compiler warnings" ON)
option(CWE_MAN_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(CWE_MAN_ENABLE_TESTS "Build unit/integration tests" ON)
option(CWE_MAN_ENABLE_COVERAGE "Enable coverage instrumentation" OFF)
option(CWE_MAN_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(CWE_MAN_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(CWE_MAN_ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(CWE_MAN_ENABLE_CLANG_TIDY "Enable clang-tidy during build" OFF)
option(CWE_MAN_ENABLE_CPPCHECK "Enable cppcheck during build" OFF)
option(CWE_MAN_ENABLE_BENCHMARKS "Build benchmark targets" OFF)
option(CWE_MAN_FETCH_TEST_DEPS "Allow downloading test-only dependencies" OFF)

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

if(CWE_MAN_ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
  else()
    message(FATAL_ERROR "CWE_MAN_ENABLE_CLANG_TIDY=ON but clang-tidy not found")
  endif()
endif()

if(CWE_MAN_ENABLE_CPPCHECK)
  find_program(CPPCHECK_EXE NAMES cppcheck)
  if(CPPCHECK_EXE)
    set(CMAKE_CXX_CPPCHECK
      "${CPPCHECK_EXE};--enable=warning,style,performance,portability;--inline-suppr")
  else()
    message(FATAL_ERROR "CWE_MAN_ENABLE_CPPCHECK=ON but cppcheck not found")
  endif()
endif()

set(CWE_MAN_SAN_FLAGS "")
if(CWE_MAN_ENABLE_ASAN)
  list(APPEND CWE_MAN_SAN_FLAGS -fsanitize=address)
endif()
if(CWE_MAN_ENABLE_UBSAN)
  list(APPEND CWE_MAN_SAN_FLAGS -fsanitize=undefined)
endif()
if(CWE_MAN_ENABLE_TSAN)
  list(APPEND CWE_MAN_SAN_FLAGS -fsanitize=thread)
endif()
if(CWE_MAN_ENABLE_TSAN AND (CWE_MAN_ENABLE_ASAN OR CWE_MAN_ENABLE_UBSAN))
  message(FATAL_ERROR "TSan cannot be combined with ASan/UBSan in one target")
endif()

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

if(CWE_MAN_ENABLE_WARNINGS)
  if(MSVC)
    target_compile_options(cwe-man PRIVATE /W4 /permissive-)
    if(CWE_MAN_WARNINGS_AS_ERRORS)
      target_compile_options(cwe-man PRIVATE /WX)
    endif()
  else()
    target_compile_options(cwe-man PRIVATE
      -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wnon-virtual-dtor
      -Wold-style-cast -Woverloaded-virtual -Wnull-dereference -Wdouble-promotion
      -Wformat=2)
    if(CWE_MAN_WARNINGS_AS_ERRORS)
      target_compile_options(cwe-man PRIVATE -Werror)
    endif()
  endif()
endif()

if(CWE_MAN_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(cwe-man PRIVATE -O0 -g --coverage)
  target_link_options(cwe-man PRIVATE --coverage)
endif()

if(CWE_MAN_SAN_FLAGS)
  target_compile_options(cwe-man PRIVATE ${CWE_MAN_SAN_FLAGS} -fno-omit-frame-pointer)
  target_link_options(cwe-man PRIVATE ${CWE_MAN_SAN_FLAGS})
endif()

if(CWE_MAN_ENABLE_TESTS)
  include(CTest)
  enable_testing()

  find_package(GTest CONFIG QUIET)
  if(NOT GTest_FOUND)
    find_package(GTest QUIET)
  endif()
  if(NOT GTest_FOUND AND CWE_MAN_FETCH_TEST_DEPS)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
  endif()
  if(GTest_FOUND OR TARGET GTest::gtest)
    add_executable(cwe-man-tests
      tests/test_main.cpp
      tests/data_database_test.cpp
      tests/data_repository_test.cpp
      tests/api_sync_test.cpp
      tests/ui_layout_event_test.cpp
      src/app/State.cpp
      src/ui/Layout.cpp
      src/ui/TreePane.cpp
      src/ui/DetailPane.cpp
      src/data/Database.cpp
      src/data/Repository.cpp
      src/logging/Logger.cpp
      src/logging/FileSink.cpp
      src/api/CweClient.cpp
      src/api/Sync.cpp
    )
    target_include_directories(cwe-man-tests PRIVATE src tests)
    target_link_libraries(cwe-man-tests PRIVATE
      SQLite::SQLite3
      CURL::libcurl
      nlohmann_json::nlohmann_json
      ftxui::screen
      ftxui::dom
      ftxui::component
      GTest::gtest
    )

    if(CWE_MAN_ENABLE_WARNINGS)
      if(MSVC)
        target_compile_options(cwe-man-tests PRIVATE /W4 /permissive-)
        if(CWE_MAN_WARNINGS_AS_ERRORS)
          target_compile_options(cwe-man-tests PRIVATE /WX)
        endif()
      else()
        target_compile_options(cwe-man-tests PRIVATE
          -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wnon-virtual-dtor
          -Wold-style-cast -Woverloaded-virtual -Wnull-dereference -Wdouble-promotion
          -Wformat=2)
        if(CWE_MAN_WARNINGS_AS_ERRORS)
          target_compile_options(cwe-man-tests PRIVATE -Werror)
        endif()
      endif()
    endif()

    if(CWE_MAN_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(cwe-man-tests PRIVATE -O0 -g --coverage)
      target_link_options(cwe-man-tests PRIVATE --coverage)
    endif()

    if(CWE_MAN_SAN_FLAGS)
      target_compile_options(cwe-man-tests PRIVATE ${CWE_MAN_SAN_FLAGS} -fno-omit-frame-pointer)
      target_link_options(cwe-man-tests PRIVATE ${CWE_MAN_SAN_FLAGS})
    endif()

    add_test(NAME cwe-man-tests COMMAND cwe-man-tests)
  else()
    message(WARNING
      "CWE_MAN_ENABLE_TESTS=ON but GTest was not found. "
      "Install GTest or configure with -DCWE_MAN_FETCH_TEST_DEPS=ON.")
  endif()
endif()

if(CWE_MAN_ENABLE_BENCHMARKS)
  add_executable(cwe-man-bench
    benchmarks/bench_repository.cpp
    src/data/Database.cpp
    src/data/Repository.cpp
    src/logging/Logger.cpp
    src/logging/FileSink.cpp
  )
  target_include_directories(cwe-man-bench PRIVATE src)
  target_link_libraries(cwe-man-bench PRIVATE
    SQLite::SQLite3
    nlohmann_json::nlohmann_json
  )
endif()

install(TARGETS cwe-man
  RUNTIME DESTINATION bin
  CONFIGURATIONS Release
)
