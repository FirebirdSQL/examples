# Firebird client usage in C++ with cmake and vcpkg

This example shows how to use Firebird client with C++, cmake and vcpkg.

The example consist of a simple code that creates and drops a database called `firebird-example-test.fdb`.

It works in Windows and Linux.

In [scripts](scripts) directory, there are scripts showing how to use cmake with different generators in the different
platforms.

The scripts can be called with environment variable `BUILD_DIR` previously set, otherwise it's automatically defined
to some sub-directory of `build` inside the source tree.

Below there are example usage of these scripts.

## Windows (Visual Studio 2022 x64 generator)

Visual Studio uses the so called multi-config cmake generator, so there is only a single generate script for both
debug and release build. This is the first script that must be run:

```
scripts\gen-windows-vs2022-x64.bat
```

It generates Visual Studio solution file that may be opened in Visual Studio, or build it from the command line with
the following script:

```
scripts\build-windows-vs2022-x64-debug.bat
```

At this time, Firebird client and the example application is built, but they live in different directories, so the
application do not know where `fbclient.dll` is.

This is fixed running the following script, that adds directory where `fbclient.dll` is to the `PATH` in the current
command line session:

```
scripts\setenv-windows-vs2022-x64-debug.bat
```

Now `build\vs2022-x64\Debug\bin\firebird-vcpkg-example.exe` can be run.

To generate a relocatable install tree that does not need the change to the `PATH`, then this script must be run:

```
scripts\install-windows-vs2022-x64-debug.bat
```

And now `build\vs2022-x64\install\Debug\bin\firebird-vcpkg-example.exe` can be run.

## Linux (Ninja Multi-Config generator)

Ninja, as Visual Studio, support cmake multi-config and this example uses it, so there is single script for both
debug and release build. This is the first script that must be run:

```
scripts/gen-linux-ninja.sh
```

It generates Ninja files that can be build from the command line with the following script:

```
scripts/build-linux-ninja-debug.sh
```

As in Windows, Firebird client and the example application are put in different directories, but different than in
Windows, it's not necessary to add `fbclient.so` location to `LD_LIBRARY_PATH`. That is because the application is
built with rpath pointing to it.

So `./build/linux-ninja/Debug/bin/firebird-vcpkg-example` can be run already.

To generate a relocatable install tree, then this script must be run:

```
scripts/install-linux-ninja-debug.sh
```

And now `./build/linux-ninja/install/Debug/bin/firebird-vcpkg-example` can be run.

## Linux (Unix Makefiles generator)

The instructions for this generator is much like the one for Linux with Ninja, but with Unix Makefiles is not
multi-config, so there are different generators for debug and release build. This is the first script that must be run:

```
scripts/gen-linux-make-debug.sh
```

It generates makefiles that can be build from the command line with the following script:

```
scripts/build-linux-make-debug.sh
```

So `./build/linux-make-Debug/bin/firebird-vcpkg-example` can be run already.

To generate a relocatable install tree, then this script must be run:

```
scripts/install-linux-make-debug.sh
```

And now `./build/linux-make-Debug/install/bin/firebird-vcpkg-example` can be run.
