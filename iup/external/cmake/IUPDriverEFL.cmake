# EFL / Elementary driver

find_package(PkgConfig REQUIRED)

pkg_check_modules(ELEMENTARY REQUIRED IMPORTED_TARGET elementary)

file(GLOB _EFL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/efl/iupefl_*.c")


set(IUP_DRIVER_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/efl"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/unix"
)

if(UNIX AND NOT APPLE)
  include(IUPUnixCommon)
  list(APPEND _EFL_SOURCES ${IUP_UNIX_COMMON_SOURCES})
  list(APPEND _EFL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_sni.c")
elseif(APPLE)
  list(APPEND _EFL_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_info.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_help.m"
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
  list(APPEND _EFL_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwin_tray.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwin_notify.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwin_singleinstance.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwindows_info.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwindows_help.c"
  )
  list(APPEND IUP_DRIVER_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/win")
endif()

set(IUP_DRIVER_SOURCES ${_EFL_SOURCES})

set(IUP_DRIVER_COMPILE_DEFINITIONS
  IUP_USE_EFL
  EFL_BETA_API_SUPPORT=1
  EFL_EO_API_SUPPORT=1
)

set(IUP_DRIVER_COMPILE_OPTIONS "")

set(IUP_DRIVER_LINK_LIBRARIES PkgConfig::ELEMENTARY)

if(UNIX AND NOT APPLE)
  list(APPEND IUP_DRIVER_COMPILE_DEFINITIONS IUPDBUS_USE_DLOPEN IUPX11_USE_DLOPEN)
  list(APPEND IUP_DRIVER_LINK_LIBRARIES dl m)

  pkg_check_modules(ECORE_X IMPORTED_TARGET ecore-x)
  pkg_check_modules(ECORE_WL2 IMPORTED_TARGET ecore-wl2)
  if(ECORE_X_FOUND)
    list(APPEND IUP_DRIVER_COMPILE_DEFINITIONS HAVE_ECORE_X)
    list(APPEND IUP_DRIVER_LINK_LIBRARIES PkgConfig::ECORE_X)
  endif()
  if(ECORE_WL2_FOUND)
    list(APPEND IUP_DRIVER_COMPILE_DEFINITIONS HAVE_ECORE_WL2)
    list(APPEND IUP_DRIVER_LINK_LIBRARIES PkgConfig::ECORE_WL2)
  endif()
endif()

if(WIN32)
  pkg_check_modules(ECORE_WIN32 IMPORTED_TARGET ecore-win32)
  if(ECORE_WIN32_FOUND)
    list(APPEND IUP_DRIVER_COMPILE_DEFINITIONS HAVE_ECORE_WIN32)
    list(APPEND IUP_DRIVER_LINK_LIBRARIES PkgConfig::ECORE_WIN32)
  endif()
endif()

if(APPLE)
  pkg_check_modules(ECORE_COCOA IMPORTED_TARGET ecore-cocoa)
  if(ECORE_COCOA_FOUND)
    list(APPEND IUP_DRIVER_COMPILE_DEFINITIONS HAVE_ECORE_COCOA)
    list(APPEND IUP_DRIVER_LINK_LIBRARIES PkgConfig::ECORE_COCOA)
  endif()
  list(APPEND IUP_DRIVER_LINK_LIBRARIES
    "-framework AppKit"
    "-framework SystemConfiguration"
    "-framework UserNotifications"
  )
endif()

set(IUP_PC_REQUIRES "elementary")
if(ECORE_X_FOUND)
  string(APPEND IUP_PC_REQUIRES " ecore-x")
endif()
if(ECORE_WL2_FOUND)
  string(APPEND IUP_PC_REQUIRES " ecore-wl2")
endif()
if(ECORE_WIN32_FOUND)
  string(APPEND IUP_PC_REQUIRES " ecore-win32")
endif()
if(ECORE_COCOA_FOUND)
  string(APPEND IUP_PC_REQUIRES " ecore-cocoa")
endif()
set(IUP_PC_LIBS_PRIVATE "")
if(UNIX AND NOT APPLE)
  set(IUP_PC_LIBS_PRIVATE "-ldl -lm")
endif()
