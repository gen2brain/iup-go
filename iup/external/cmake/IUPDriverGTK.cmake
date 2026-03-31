# GTK2/GTK3 driver

find_package(PkgConfig REQUIRED)

file(GLOB _GTK_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/gtk/iupgtk_*.c")

list(FILTER _GTK_SOURCES EXCLUDE REGEX "iupgtk_tray_")

if(APPLE)
  list(FILTER _GTK_SOURCES EXCLUDE REGEX "iupgtk_info\\.c$")
endif()

if(IUP_USE_XEMBED)
  list(APPEND _GTK_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gtk/iupgtk_tray_xembed.c"
  )
endif()

set(IUP_DRIVER_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/gtk"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/unix"
)

if(UNIX AND NOT APPLE)
  include(IUPUnixCommon)
  list(APPEND _GTK_SOURCES ${IUP_UNIX_COMMON_SOURCES})
  if(NOT IUP_USE_XEMBED)
    list(APPEND _GTK_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_sni.c")
  endif()
elseif(APPLE)
  list(APPEND _GTK_SOURCES
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
  list(APPEND IUP_DRIVER_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa")
elseif(WIN32)
  list(APPEND _GTK_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwin_tray.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwin_notify.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwin_singleinstance.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwindows_info.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwindows_help.c"
  )
  list(APPEND IUP_DRIVER_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/win")
endif()

set(IUP_DRIVER_SOURCES ${_GTK_SOURCES})

if(IUP_BACKEND STREQUAL "gtk2")
  pkg_check_modules(GTK2 REQUIRED IMPORTED_TARGET gtk+-2.0 gdk-2.0)
  pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)

  set(IUP_DRIVER_COMPILE_DEFINITIONS
    IUP_USE_GTK2
    GDK_DISABLE_DEPRECATED
    GTK_DISABLE_DEPRECATED
  )
  set(IUP_DRIVER_LINK_LIBRARIES PkgConfig::GTK2 PkgConfig::X11)
else()
  pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)

  set(IUP_DRIVER_COMPILE_DEFINITIONS
    IUP_USE_GTK3
    GDK_DISABLE_DEPRECATED
    GTK_DISABLE_DEPRECATED
  )
  set(IUP_DRIVER_LINK_LIBRARIES PkgConfig::GTK3)

  if(UNIX AND NOT APPLE)
    pkg_check_modules(GDK3_WAYLAND IMPORTED_TARGET gdk-wayland-3.0)
    pkg_check_modules(GDK3_X11 IMPORTED_TARGET gdk-x11-3.0)
    if(GDK3_WAYLAND_FOUND)
      list(APPEND IUP_DRIVER_LINK_LIBRARIES PkgConfig::GDK3_WAYLAND)
    endif()
    if(GDK3_X11_FOUND)
      list(APPEND IUP_DRIVER_LINK_LIBRARIES PkgConfig::GDK3_X11)
    endif()
  endif()
endif()

if(UNIX AND NOT APPLE)
  list(APPEND IUP_DRIVER_COMPILE_DEFINITIONS IUPDBUS_USE_DLOPEN)
  list(APPEND IUP_DRIVER_LINK_LIBRARIES dl m)
elseif(APPLE)
  list(APPEND IUP_DRIVER_LINK_LIBRARIES
    "-framework AppKit"
    "-framework SystemConfiguration"
    "-framework UserNotifications"
  )
endif()

set(IUP_DRIVER_COMPILE_OPTIONS "")

if(IUP_BACKEND STREQUAL "gtk2")
  set(IUP_PC_REQUIRES "gtk+-2.0 gdk-2.0 x11")
else()
  set(IUP_PC_REQUIRES "gtk+-3.0")
  if(GDK3_WAYLAND_FOUND)
    string(APPEND IUP_PC_REQUIRES " gdk-wayland-3.0")
  endif()
  if(GDK3_X11_FOUND)
    string(APPEND IUP_PC_REQUIRES " gdk-x11-3.0")
  endif()
endif()
set(IUP_PC_LIBS_PRIVATE "")
if(UNIX AND NOT APPLE)
  set(IUP_PC_LIBS_PRIVATE "-ldl -lm")
endif()
