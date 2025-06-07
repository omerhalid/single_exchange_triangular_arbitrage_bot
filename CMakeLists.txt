cmake_minimum_required(VERSION 3.22)
project(TriArbBot LANGUAGES CXX)

enable_testing()
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

# ---------- Find Boost (header-only via vcpkg) ----------
# Do NOT specify asio/beast as components—just request Boost’s include dir.
find_package(Boost REQUIRED)

if (NOT Boost_FOUND)
  message(FATAL_ERROR "Boost not found; please ensure vcpkg toolchain is passed to CMake.")
endif()

message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")

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

# ---------- Find OpenSSL ----------
if(WIN32)
    # Help CMake find OpenSSL installed by vcpkg
    if(DEFINED ENV{VCPKG_ROOT})
        set(OPENSSL_ROOT_DIR "$ENV{VCPKG_ROOT}/installed/x64-windows")
    else()
        set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg/installed/x64-windows")
    endif()
endif()

# ---------- Find dependencies ----------
find_package(OpenSSL REQUIRED)
message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")
message(STATUS "OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")

find_package(nlohmann_json CONFIG REQUIRED)

# ---------- Executable (TriArbBot) ----------
add_executable(triarb
    src/main.cpp
    src/websocket_session.cpp
    src/orderbook.cpp
    src/triarb_bot.cpp
    src/gateway.cpp  
)

# Include the headers and Boost's include directory
target_include_directories(triarb PRIVATE 
    include 
    ${Boost_INCLUDE_DIRS}
)

# Link against OpenSSL and JSON library
target_link_libraries(triarb PRIVATE 
    OpenSSL::SSL 
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)

# ---------- Unit tests ----------
add_executable(tests
  test/arbitrage_test.cpp
  test/orderbook_test.cpp
  src/orderbook.cpp
)

target_include_directories(tests PRIVATE 
  include
)

target_link_libraries(tests PRIVATE 
  Catch2::Catch2WithMain
  nlohmann_json::nlohmann_json 
)

add_test(NAME all_tests COMMAND tests)

if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()
