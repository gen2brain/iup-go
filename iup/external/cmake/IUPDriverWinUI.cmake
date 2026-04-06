# WinUI / XAML Islands driver (requires Clang++)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR
    "WinUI backend requires Clang++ compiler. "
    "Set CMAKE_CXX_COMPILER to clang++ or use the 'winui' CMake preset."
  )
endif()

file(GLOB _WINUI_CXX_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/winui/iupwinui_*.cpp")

# Build WinUI C++ sources as an object library so we can attach a PCH.
add_library(iup_winui_driver OBJECT ${_WINUI_CXX_SOURCES})

target_compile_options(iup_winui_driver PRIVATE -std=c++20 -stdlib=libc++)

target_compile_definitions(iup_winui_driver PRIVATE
  IUP_BUILD_LIBRARY
  IUP_USE_WINUI
  UNICODE
  _UNICODE
  _WIN32_WINNT=0x0A00
  NTDDI_VERSION=0x0A000000
)

target_include_directories(iup_winui_driver PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/winui"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/winui/winrt"
)

set_target_properties(iup_winui_driver PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_precompile_headers(iup_winui_driver PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src/winui/pch.h"
)

# WinUI reuses Win32 singleinstance and thread implementations (plain C).
# Use $<TARGET_OBJECTS:> to merge C++ objects directly into the iup target
# so they share the same link unit and can resolve core IUP symbols.
set(IUP_DRIVER_SOURCES
  $<TARGET_OBJECTS:iup_winui_driver>
  "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwin_singleinstance.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwin_thread.c"
)

set(IUP_DRIVER_COMPILE_DEFINITIONS
  IUP_USE_WINUI
  UNICODE
  _UNICODE
  _WIN32_WINNT=0x0A00
  NTDDI_VERSION=0x0A000000
)

set(IUP_DRIVER_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/winui"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/winui/winrt"
)

set(IUP_DRIVER_LINK_LIBRARIES
  windowsapp runtimeobject d2d1 dwrite uuid oleaut32 ole32 user32 shell32 gdi32 advapi32 -l:libc++.a -l:libc++abi.a
)

set(IUP_DRIVER_COMPILE_OPTIONS "")
set(IUP_DRIVER_LINK_OPTIONS -fuse-ld=lld)

set(IUP_PC_REQUIRES "")
set(IUP_PC_LIBS_PRIVATE "-lwindowsapp -lruntimeobject -ld2d1 -ldwrite -luuid -loleaut32 -lole32 -luser32 -lshell32 -lgdi32 -ladvapi32 -l:libc++.a -l:libc++abi.a")
