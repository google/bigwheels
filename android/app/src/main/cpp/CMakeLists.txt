# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.18.1)

# Declares and names the project.

project("bigwheels")

if(PPX_BUILD_XR AND PPX_XR_QUEST)
  # Quest devices do not use the standard OpenXR loader.
  add_library(openxr_loader SHARED IMPORTED)
  set_property(
          TARGET
          openxr_loader
          PROPERTY
          IMPORTED_LOCATION
          ${OCULUS_OPENXR_SDK}/OpenXR/Libs/Android/${ANDROID_ABI}/Debug/libopenxr_loader.so)
endif()

set(BIGWHEELS_ROOT_DIR "${CMAKE_SOURCE_DIR}/../../../../..")
add_subdirectory(${BIGWHEELS_ROOT_DIR} "${BIGWHEELS_ROOT_DIR}/android/app/build")

add_library(bigwheels SHARED main.cpp)

find_library(log-lib log)

# Note: android and game-activity::game-activity are already linked
# in BigWheels CMakeLists and do not need to be linked here

target_link_libraries(bigwheels ${PPX_PROJECT} ${log-lib})

