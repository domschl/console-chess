cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# Build type needs to be set as parameter to CMAKE: -DCMAKE_BUILD_TYPE=xxxx
# set(CMAKE_BUILD_TYPE Release)
# If the user specifies -DCMAKE_BUILD_TYPE on the command line, take their definition
# and dump it in the cache along with proper documentation, otherwise set CMAKE_BUILD_TYPE
# to Debug prior to calling PROJECT()
IF(DEFINED CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of `build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE()
   SET(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()

set(CMAKE_CXX_FLAGS_DEBUG  "-g")
set(CMAKE_CXX_FLAGS_DEBUG  "-ggdb")

project(chess)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(gcc_like_compilers GNU Clang AppleClang Intel)
set(intel_archs x86_64 i386 i686)

# Setup some options to allow a user to enable SSE and AVX instruction use.
if ((";${gcc_like_compilers};" MATCHES ";${CMAKE_CXX_COMPILER_ID};") AND
    (";${intel_archs};"        MATCHES ";${CMAKE_SYSTEM_PROCESSOR};"))
    option(USE_SSE2_INSTRUCTIONS "Compile your program with SSE2 instructions" ON)
    option(USE_SSE4_INSTRUCTIONS "Compile your program with SSE4 instructions" ON)
    option(USE_AVX_INSTRUCTIONS  "Compile your program with AVX & AVX2 instructions"  ON)
    option(USE_FMA_INSTRUCTIONS  "Compile your program with FMA instructions"  ON)
    if(USE_AVX_INSTRUCTIONS)
        add_definitions(-mavx -mavx2)
        add_definitions(-DUSE_AVX)
        message(STATUS "Enabling AVX & AVX2 instructions")
    elseif (USE_SSE4_INSTRUCTIONS)
        add_definitions(-msse4)
        add_definitions(-DUSE_SSE4)
        message(STATUS "Enabling SSE4 instructions")
    elseif(USE_SSE2_INSTRUCTIONS)
        add_definitions(-msse2)
        add_definitions(-DUSE_SSE2)
        message(STATUS "Enabling SSE2 instructions")
    endif()
    if(USE_FMA_INSTRUCTIONS)
        add_definitions(-mfma)
        add_definitions(-DUSE_FMA)
        message(STATUS "Enabling FMA (fused multiple add, Haswell onwards) instructions")
    endif()
endif()


if (WITH_OPENMP)
    find_package(OpenMP)
    if (OPENMP_FOUND)
        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        add_definitions(-DUSE_OPENMP)
        message(STATUS "Enabling OpenMP")
    endif()
endif()

add_subdirectory (chess)
include_directories("chess")

set_property(TARGET chess PROPERTY CXX_STANDARD 11)
