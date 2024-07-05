# Code Coverage

## Linux GCC (gcov)

Prerequisites:

- Install a gcov report tool, e.g. gcovr: `sudo apt install gcovr`

First, compile with coverage enabled. For GCC, this means providing `-fprofile-arcs -ftest-coverage` to both the compiler and linker. In addition, compile with debug information for more accurate results. With CMake, we can do that like so:

```sh
cmake . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fprofile-arcs -ftest-coverage" -DCMAKE_C_FLAGS="-fprofile-arcs -ftest-coverage" -DCMAKE_EXE_LINKER_FLAGS="-fprofile-arcs -ftest-coverage"
```

Then, build:

```sh
ninja -C build
```

NOTE: It is recommended to do a full build. Otherwise, coverage for files not included in the unit tests will be ommitted. This results in inaccurate reports. We can filter later with gcov to pare down the information in the report.

Next, run the unit tests to determine lines exercised:

```sh
ninja -C build run-tests
```

Finally, run the report. This filters for only BigWheels source files:

```sh
cd build
gcovr --gcov-ignore-parse-errors all --html-details coverage.html -f '.*/include/ppx/.*' -f '.*/src/ppx/.*' -j $(nproc) -r ..
```

Open `coverage.html` with your browser.

For more information -- including details about how gcov generates and tracks coverage -- read the [gcov docs](https://gcc.gnu.org/onlinedocs/gcc/Gcov.html).
