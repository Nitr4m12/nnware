# create a project that includes all headers in nnware modules and nnsdk
# and try to compile that
cmake_minimum_required(VERSION 3.24)

project(NintendoWareHeaderCheck CXX)

set(NNWARE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
add_subdirectory(${NNWARE_DIR} nnware)
get_directory_property(NNWARE_COMPILE_FLAGS DIRECTORY ${NNWARE_DIR} DEFINITION NNWARE_COMPILE_FLAGS)

set(HEADER_CHECK_TARGETS nnSdk nn_gfx nn_atk nn_font nn_g3d nn_ui2d nn_vfx)
set(HEADER_CHECK_HEADERS "")
foreach(TARGET ${HEADER_CHECK_TARGETS})
  list(APPEND HEADER_CHECK_HEADERS "$<TARGET_PROPERTY:${TARGET},HEADER_SET>")
endforeach()

# header sets are evaluated at generate time, so this stays in sync with the globs
file(GENERATE
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/header_check.cpp
  CONTENT "#include \"$<JOIN:${HEADER_CHECK_HEADERS},\"\n#include \">\"\n"
)

add_library(header_check OBJECT ${CMAKE_CURRENT_BINARY_DIR}/header_check.cpp)
target_link_libraries(header_check PRIVATE ${HEADER_CHECK_TARGETS})
target_compile_options(header_check PRIVATE ${NNWARE_COMPILE_FLAGS})
