# GTK4 driver

find_package(PkgConfig REQUIRED)

file(GLOB _GTK4_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/gtk4/iupgtk4_*.c")

if(APPLE)
  list(FILTER _GTK4_SOURCES EXCLUDE REGEX "iupgtk4_info\\.c$")
endif()

set(IUP_DRIVER_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/gtk4")

if(UNIX AND NOT APPLE)
  include(IUPUnixCommon)
  list(APPEND _GTK4_SOURCES ${IUP_UNIX_COMMON_SOURCES})
  list(APPEND _GTK4_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_sni.c")
  list(APPEND IUP_DRIVER_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/unix")
elseif(APPLE)
  list(APPEND _GTK4_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_help.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_info.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_tray.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_notify.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_singleinstance.m"
  )
  set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_help.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_info.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_tray.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_notify.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_singleinstance.m"
    PROPERTIES LANGUAGE OBJC
  )
  list(APPEND IUP_DRIVER_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/unix"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa"
  )
elseif(WIN32)
  list(APPEND _GTK4_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwin_tray.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwin_notify.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwin_singleinstance.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwindows_info.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwindows_help.c"
  )
  list(APPEND IUP_DRIVER_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/win")
endif()

set(IUP_DRIVER_SOURCES ${_GTK4_SOURCES})

pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)

set(IUP_DRIVER_COMPILE_DEFINITIONS
  IUP_USE_GTK4
  GDK_DISABLE_DEPRECATED
  GTK_DISABLE_DEPRECATED
)

set(IUP_DRIVER_LINK_LIBRARIES PkgConfig::GTK4)

if(UNIX AND NOT APPLE)
  pkg_check_modules(GTK4_WAYLAND IMPORTED_TARGET gtk4-wayland)
  pkg_check_modules(GTK4_X11 IMPORTED_TARGET gtk4-x11)
  if(GTK4_WAYLAND_FOUND)
    list(APPEND IUP_DRIVER_LINK_LIBRARIES PkgConfig::GTK4_WAYLAND)
  endif()
  if(GTK4_X11_FOUND)
    list(APPEND IUP_DRIVER_LINK_LIBRARIES PkgConfig::GTK4_X11)
  endif()
  list(APPEND IUP_DRIVER_COMPILE_DEFINITIONS IUPDBUS_USE_DLOPEN IUPX11_USE_DLOPEN)
  list(APPEND IUP_DRIVER_LINK_LIBRARIES dl m)
elseif(APPLE)
  list(APPEND IUP_DRIVER_LINK_LIBRARIES
    "-framework AppKit"
    "-framework SystemConfiguration"
    "-framework UserNotifications"
  )
endif()

set(IUP_DRIVER_COMPILE_OPTIONS "")

set(IUP_PC_REQUIRES "gtk4")
if(GTK4_WAYLAND_FOUND)
  string(APPEND IUP_PC_REQUIRES " gtk4-wayland")
endif()
if(GTK4_X11_FOUND)
  string(APPEND IUP_PC_REQUIRES " gtk4-x11")
endif()
set(IUP_PC_LIBS_PRIVATE "")
if(UNIX AND NOT APPLE)
  set(IUP_PC_LIBS_PRIVATE "-ldl -lm")
endif()
