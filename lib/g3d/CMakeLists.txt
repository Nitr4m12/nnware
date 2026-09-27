file(GLOB_RECURSE NN_G3D_HEADERS CONFIGURE_DEPENDS
  include/*.h
)
file(GLOB_RECURSE NN_G3D_SOURCES CONFIGURE_DEPENDS
  src/*.cpp
)

add_library(nn_g3d STATIC ${NN_G3D_SOURCES})

target_sources(nn_g3d PUBLIC
  FILE_SET HEADERS
  BASE_DIRS include/
  FILES ${NN_G3D_HEADERS}
)

set_target_properties(nn_g3d PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)

nnware_ver(nn_g3d)
target_compile_options(nn_g3d PRIVATE ${NNWARE_COMPILE_FLAGS})
nnware_header_verification_compile_options(nn_g3d)

target_link_libraries(nn_g3d PUBLIC nn_gfx)
