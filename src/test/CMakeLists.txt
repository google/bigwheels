# Copyright 2022 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# For Windows: prevent overriding the parent project's compiler/linker settings.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/googletest third_party/googletest)

# Macro to create a test executable given the test name and
# the list of test code sources.
macro(package_add_test TESTNAME)
    add_executable(${TESTNAME} ${ARGN})
    add_dependencies(build-tests ${TESTNAME})
    target_link_libraries(${TESTNAME} ${PROJECT_NAME} gtest gmock gtest_main)
    set_target_properties(${TESTNAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
    )
    gtest_discover_tests(${TESTNAME}
        # Set working directory to project output root.
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
endmacro()

# The default `test` target automatically created by CMake runs the tests but
# does not build them.
# This custom `build-tests` target builds all the tests.
add_custom_target(build-tests)
# This custom `run-tests` target builds and runs all the tests.
add_custom_target(run-tests
    COMMAND ${CMAKE_CTEST_COMMAND}
    DEPENDS build-tests
)

# List of test sources. Add new tests here.
list(
    APPEND TEST_SOURCES
    command_line_parser_test.cpp
    format_test.cpp
    knob_test.cpp
    log_console_test.cpp
    metrics_test.cpp
    ppm_export_test.cpp
    string_util_test.cpp
    transform_test.cpp
    filesystem_test.cpp
)
package_add_test(ppx_tests ${TEST_SOURCES})
