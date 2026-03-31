# WinUI / XAML Islands driver (requires Clang++)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR
    "WinUI backend requires Clang++ compiler. "
    "Set CMAKE_CXX_COMPILER to clang++ or use the 'winui' CMake preset."
  )
endif()

file(GLOB _WINUI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/winui/iupwinui_*.cpp")

# C++20 and libc++ apply only to the C++ driver sources, not C core files
set_source_files_properties(${_WINUI_SOURCES}
  PROPERTIES COMPILE_OPTIONS "-std=c++20;-stdlib=libc++"
)

# WinUI reuses Win32 singleinstance and thread implementations
list(APPEND _WINUI_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwin_singleinstance.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwin_thread.c"
)

set(IUP_DRIVER_SOURCES ${_WINUI_SOURCES})

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

set(IUP_PC_REQUIRES "")
set(IUP_PC_LIBS_PRIVATE "-lwindowsapp -lruntimeobject -ld2d1 -ldwrite -luuid -loleaut32 -lole32 -luser32 -lshell32 -lgdi32 -ladvapi32 -l:libc++.a -l:libc++abi.a")
