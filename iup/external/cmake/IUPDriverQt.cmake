# Qt6 / Qt5 driver

if(IUP_BACKEND STREQUAL "qt6")
  find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
  set(_QT_LIBS Qt6::Core Qt6::Gui Qt6::Widgets)
else()
  find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)
  set(_QT_LIBS Qt5::Core Qt5::Gui Qt5::Widgets)
endif()

file(GLOB _QT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/qt/iupqt_*.cpp")

set(IUP_DRIVER_COMPILE_DEFINITIONS IUP_USE_QT)
set(IUP_DRIVER_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/qt")
set(IUP_DRIVER_LINK_LIBRARIES ${_QT_LIBS})

if(UNIX AND NOT APPLE)
  include(IUPUnixCommon)
  list(APPEND _QT_SOURCES ${IUP_UNIX_COMMON_SOURCES})
  list(APPEND _QT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_sni.c")
  list(APPEND IUP_DRIVER_COMPILE_DEFINITIONS IUPDBUS_USE_DLOPEN IUPX11_USE_DLOPEN)
  list(APPEND IUP_DRIVER_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/unix")
  list(APPEND IUP_DRIVER_LINK_LIBRARIES dl m)
elseif(APPLE)
  list(APPEND _QT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_tray.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_notify.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_singleinstance.m"
  )
  set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_tray.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_notify.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_singleinstance.m"
    PROPERTIES LANGUAGE OBJC
  )
  list(APPEND IUP_DRIVER_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa")
elseif(WIN32)
  list(APPEND _QT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwin_tray.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwin_notify.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/iupwin_singleinstance.c"
  )
  list(APPEND IUP_DRIVER_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/win")
endif()

set(IUP_DRIVER_SOURCES ${_QT_SOURCES})
set(IUP_DRIVER_COMPILE_OPTIONS "")

if(IUP_BACKEND STREQUAL "qt6")
  set(IUP_PC_REQUIRES "Qt6Core Qt6Gui Qt6Widgets")
else()
  set(IUP_PC_REQUIRES "Qt5Core Qt5Gui Qt5Widgets")
endif()
set(IUP_PC_LIBS_PRIVATE "")
if(UNIX AND NOT APPLE)
  set(IUP_PC_LIBS_PRIVATE "-ldl -lm")
elseif(APPLE)
  list(APPEND IUP_DRIVER_LINK_LIBRARIES
    "-framework AppKit"
    "-framework SystemConfiguration"
    "-framework UserNotifications"
  )
endif()
