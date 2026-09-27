file(GLOB_RECURSE NN_UI2D_HEADERS CONFIGURE_DEPENDS
  include/*.h
)
file(GLOB_RECURSE NN_UI2D_SOURCES CONFIGURE_DEPENDS
  src/*.cpp
)

add_library(nn_ui2d STATIC ${NN_UI2D_SOURCES})

target_sources(nn_ui2d PUBLIC
  FILE_SET HEADERS
  BASE_DIRS include/
  FILES ${NN_UI2D_HEADERS}
)

set_target_properties(nn_ui2d PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)

nnware_ver(nn_ui2d)
target_compile_options(nn_ui2d PRIVATE ${NNWARE_COMPILE_FLAGS})
nnware_header_verification_compile_options(nn_ui2d)

target_link_libraries(nn_ui2d PUBLIC nn_font)
target_link_libraries(nn_ui2d PUBLIC nn_gfx)
