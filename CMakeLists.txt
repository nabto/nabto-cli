cmake_minimum_required(VERSION 3.8)
project (nabto-cli)

find_library(NABTO_CORE_LIB
    NAMES "libnabto_client_api_static.a"
    PATHS "lib"
)
if(NOT NABTO_CORE_LIB)
  message(FATAL_ERROR "nabto lib not found - please download and install as per the README file")
endif()

find_library(NABTO_EXT_LIB
    NAMES "libnabto_static_external.a"
    PATHS "lib"
)
if(NOT NABTO_EXT_LIB)
  message(FATAL_ERROR "nabto external lib not found - please download and install as per the README file")
endif()

add_executable (nabto-cli src/nabto_cli.cpp src/tunnel_manager.cpp)
target_compile_features(nabto-cli PRIVATE cxx_range_for)

include_directories(include)
target_link_libraries (nabto-cli ${NABTO_CORE_LIB} ${NABTO_EXT_LIB})

install (TARGETS nabto-cli DESTINATION bin)
