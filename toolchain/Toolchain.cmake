# Toolchain file used to do verification build tests. Adopted from BOTW

if (DEFINED ENV{NNSDK_DIR})
    message(STATUS "Using NNSDK_DIR: $ENV{NNSDK_DIR}")
    set(NNSDK_DIR "$ENV{NNSDK_DIR}")
else()
    set(NNSDK_DIR "${CMAKE_CURRENT_LIST_DIR}/../lib/nnsdk")
endif()
if (DEFINED ENV{NX_CLANG})
    message(STATUS "Using NX_CLANG: $ENV{NX_CLANG}")
    set(NX_CLANG "$ENV{NX_CLANG}")
else()
    if (NOT DEFINED ENV{NX_AARCH64})
        message(FATAL_ERROR "please define NX_CLANG or NX_AARCH64")
    endif()
    message(STATUS "Using NX_AARCH64: $ENV{NX_AARCH64}")
    set(NX_CLANG "${CMAKE_CURRENT_LIST_DIR}/nx-aarch64-$ENV{NX_AARCH64}")
endif()
if (DEFINED ENV{NX_MUSL})
    message(STATUS "Using NX_MUSL: $ENV{NX_MUSL}")
    set(NX_MUSL "$ENV{NX_MUSL}")
else()
    set(NX_MUSL "${CMAKE_CURRENT_LIST_DIR}/musl")
endif()


# no need for optimization or debug info for testing
# set(NX64_OPT_FLAGS "-O3 -g")
set(NX64_TRIPLE aarch64-linux-elf)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_SYSROOT ${NX_MUSL})
set(CMAKE_C_COMPILER "${NX_CLANG}/bin/clang")
set(CMAKE_C_COMPILER_TARGET ${NX64_TRIPLE})
set(CMAKE_CXX_COMPILER "${NX_CLANG}/bin/clang++")
set(CMAKE_CXX_COMPILER_TARGET ${NX64_TRIPLE})

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Target options
add_compile_options(-mcpu=cortex-a57+fp+simd+crypto+crc)
add_compile_options(-mno-implicit-float)
# Environment
add_compile_options(-stdlib=libc++)
add_compile_options(-fPIC)
# Helps with matching as this causes Clang to emit debug type info even for dynamic classes
# with undefined vtables.
add_compile_options(-fstandalone-debug)

add_definitions(-D SWITCH)
add_definitions(-D NNSDK)
add_definitions(-D MATCHING_HACK_NX_CLANG)

add_link_options(
  # Use lld for performance reasons (and because we don't want a dependency on GNU tools)
  -fuse-ld=lld
  # We are not linking executables so these aren't needed
  # (technically we are not linking at all but CMake will always ensure linking is possible)
  -nostartfiles -nodefaultlibs
)

# TODO: Currently, we only build for a stub versions
# this means we might not catch errors that only happen
# in one of the versions. This is partially mitigated
# by building SMO and BOTW in CI
set(NN_SDK 1.2.3)
set(NN_WARE 4.5.6)
