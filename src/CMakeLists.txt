add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/include/success_audio.h ${CMAKE_SOURCE_DIR}/include/failure_audio.h
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/generateAudioHeaders.py
    DEPENDS ${CMAKE_SOURCE_DIR}/scripts/generateAudioHeaders.py
            ${CMAKE_SOURCE_DIR}/media/success.mp3
            ${CMAKE_SOURCE_DIR}/media/failure.mp3
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating audio header files from MP3s"
    VERBATIM
)

add_executable(baregear
        main.cpp
        lexer.cpp
        parser.cpp
        preprocessor.cpp
        transpiler.cpp
        debugger.cpp
        emulatorUtil.cpp
        ai.cpp
        cparser.cpp
        ${CMAKE_SOURCE_DIR}/include/success_audio.h
        ${CMAKE_SOURCE_DIR}/include/failure_audio.h
)

target_compile_definitions(baregear PRIVATE
        APP_VERSION="${APP_VERSION}"
)

target_compile_options(baregear PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++23>
        $<$<COMPILE_LANGUAGE:C>:-std=c11>
)

llvm_map_components_to_libnames(llvm_libs core native support option)
target_link_libraries(baregear PRIVATE
        ${llvm_libs}
        Qt5::Core
        Qt5::Network
        ${PULSE_LIBRARIES}
        ZLIB::ZLIB
        optimizedRuntime
        seccomp
        ${CAPSTONE_LIBRARIES}
        /usr/lib/llvm-18/lib/liblldb.so
        /usr/lib/llvm-18/lib/libclang-cpp.so.18.1
)

target_include_directories(baregear PRIVATE
        ${SECCOMP_INCLUDE_DIRS}
        ${PULSE_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/include
        /usr/lib/llvm-18/include
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(baregear PRIVATE -O0 -fno-eliminate-unused-debug-types)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    target_compile_options(baregear PRIVATE -O0 -fno-limit-debug-info -fstandalone-debug)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(baregear PRIVATE /Od /Oy-)
endif()

if(BUILD_TESTING)
    file(GLOB SAMPLE_FILES "${CMAKE_SOURCE_DIR}/sample/*.br")
    foreach(SAMPLE_FILE ${SAMPLE_FILES})
        get_filename_component(SAMPLE_NAME ${SAMPLE_FILE} NAME_WE)
        add_test(NAME "sample_${SAMPLE_NAME}"
            COMMAND baregear ${SAMPLE_FILE}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    endforeach()
endif()