cmake_minimum_required(VERSION 3.28)

if(EXISTS "${CMAKE_SOURCE_DIR}/vcpkg.json" AND NOT CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    endif()
endif()

project(Baregear)
include(CTest)
include(ExternalProject)
set(APP_VERSION 1.0)

set(INSSET_BUILD_DIR "${CMAKE_BINARY_DIR}/_deps/libinsset-build")
set(INSSET_LIBRARY "${INSSET_BUILD_DIR}/src/libinsset.so")

# Parent project owns workspace orchestration; child project remains independent.
# When a parent build should drive a child project, use ExternalProject_Add() and
# build the child target directly instead of requiring an install step.
ExternalProject_Add(baregear_insset
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/lib/insset
    BINARY_DIR ${INSSET_BUILD_DIR}
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    CMAKE_ARGS
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_CXX_STANDARD=23
        -DCMAKE_CXX_STANDARD_REQUIRED=ON
        -DCMAKE_PREFIX_PATH=/usr/lib/llvm-18
        -DLLVM_DIR=/usr/lib/llvm-18/lib/cmake/llvm
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target insset
    BUILD_BYPRODUCTS ${INSSET_LIBRARY}
    INSTALL_COMMAND ""
    BUILD_ALWAYS OFF
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(include)
include_directories("${CMAKE_SOURCE_DIR}/lib/insset/include")

find_path(LLDB_INCLUDE_DIR lldb/API/SBDebugger.h
    HINTS ${LLVM_INCLUDE_DIRS} /usr/lib/llvm-18/include)
find_path(CLANG_INCLUDE_DIR clang/AST/ASTContext.h
    HINTS ${LLVM_INCLUDE_DIRS} /usr/lib/llvm-18/include)
if(NOT LLDB_INCLUDE_DIR OR NOT CLANG_INCLUDE_DIR)
    message(FATAL_ERROR
        "Missing LLVM development headers. Install liblldb-18-dev and libclang-18-dev.")
endif()
include_directories(${LLDB_INCLUDE_DIR} ${CLANG_INCLUDE_DIR})

find_package(ZLIB REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Network)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PULSE REQUIRED libpulse)
pkg_check_modules(SECCOMP REQUIRED libseccomp)
pkg_check_modules(CAPSTONE REQUIRED capstone)
include_directories(${CAPSTONE_INCLUDE_DIRS})

# Documentation target
find_program(SPHINX_EXECUTABLE sphinx-build)
if(SPHINX_EXECUTABLE)
    add_custom_target(docs
        COMMAND ${SPHINX_EXECUTABLE} -b html ${CMAKE_SOURCE_DIR}/docs ${CMAKE_BINARY_DIR}/docs
        COMMENT "Building documentation with Sphinx"
        VERBATIM
    )
else()
    message(WARNING "Sphinx not found. Documentation target will not be available.")
endif()

add_subdirectory(src)
