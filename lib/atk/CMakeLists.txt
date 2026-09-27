file(GLOB_RECURSE NN_ATK_HEADERS CONFIGURE_DEPENDS
  include/*.h
)
file(GLOB_RECURSE NN_ATK_SOURCES CONFIGURE_DEPENDS
  src/*.cpp
)

add_library(nn_atk STATIC ${NN_ATK_SOURCES})

target_sources(nn_atk PUBLIC
  FILE_SET HEADERS
  BASE_DIRS include/
  FILES ${NN_ATK_HEADERS}
)

set_target_properties(nn_atk PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)

nnware_ver(nn_atk)
target_compile_options(nn_atk PRIVATE ${NNWARE_COMPILE_FLAGS})
nnware_header_verification_compile_options(nn_atk)
