# Nabto CLI Demo

This application demonstrates how to use the Nabto Client SDK to exercise certificate management, device discovery, RPC device invocation and tunnel management.

## SDK installation

To build the Nabto CLI Demo, first download the Nabto Client SDK bundle from https://www.nabto.com/downloads.html. Note: The demo uses the 4.1.0 version of the Nabto Client SDK; as of writing, only a pre-release version with linux64 libraries is available. This early version can be downloaded here: https://www.nabto.com/downloads/nabto-libs/4.1.0-rc1/nabto_libraries.zip

After download, install the libraries and header files into the `lib` and `include` directories, so you have the following structure:

```console
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

```console
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

## Examples

### Create certificate

```console
$ ./nabto-cli --create-cert --cert-name nabto-user
Created self signed cert with fingerprint [53:84:b3:a6:f6:4a:c5:73:4e:5d:7a:3a:62:36:11:21]
```


### Invoke RPC function

```console
$ ./nabto-cli --cert-name nabto-user --interface-definition ../unabto_queries.xml \
  --rpc-invoke-url nabto://xj00cmgr.nw7xqz.appmyproduct.com/get_public_device_info.json? 
{
   "request" : {},
   "response" : {
      "device_icon" : "img/chip-small.png",
      "device_name" : "AMP stub",
      "device_type" : "ACME 9002 Heatpump",
      "is_current_user_owner" : 0,
      "is_current_user_paired" : 0,
      "is_open_for_pairing" : 1
   }
}
```


### Open TCP tunnel using ephemeral port

Per default a TCP tunnel connects to a TCP socket on localhost on the remote peer, the only mandatory parameters are the remote nabto device id and the remote TCP port. For instance, the following retrieves a page from an HTTP server on the remote peer:

```console
$ ./nabto-cli --cert-name nabto-user --tunnel-host xj00cmgr.nw7xqz.appmyproduct.com \
  --tunnel-remote-port 80
State has changed for tunnel 0x961aa635 status CONNECTING (0)
State has changed for tunnel 0x961aa635 status REMOTE_P2P (4)
Tunnel 0x961aa635 connected, tunnel version: 1, local TCP port: 54285
```

The ephemeral port 54285 was chosen by the system, the webpage can now be retrieved using:

```console
$ curl -s http://127.0.0.1:54285
<!DOCTYPE html>
<html>
<head>
<title>Welcome to nginx on Debian!</title>
<style>
```


### Open TCP tunnel using specific port 

Set the local TCP tunnel client end point to listen on port 12345:

```console
$ ./nabto-cli --cert-name nabto-user --tunnel-local-port 12345 \
  --tunnel-host xj00cmgr.nw7xqz.appmyproduct.com \
  --tunnel-remote-port 80
State has changed for tunnel 0x961aa635 status CONNECTING (0)
State has changed for tunnel 0x961aa635 status REMOTE_P2P (4)
Tunnel 0x961aa635 connected, tunnel version: 1, local TCP port: 12345
```

```console
$ curl -s http://127.0.0.1:12345
<!DOCTYPE html>
[...]
```


### Open TCP tunnel to using a non-local remote TCP server

Open a local TCP tunnel client end point on port 12345. The remote tunnel endpoint must connect to TCP port 80 on the host 192.168.1.123, reachable from the remote peer:

```console
$ ./nabto-cli --cert-name nabto-user --tunnel-local-port 12345 \
  --tunnel-host xj00cmgr.nw7xqz.appmyproduct.com \
  --tunnel-remote-host 192.168.1.123 --tunnel-remote-port 80
State has changed for tunnel 0x961aa635 status CONNECTING (0)
State has changed for tunnel 0x961aa635 status REMOTE_P2P (4)
Tunnel 0x961aa635 connected, tunnel version: 1, local TCP port: 12345
```

```console
$ curl -s http://127.0.0.1:12345
<!DOCTYPE html>
[...]
```

