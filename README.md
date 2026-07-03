# libapclient

A C++ [Archipelago](https://archipelago.gg/) client library.

Can be used to create any type of client: text-only (chat), tracker, or integrate with a game to provide support for a game.

The library is designed to be usable as a CMake module, meaning that it can be loaded via [CPM.cmake](https://github.com/cpm-cmake/cpm.cmake) or using [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html) directly.

## Building

Building this directly is fairly simple: Clone the repository, create a build directory, and run CMake in it.

For example:

```sh
git clone https://github.com/Xenoveritas/libapclient.git
cd libapclient
mkdir build
cd build
cmake -D LIBAPCLIENT_BUILD_EXAMPLE=ON -S ..
make
```

Visual Studio makes building it for Windows even simpler: just open the cloned project in Visual Studio and it will handle running CMake directly.

The example client can then be run, which can be used as a basic text client.

## Using as a library

The base `archipelago::Client` is designed to be extended. It provides hooks for the various packets that can be received and various parts of the connection lifecycle.

An example client is provided in the example directory.