# Nabto CLI Demo

[![Build Status][1]][2] [![Windows Build Status][3]][4]

[1]: https://travis-ci.org/nabto/nabto-cli.svg?branch=master
[2]: https://travis-ci.org/nabto/nabto-cli
[3]: https://ci.appveyor.com/api/projects/status/github/nabto/nabto-cli?svg=true&branch=master
[4]: https://ci.appveyor.com/project/nabto/nabto-cli

This application demonstrates how to use the Nabto Client SDK to exercise certificate management, device discovery, RPC device invocation and tunnel management. Precompiled binaries are available here: https://github.com/nabto/nabto-cli/releases

## SDK installation

To build the Nabto CLI Demo, first download the `Nabto SDK libs` bundle from https://www.nabto.com/downloads.html.

After download, install the libraries and header files into the `lib` and `include` directories, so you have the following structure:

```console
.
├── CMakeLists.txt
├── README.md
├── include
│   ├── cxxopts.hpp
│   ├── json.h
│   ├── json_helper.hpp
│   └── nabto_client_api.h
├── lib
│   └── libnabto_client_api.so
└── src
    ├── nabto_cli.cpp
    ├── jsoncpp.cpp
    ├── tunnel_manager.cpp
    └── tunnel_manager.hpp
```

Note that no resources are necessary to install, with the 4.1.0 release these come bundled in the SDK and are automatically installed by the demo app (through `nabtoInstallDefaultStaticResources` in the client API).

The lib folder should contain the following files:

| Linux                    | Mac                         | Windows                |
| ------------------------ | --------------------------- | ---------------------- |
| `libnabto_client_api.so` | `libnabto_client_api.dylib` | `nabto_client_api.lib` |
|                          |                             | `nabto_client_api.dll` |

## Building and running

To build the app, a C++11 compiler and CMake must be available and the
Nabto Client SDK installed as described above. The software is tested
to compile with gcc 4.9, VS 2017 and recent clang versions.

```console
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=../bin ..
-- The C compiler identification is AppleClang 8.1.0.8020042
-- The CXX compiler identification is AppleClang 8.1.0.8020042
-- Check for working C compiler: /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc
[...]
$ make -j 9 install
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

The example plain build target (e.g. `make` (without `install`)) configures the binary to look in the
`lib`.

The example install build target (e.g. `make install`) configures the binary to look for the Nabto
SDK dynamic library next to the binary itself: It copies the SDK library and the `nabto-cli` binary
to the location specified to CMake with `-DCMAKE_INSTALL_PREFIX=<path>`).

### Simple build file

There is a minimal `CMakeLists.txt` build file in the simplebuild
folder. The purpose of that file is to show the minimum required
effort to build the nabto cli, this can be used as an example for your
own projects which integrates the `nabto_client_api`.

## Examples

It is assumed that the fingerprint of an available certificate (see first example) is added to the ACL of the target device. See section 8 in [TEN036 "Security in Nabto Solutions"](https://www.nabto.com/downloads/docs/TEN036%20Security%20in%20Nabto%20Solutions.pdf) for further details.

It is also assumed that a uNabto device is available. For the examples, `xj00cmgr.nw7xqz.trial.nabto.net` is used which should be replaced by your device ID. To get a device ID go to https://console.cloud.nabto.com

Further more, it is assumed for the tunnel examples that the device endpoint has been configured to allow access to the specified remote hosts and TCP ports. See section 4.4.1 in [TEN030 "Nabto Tunnels"](https://www.nabto.com/downloads/docs/TEN030%20Nabto%20Tunnels.pdf) for further details.


### Create certificate

```console
$ ./nabto-cli --create-cert --cert-name nabto-user
Created self signed cert with fingerprint [53:84:b3:a6:f6:4a:c5:73:4e:5d:7a:3a:62:36:11:21]
```

### RPC functions

This example uses the [appmyproduct-device-stub device](https://github.com/nabto/appmyproduct-device-stub) as the device endpoint. This device uses the query definitions defined in the `unabto_queries.xml` file found at https://github.com/nabto/ionic-starter-nabto/blob/master/www/nabto/unabto_queries.xml.

#### Strict interface checking
When invoking RPC functionallity, Strict Interface Checking will ensure the interface definition between the device and the client is compatible. This is enabled with the `--strict-interface-check` argument, which will check the interface definition of the device with values provided by the `--interface-id` and `--interface-version` arguments.

See https://github.com/nabto/ionic-starter-nabto#rpc-interface-configuration for more information about convention based interface checking at the application level.


#### Pair with device
```console
./nabto-cli --cert-name nabto-user --pair --interface-def /path/to/unabto_queries.xml \
--strict-interface-check --interface-id 317aadf2-3137-474b-8ddb-fea437c424f4 --interface-version 1.0

