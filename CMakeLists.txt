cmake_minimum_required(VERSION 3.28)

#[[
  Agent: Github Copilot
  LLM: GPT 5.6 Luna
]]
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND EXISTS "${CMAKE_SOURCE_DIR}/vcpkg.json" AND DEFINED ENV{VCPKG_ROOT})
    set(_libuuid_archive "$ENV{VCPKG_ROOT}/downloads/libuuid-1.0.3.tar.gz")
    if(NOT EXISTS "${_libuuid_archive}")
        file(MAKE_DIRECTORY "$ENV{VCPKG_ROOT}/downloads")
        find_program(CURL_EXECUTABLE curl REQUIRED)
        execute_process(
            COMMAND "${CURL_EXECUTABLE}" -fL --retry 3 --retry-all-errors --max-time 120
                "https://downloads.sourceforge.net/project/libuuid/libuuid-1.0.3.tar.gz"
                -o "${_libuuid_archive}"
            RESULT_VARIABLE _libuuid_download_result
        )
        if(NOT "${_libuuid_download_result}" STREQUAL "0")
            file(REMOVE "${_libuuid_archive}")
            message(FATAL_ERROR "Failed to download the libuuid source archive.")
        endif()

        execute_process(
            COMMAND sha512sum "${_libuuid_archive}"
            OUTPUT_VARIABLE _libuuid_sha512_output
            RESULT_VARIABLE _libuuid_sha512_result
        )
        string(REGEX MATCH "^[0-9a-fA-F]+" _libuuid_sha512 "${_libuuid_sha512_output}")
        if(NOT "${_libuuid_sha512_result}" STREQUAL "0" OR NOT _libuuid_sha512 STREQUAL "77488caccc66503f6f2ded7bdfc4d3bc2c20b24a8dc95b2051633c695e99ec27876ffbafe38269b939826e1fdb06eea328f07b796c9e0aaca12331a787175507")
            file(REMOVE "${_libuuid_archive}")
            message(FATAL_ERROR "The downloaded libuuid archive failed its SHA-512 check.")
        endif()
    endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/vcpkg.json" AND NOT CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
        if (WIN32 || WIN64)
            execute_process(
                COMMAND "${VCPKG_ROOT}/vcpkg.exe" contact
                RESULT_VARIABLE vcpkg_result
                OUTPUT_VARIABLE vcpkg_output
                ERROR_VARIABLE vcpkg_error
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
        else()
            execute_process(
                COMMAND "${VCPKG_ROOT}/vcpkg" contact
                RESULT_VARIABLE vcpkg_result
                OUTPUT_VARIABLE vcpkg_output
                ERROR_VARIABLE vcpkg_error
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
        endif()
    endif()
endif()

project(Baregear)

# Keep manifest installation explicit so `cmake -B build` also installs the
# dependencies declared by vcpkg.json before package discovery below.
if(EXISTS "${CMAKE_SOURCE_DIR}/vcpkg.json")
    set(_vcpkg_root "$ENV{VCPKG_ROOT}")
    if(NOT _vcpkg_root AND CMAKE_TOOLCHAIN_FILE MATCHES "/scripts/buildsystems/vcpkg\\.cmake$")
        get_filename_component(_vcpkg_root "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)
        get_filename_component(_vcpkg_root "${_vcpkg_root}" DIRECTORY)
        get_filename_component(_vcpkg_root "${_vcpkg_root}" DIRECTORY)
    endif()

    if(WIN32)
        set(_vcpkg_executable "${_vcpkg_root}/vcpkg.exe")
    else()
        set(_vcpkg_executable "${_vcpkg_root}/vcpkg")
    endif()

    if(NOT EXISTS "${_vcpkg_executable}")
        message(FATAL_ERROR
            "vcpkg.json exists, but vcpkg was not found. Set VCPKG_ROOT or "
            "configure with -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>/scripts/buildsystems/vcpkg.cmake.")
    endif()

    message(STATUS "Installing vcpkg manifest dependencies")
    execute_process(
        COMMAND "${_vcpkg_executable}" install
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _vcpkg_install_result
    )
    if(NOT "${_vcpkg_install_result}" STREQUAL "0")
        message(FATAL_ERROR "vcpkg install failed (${_vcpkg_install_result}).")
    endif()
endif()
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
include_directories("lib\\insset\\include")

find_package(ZLIB REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Network)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PULSE REQUIRED libpulse)
pkg_check_modules(UNICORN REQUIRED IMPORTED_TARGET unicorn)
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

# add_subdirectory(lib)
add_subdirectory(src)
