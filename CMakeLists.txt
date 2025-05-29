cmake_minimum_required(VERSION 3.22)
project(TriArbBot LANGUAGES CXX)

include(CTest)     

# ---------- Compiler flags ----------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
  add_compile_options(/O2 /permissive-)
else()
  add_compile_options(-O3 -march=native -Wall -Wextra -Wpedantic -flto)
endif()

# ---------- Dependencies via CPM ----------
include(FetchContent)
FetchContent_Declare(
  CPM
  GIT_REPOSITORY https://github.com/cpm-cmake/CPM.cmake
  GIT_TAG origin/master
)
FetchContent_MakeAvailable(CPM)

# Catch2 for tests
CPMAddPackage(
  NAME Catch2
  GITHUB_REPOSITORY catchorg/Catch2
  GIT_TAG v3.5.4
  OPTIONS "CATCH_INSTALL_EXTRAS YES"
)

# ---------- Executable ----------
add_executable(triarb src/main.cpp)
target_include_directories(triarb PRIVATE include)
# target_link_libraries(triarb PRIVATE Boost::headers)

# ---------- Unit tests ----------
add_executable(tests
  test/arbitrage_test.cpp
)
target_include_directories(tests PRIVATE include)
# target_link_libraries(tests PRIVATE Catch2::Catch2WithMain Boost::headers)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
add_test(NAME all_tests COMMAND tests)
