# Haiku / BeAPI driver

file(GLOB _HAIKU_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/haiku/iuphaiku_*.cpp")

set(IUP_DRIVER_SOURCES ${_HAIKU_SOURCES})

set(IUP_DRIVER_COMPILE_DEFINITIONS IUP_USE_HAIKU)

set(IUP_DRIVER_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/haiku")

# BCalendarView / BDateTimeEdit / BColumnListView live under private/shared|interface.
if(CMAKE_CROSSCOMPILING)
  execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} -print-sysroot
    OUTPUT_VARIABLE _HAIKU_SYSROOT
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()
set(_HAIKU_PRIV "${_HAIKU_SYSROOT}/boot/system/develop/headers/private")
list(APPEND IUP_DRIVER_INCLUDE_DIRS
  "${_HAIKU_PRIV}/shared"
  "${_HAIKU_PRIV}/interface"
)

if(IUP_BUILD_WEB)
  list(APPEND IUP_DRIVER_INCLUDE_DIRS "${_HAIKU_PRIV}/netservices")
endif()

set(IUP_DRIVER_LINK_LIBRARIES
  be
  tracker
  translation
  shared
  columnlistview
)

set(IUP_DRIVER_COMPILE_OPTIONS -Wno-odr)
set(IUP_DRIVER_LINK_OPTIONS "")

set(IUP_PC_REQUIRES "")
set(IUP_PC_LIBS_PRIVATE "-lbe -ltracker -ltranslation -lshared -lcolumnlistview")
