# 
# Copyright (C) 2017 Nabto - All Rights Reserved.
# 

cmake_minimum_required(VERSION 3.2)
project (nabto-cli)

# Find the library the linker needs to use.
find_library(NABTO_CORE_LIB
  NAMES "nabto_client_api"
  PATHS "lib"
)

# Find the library we need to copy to the output folder such that the folder is selfcontained.
find_file(NABTO_CORE_LIB_INSTALL
  NAMES "libnabto_client_api.so" "libnabto_client_api.dylib" "nabto_client_api.dll"
  PATHS "lib"
)
  
if(NOT NABTO_CORE_LIB)
  message(FATAL_ERROR "Nabto lib not found - please download and install as per the README file")
endif()

SET(CMAKE_INSTALL_RPATH "$ORIGIN")

add_executable (nabto-cli src/nabto_cli.cpp src/tunnel_manager.cpp)
target_compile_features(nabto-cli PRIVATE cxx_range_for)

include_directories(include)
target_link_libraries (nabto-cli ${NABTO_CORE_LIB})

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  target_link_libraries(nabto-cli pthread dl)
endif()

# install dependent shared library to make the installation folder selfcontained.
install(FILES ${NABTO_CORE_LIB_INSTALL} DESTINATION .)

install (TARGETS nabto-cli DESTINATION .)

# On apple the rpath trick does not work as intended, fix it given the idea from the following blog post
# https://blogs.oracle.com/dipol/dynamic-libraries,-rpath,-and-mac-os
if (APPLE)
  install(
    CODE "execute_process(COMMAND install_name_tool -change libnabto_client_api.dylib \"@loader_path/libnabto_client_api.dylib\" ${CMAKE_INSTALL_PREFIX}/bin/nabto-cli)"
)
endif()

string(TOLOWER ${CMAKE_SYSTEM_NAME} osname)

set(CPACK_GENERATOR "ZIP")

set( CPACK_PACKAGE_FILE_NAME nabto-cli-${osname})
include(CPack)


