## This is an example on how simple the nabto-cli app can be built for
## linux. For a crossplatform build file with dynamic library magic
## see ../CMakeLists.txt

cmake_minimum_required(VERSION 3.2)
project(nabto-cli)
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set(root_dir ${CMAKE_CURRENT_SOURCE_DIR}/..)

set(nabto_client_api_lib ${root_dir}/lib/libnabto_client_api.so)

set(src
  ${root_dir}/src/nabto_cli.cpp
  ${root_dir}/src/nabto_cli.cpp
  ${root_dir}/src/tunnel_manager.cpp
  ${root_dir}/3rdparty/jsoncpp.cpp
  )

add_executable (nabto-cli ${src})
target_compile_features(nabto-cli PRIVATE cxx_range_for)

include_directories(${root_dir}/include ${root_dir}/3rdparty)

target_link_libraries(nabto-cli ${nabto_client_api_lib} pthread dl)