Choose a device for pairing:
[q]: Quit without pairing
[0]: xj00cmgr.nw7xqz.trial.nabto.net
0
{
   "request" : {
      "name" : "nabto-user"
   },
   "response" : {
      "fingerprint" : "37b02567fbbf1257adea75dfbb6c438b",
      "name" : "nabto-user",
      "permissions" : 2147483648,
      "status" : 0
   }
}
```

#### Invoke Function
```console
$ ./nabto-cli --cert-name nabto-user --interface-def /path/to/unabto_queries.xml \
 --strict-interface-check --interface-id 317aadf2-3137-474b-8ddb-fea437c424f4 --interface-version 1.0 \
 --rpc-invoke-url nabto://xj00cmgr.nw7xqz.trial.nabto.net/get_public_device_info.json?
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
### Opening TCP tunnels

A TCP tunnel is defined using the `--tunnel` argument, which takes a string of the following format:

```
<localPort>:<remoteHost>:<remotePort>
```

`<localPort>` is where the local tunnel (this application) will listen.

`<remoteHost>` and `<remotPort>` is where the the remote tunnel endpoint connects to.

The `<localPort>:<remoteHost>:<remotePort>` argument can be specified multiple times to open multiple tunnels. The string must always contain two colons and a `<remotePort>` number, whereas `<localPort>` and `<remoteHost>` can be ommitted to use an ephemeral local port and localhost as the `<remoteHost>`, respectively. If using an ephemeral port, the actual port being listened on can be read from the console.

#### Open TCP tunnel using ephemeral port

Per default a TCP tunnel connects to a TCP socket on localhost on the remote peer, the only mandatory parameters are the remote nabto device id, the remote TCP port, and the certificate name. For instance, the following retrieves a page from an HTTP server on the remote peer:

```console
$ ./nabto-cli --cert-name nabto-user --tunnel-device xj00cmgr.nw7xqz.trial.nabto.net \
  --tunnel ::80
State has changed for tunnel 0x9f2ede35 status CONNECTING (0)
State has changed for tunnel 0x9f2ede35 status REMOTE_P2P (4)
Tunnel 0x92b29434 connected, tunnel version: 1, local TCP port: 46785
```

The ephemeral port 46785 was chosen by the system, the webpage can now be retrieved using:

```console
$ curl -s http://127.0.0.1:46785
<!DOCTYPE html>
<html>
<head>
<title>Welcome to nginx on Debian!</title>
<style>
```


#### Open TCP tunnel using specific port

Set the local TCP tunnel client end point to listen on port 12345:

```console
$ ./nabto-cli --cert-name nabto-user --tunnel-device xj00cmgr.nw7xqz.trial.nabto.net \
  --tunnel 12345::80
State has changed for tunnel 0xbc496535 status CONNECTING (0)
State has changed for tunnel 0xbc496535 status REMOTE_P2P (4)
Tunnel 0xbc496535 connected, tunnel version: 1, local TCP port: 12345
```

```console
$ curl -s http://127.0.0.1:12345
<!DOCTYPE html>
[...]
```


#### Open TCP tunnel to using a non-local remote TCP server

Open a local TCP tunnel client end point on port 12345. The remote tunnel endpoint must connect to TCP port 80 on the host 192.168.1.123, reachable from the remote peer:

```console
$ ./nabto-cli --cert-name nabto-user --tunnel-device xj00cmgr.nw7xqz.trial.nabto.net \
  --tunnel 12345:192.168.1.123:80
State has changed for tunnel 0x961aa635 status CONNECTING (0)
State has changed for tunnel 0x961aa635 status REMOTE_P2P (4)
Tunnel 0x961aa635 connected, tunnel version: 1, local TCP port: 12345
```

```console
$ curl -s http://127.0.0.1:12345
<!DOCTYPE html>
[...]
```

This can also be done using an ephemeral port by setting the local port to 0 or ommitting it completely.
