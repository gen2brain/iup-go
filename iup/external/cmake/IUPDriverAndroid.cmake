# Android driver

if(NOT ANDROID)
  message(FATAL_ERROR "IUP_BACKEND=android requires the Android NDK toolchain. Pass -DCMAKE_TOOLCHAIN_FILE=\$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake")
endif()

file(GLOB _ANDROID_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/android/iupandroid_*.c")

set(IUP_DRIVER_SOURCES ${_ANDROID_SOURCES})

set(IUP_DRIVER_COMPILE_DEFINITIONS IUP_USE_ANDROID)

set(IUP_DRIVER_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/android")

set(IUP_DRIVER_LINK_LIBRARIES log jnigraphics dl m)

set(IUP_DRIVER_COMPILE_OPTIONS "")
set(IUP_DRIVER_LINK_OPTIONS "")

set(IUP_PC_REQUIRES "")
set(IUP_PC_LIBS_PRIVATE "-llog -ljnigraphics -ldl -lm")
