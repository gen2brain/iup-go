# FLTK driver

file(GLOB _FLTK_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/fltk/iupfltk_*.cpp")

set(IUP_DRIVER_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/fltk"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/unix"
)

if(UNIX AND NOT APPLE)
  include(IUPUnixCommon)
  list(APPEND _FLTK_SOURCES ${IUP_UNIX_COMMON_SOURCES})
  list(APPEND _FLTK_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_sni.c")
elseif(APPLE)
  list(APPEND _FLTK_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_info.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_help.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_thread.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_tray.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_notify.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_singleinstance.m"
  )
  set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_help.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_thread.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_tray.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_notify.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_singleinstance.m"
    PROPERTIES LANGUAGE OBJC
  )
  list(APPEND IUP_DRIVER_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa")
elseif(WIN32)
  list(APPEND _FLTK_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwin_thread.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwin_tray.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwin_notify.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwin_singleinstance.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwindows_info.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwindows_help.c"
  )
  list(APPEND IUP_DRIVER_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/win")
endif()

set(IUP_DRIVER_SOURCES ${_FLTK_SOURCES})

set(IUP_DRIVER_COMPILE_DEFINITIONS IUP_USE_FLTK)

set(IUP_DRIVER_COMPILE_OPTIONS "")

set(IUP_DRIVER_LINK_LIBRARIES fltk fltk_images)

if(UNIX AND NOT APPLE)
  list(APPEND IUP_DRIVER_COMPILE_DEFINITIONS IUPDBUS_USE_DLOPEN IUPX11_USE_DLOPEN)
  list(APPEND IUP_DRIVER_LINK_LIBRARIES dl m pthread resolv)
elseif(APPLE)
  list(APPEND IUP_DRIVER_LINK_LIBRARIES
    "-framework AppKit"
    "-framework QuartzCore"
    "-framework SystemConfiguration"
    "-framework UserNotifications"
    pthread resolv
  )
elseif(WIN32)
  list(APPEND IUP_DRIVER_LINK_LIBRARIES
    gdi32 comdlg32 ole32
  )
endif()

set(IUP_PC_REQUIRES "")
set(IUP_PC_LIBS_PRIVATE "-lfltk -lfltk_images")
if(UNIX AND NOT APPLE)
  string(APPEND IUP_PC_LIBS_PRIVATE " -ldl -lm -lpthread -lresolv")
elseif(APPLE)
  string(APPEND IUP_PC_LIBS_PRIVATE " -framework AppKit -framework QuartzCore -framework SystemConfiguration -framework UserNotifications -lpthread -lresolv")
elseif(WIN32)
  string(APPEND IUP_PC_LIBS_PRIVATE " -lgdi32 -lcomdlg32 -lole32")
endif()
