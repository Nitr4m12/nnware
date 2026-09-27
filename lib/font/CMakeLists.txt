file(GLOB_RECURSE NN_FONT_HEADERS CONFIGURE_DEPENDS
  include/*.h
)
file(GLOB_RECURSE NN_FONT_SOURCES CONFIGURE_DEPENDS
  src/*.cpp
)

add_library(nn_font STATIC ${NN_FONT_SOURCES})

target_sources(nn_font PUBLIC
  FILE_SET HEADERS
  BASE_DIRS include/
  FILES ${NN_FONT_HEADERS}
)

set_target_properties(nn_font PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)

nnware_ver(nn_font)
target_compile_options(nn_font PRIVATE ${NNWARE_COMPILE_FLAGS})
nnware_header_verification_compile_options(nn_font)
