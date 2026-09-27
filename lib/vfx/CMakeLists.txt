file(GLOB_RECURSE NN_VFX_HEADERS CONFIGURE_DEPENDS
  include/*.h
)
file(GLOB_RECURSE NN_VFX_SOURCES CONFIGURE_DEPENDS
  src/*.cpp
)

add_library(nn_vfx STATIC ${NN_VFX_SOURCES})

target_sources(nn_vfx PUBLIC
  FILE_SET HEADERS
  BASE_DIRS include/
  FILES ${NN_VFX_HEADERS}
)

set_target_properties(nn_vfx PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)

nnware_ver(nn_vfx)
target_compile_options(nn_vfx PRIVATE ${NNWARE_COMPILE_FLAGS})
nnware_header_verification_compile_options(nn_vfx)

target_link_libraries(nn_vfx PUBLIC nn_g3d)
