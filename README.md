# Nabto CLI Demo

This application demonstrates how to use the Nabto Client SDK to exercise certificate management, device discovery, RPC device invocation and tunnel management.

## SDK installation

To build the Nabto CLI Demo, first download the Nabto Client SDK bundle from https://www.nabto.com/downloads.html. Note: The demo uses the 4.1.0 version of the Nabto Client SDK; as of writing, only a pre-release version with linux64 libraries is available. This early version can be downloaded here: https://www.nabto.com/downloads/nabto-libs/4.1.0-rc1/nabto_libraries.zip

After download, install the libraries and header files into the `lib` and `include` directories, so you have the following structure:

```
.
├── CMakeLists.txt
├── README.md
├── include
│   ├── cxxopts.hpp
│   └── nabto_client_api.h
├── lib
│   ├── libnabto_client_api_static.a
│   └── libnabto_static_external.a
└── src
    ├── nabto_cli.cpp
    ├── tunnel_manager.cpp
    └── tunnel_manager.hpp
```

Note that no resources are necessary to install, with the 4.1.0 release these come bundles in the SDK and are automatically installed by the demo app (through `nabtoInstallDefaultStaticResources` in the client API).

## Building and running

To build the app, a C++11 compiler and CMake must be available and the Nabto Client SDK installed as described above:

```
$ mkdir build
$ cd build
$ cmake ..
-- The C compiler identification is AppleClang 8.1.0.8020042
-- The CXX compiler identification is AppleClang 8.1.0.8020042
-- Check for working C compiler: /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc
[...]
$ make -j 9
Scanning dependencies of target nabto-cli
[ 66%] Building CXX object CMakeFiles/nabto-cli.dir/src/nabto_cli.cpp.o
[ 66%] Building CXX object CMakeFiles/nabto-cli.dir/src/tunnel_manager.cpp.o
[100%] Linking CXX executable nabto-cli
[100%] Built target nabto-cli
$ ./nabto-cli --help
Nabto Command Line demo
Usage:
  ./nabto-cli [OPTION...]

  -c, --create-cert             Create self signed certificate
[...]
```
