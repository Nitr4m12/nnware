cmake_minimum_required(VERSION 3.24)

project(NintendoWare C CXX)

# Downstream projects should set something like:
# set(NN_WARE 1.2.3)
function(nnware_ver TARGET)
  set(VER ${NN_WARE})
  string(REPLACE "." ";" VER "${VER}")

  list(LENGTH VER SEGS)
  if(SEGS LESS 3)
    set(DEF_VER 1 0 0)
    list(SUBLIST DEF_VER ${SEGS} -1 DEF_VER)
    list(APPEND VER ${DEF_VER})

    message(AUTHOR_WARNING "NN_WARE semver is malformed (should be %d.%d.%d): ${NN_WARE}")
  endif()

  list(POP_FRONT VER MAJOR MINOR PATCH)

  foreach(VAR IN ITEMS MAJOR MINOR PATCH)
    target_compile_definitions(${TARGET} PUBLIC NN_WARE_${VAR}=${${VAR}})
  endforeach()
endfunction()

# these flags are shared with all modules
set(NNWARE_COMPILE_FLAGS
  -fno-strict-aliasing
  -fno-exceptions
  -Wall
  -Wextra
  -Wno-invalid-offsetof
  # some WIP stuff might contain unused fields
  -Wno-unused-private-field
)
function(nnware_header_verification_compile_options TARGET)
  target_compile_options(${TARGET} INTERFACE
    "$<$<STREQUAL:$<TARGET_PROPERTY:NAME>,${TARGET}_verify_interface_header_sets>:${NNWARE_COMPILE_FLAGS}>"
  )
endfunction()

if(NOT TARGET nnSdk)
  if(NOT DEFINED NNSDK_DIR)
    message(FATAL_ERROR "NintendoSDK cannot be found. Ensure your project adds open-ead/nnsdk as a dependency (submodule), and your CMakeLists.txt contains `add_subdirectory(path/to/nnsdk)`")
  endif()
  if(NOT IS_DIRECTORY "${NNSDK_DIR}")
    message(FATAL_ERROR "NNSDK_DIR is provided, but NintendoSDK is not found at: ${NNSDK_DIR}")
  endif()
  message(STATUS "Found NintendoSDK: ${NNSDK_DIR}")
  add_subdirectory(${NNSDK_DIR} nnsdk)
endif()

add_subdirectory(lib/atk)
add_subdirectory(lib/font)
add_subdirectory(lib/g3d)
add_subdirectory(lib/ui2d)
add_subdirectory(lib/vfx)
