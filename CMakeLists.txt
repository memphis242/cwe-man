cmake_minimum_required(VERSION 3.20)
project(cwe-man VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CWE_MAN_ENABLE_TESTS "Build unit tests" OFF)
option(CWE_MAN_ENABLE_COVERAGE "Enable coverage instrumentation" OFF)
option(CWE_MAN_FETCH_DEPS "Allow fetching dependencies via FetchContent" ON)
option(CWE_MAN_FETCH_TEST_DEPS "Allow fetching test dependencies" OFF)

# System dependencies
find_package(SQLite3 REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

# FTXUI: prefer system package, fallback to FetchContent.
find_package(ftxui CONFIG QUIET)
if(NOT ftxui_FOUND)
  if(NOT CWE_MAN_FETCH_DEPS)
    message(FATAL_ERROR
      "FTXUI not found via find_package(ftxui). "
      "Install FTXUI or set -DCWE_MAN_FETCH_DEPS=ON.")
  endif()
  include(FetchContent)
  FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
    GIT_TAG        v6.1.9
  )
  FetchContent_MakeAvailable(ftxui)
endif()

function(cwe_man_apply_warnings target_name)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${target_name} PRIVATE
      # COMMON_WARNS (C++-applicable subset)
      -Wall -Wextra -Wpedantic
      -Wuninitialized -Wshadow
      -Wdouble-promotion
      -Wformat=2 -Wformat-signedness -Wformat-truncation
      -Wundef
      -fno-common
      -Wconversion
      -Wno-sign-conversion
      -Wunused-parameter
      -Wunused-result
      -Wswitch-default -Wswitch-enum
      -Wnull-dereference
      -Wmissing-declarations
      -Wfloat-equal
      -Wwrite-strings
      -Wcast-qual
      -Wredundant-decls -Wparentheses
      -Wmissing-include-dirs
      -Winit-self -Wstrict-aliasing
      -Wpointer-arith -Wreturn-type
      -Wstack-protector
      -Walloca
      -Wdeprecated-declarations
      # Additional broadly useful C++ warnings.
      -Wold-style-cast
      -Wnon-virtual-dtor
      -Woverloaded-virtual
    )
  endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${target_name} PRIVATE
      # GCC_EXTRA_WARNS
      -Wmaybe-uninitialized
      -Wlogical-op
      -Wcast-align=strict
      -Wformat-overflow=2 -Wformat-truncation=2
      -Wduplicated-cond
      -Wduplicated-branches
      -Wrestrict
      -Wstringop-overflow=4
      -Warith-conversion
      -Warray-bounds=2
      -Wuse-after-free=2
      -Wimplicit-fallthrough=4
      -Walloc-zero -Walloc-size
      -Wuseless-cast -Wtrampolines
      -fdiagnostics-color
    )
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${target_name} PRIVATE
      # CLANG_EXTRA_WARNS
      -Wformat-overflow
      -Wassign-enum
      -Wcast-align
      -Wconditional-uninitialized
      -Widiomatic-parentheses
      -Wunreachable-code-aggressive
      -Wcomma
      -Wimplicit-fallthrough
      -Wover-aligned
    )
  endif()

  # Always warnings; treat as errors for Release only.
  target_compile_options(${target_name} PRIVATE
    $<$<CONFIG:Release>:-Werror>
  )
endfunction()

function(cwe_man_apply_coverage target_name)
  if(CWE_MAN_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${target_name} PRIVATE -Og -g3 --coverage)
    target_link_options(${target_name} PRIVATE --coverage)
  endif()
endfunction()

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
cwe_man_apply_warnings(cwe-man)
cwe_man_apply_coverage(cwe-man)

if(CWE_MAN_ENABLE_TESTS)
  include(CTest)
  enable_testing()

  find_package(GTest CONFIG QUIET)
  if(NOT GTest_FOUND)
    find_package(GTest QUIET)
  endif()
  if(NOT GTest_FOUND AND CWE_MAN_FETCH_TEST_DEPS)
    include(FetchContent)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
  endif()

  if(NOT GTest_FOUND AND NOT TARGET GTest::gtest)
    message(FATAL_ERROR
      "CWE_MAN_ENABLE_TESTS=ON but GTest was not found. "
      "Install GTest or set -DCWE_MAN_FETCH_TEST_DEPS=ON.")
  endif()

  add_executable(cwe-man-test-database
    tests/test_main.cpp
    tests/data_database_test.cpp
    tests/logging_filesink_test.cpp
    src/data/Database.cpp
    src/logging/Logger.cpp
    src/logging/FileSink.cpp
  )
  target_include_directories(cwe-man-test-database PRIVATE src)
  target_link_libraries(cwe-man-test-database PRIVATE
    SQLite::SQLite3
    GTest::gtest
  )
  cwe_man_apply_warnings(cwe-man-test-database)
  cwe_man_apply_coverage(cwe-man-test-database)

  add_test(NAME cwe-man-test-database COMMAND cwe-man-test-database)

  add_executable(cwe-man-test-logger
    tests/test_main.cpp
    tests/logger_test.cpp
    src/logging/Logger.cpp
    src/logging/FileSink.cpp
  )
  target_include_directories(cwe-man-test-logger PRIVATE src)
  target_link_libraries(cwe-man-test-logger PRIVATE GTest::gtest)
  cwe_man_apply_warnings(cwe-man-test-logger)
  cwe_man_apply_coverage(cwe-man-test-logger)
  add_test(NAME cwe-man-test-logger COMMAND cwe-man-test-logger)

  add_executable(cwe-man-test-logger-nohome
    tests/test_main.cpp
    tests/logger_no_home_test.cpp
    src/logging/Logger.cpp
    src/logging/FileSink.cpp
  )
  target_include_directories(cwe-man-test-logger-nohome PRIVATE src)
  target_link_libraries(cwe-man-test-logger-nohome PRIVATE GTest::gtest)
  cwe_man_apply_warnings(cwe-man-test-logger-nohome)
  cwe_man_apply_coverage(cwe-man-test-logger-nohome)
  add_test(NAME cwe-man-test-logger-nohome COMMAND cwe-man-test-logger-nohome)
endif()

install(TARGETS cwe-man
  RUNTIME DESTINATION bin
  CONFIGURATIONS Release
)
