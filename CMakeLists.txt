# 
# Copyright (C) 2017 Nabto - All Rights Reserved.
# 

# For a simple build file which only targets linux see simplebuild/CMakeLists.txt

cmake_minimum_required(VERSION 3.10)
project (nabto-cli)
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

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

if (APPLE)
  # The nabto dylib has the install name @rpath/libnabto_client_api.dylib, so
  # look for it next to the executable when installed.
  SET(CMAKE_INSTALL_RPATH "@loader_path")
else()
  SET(CMAKE_INSTALL_RPATH "$ORIGIN")
endif()

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

string(TOLOWER ${CMAKE_SYSTEM_NAME} osname)
if (APPLE)
  # keep the release asset name used for earlier releases
  set(osname "osx")
endif()

set(CPACK_GENERATOR "ZIP")

set( CPACK_PACKAGE_FILE_NAME nabto-cli-${osname})
include(CPack)


