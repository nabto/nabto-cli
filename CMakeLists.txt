# 
# Copyright (C) 2017 Nabto - All Rights Reserved.
# 

# For a simple build file which only targets linux see simplebuild/CMakeLists.txt

cmake_minimum_required(VERSION 3.2)
project (nabto-cli)
set (CMAKE_CXX_STANDARD 11)

include(InstallRequiredSystemLibraries)

# Find the library the linker needs to use.
find_library(NABTO_CORE_LIB
  NAMES "nabto_client_api"
  PATHS "lib"
)

# Find the library we need to copy to the output folder such that the folder is selfcontained.
find_file(NABTO_CORE_LIB_INSTALL
  NAMES "libnabto_client_api.so" "libnabto_client_api.dylib" "nabto_client_api.dll" "msvcp140.dll" "vcruntime140.dll"
  PATHS "lib"
  )

if(NOT NABTO_CORE_LIB)
  message(FATAL_ERROR "Nabto lib not found - please download and install as per the README file")
endif()

SET(CMAKE_INSTALL_RPATH "$ORIGIN")

add_executable (nabto-cli src/nabto_cli.cpp src/tunnel_manager.cpp 3rdparty/jsoncpp.cpp)
target_compile_features(nabto-cli PRIVATE cxx_range_for)

include_directories(include 3rdparty)
target_link_libraries (nabto-cli ${NABTO_CORE_LIB})

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  target_link_libraries(nabto-cli pthread dl)
endif()

# install dependent shared library to make the installation folder selfcontained.
install(FILES ${NABTO_CORE_LIB_INSTALL} DESTINATION .)
install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION .)
install (TARGETS nabto-cli DESTINATION .)

# On apple the rpath trick does not work as intended, fix it given the idea from the following blog post
# https://blogs.oracle.com/dipol/dynamic-libraries,-rpath,-and-mac-os
if (APPLE)
  # Default when only running make, the executable cannot find the libnabto_client_api.dylib this fixes it.
  add_custom_command(
    TARGET nabto-cli
    POST_BUILD
    COMMAND install_name_tool -change libnabto_client_api.dylib "${CMAKE_SOURCE_DIR}/lib/libnabto_client_api.dylib" $<TARGET_FILE:nabto-cli>)

  # When installing the nabto-cli the libnabto_client_api.dylib is placed alongside the executable and the rpath needs to reflect that.
  install(
    CODE "execute_process(COMMAND install_name_tool -change \"${CMAKE_SOURCE_DIR}/lib/libnabto_client_api.dylib\" \"@loader_path/libnabto_client_api.dylib\" ${CMAKE_INSTALL_PREFIX}/nabto-cli)"
)
endif()

# Here be dragons! Cpack does the wrong thing on osx, the executable added 
# to the zip file does not include the correct executable where the correct
# install_name_tool magic has been applied.

string(TOLOWER ${CMAKE_SYSTEM_NAME} osname)

set(CPACK_GENERATOR "ZIP")

set( CPACK_PACKAGE_FILE_NAME nabto-cli-${osname})
include(CPack)


