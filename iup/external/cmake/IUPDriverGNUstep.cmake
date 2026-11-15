# GNUstep driver (the Cocoa driver on Linux and BSD)

if(NOT CMAKE_OBJC_COMPILER_ID STREQUAL "Clang")
  message(FATAL_ERROR "IUP_BACKEND=gnustep requires clang as the Objective-C compiler (CMAKE_OBJC_COMPILER)")
endif()

find_program(GNUSTEP_CONFIG gnustep-config REQUIRED)

execute_process(COMMAND ${GNUSTEP_CONFIG} --objc-flags OUTPUT_VARIABLE _GS_OBJC_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${GNUSTEP_CONFIG} --gui-libs OUTPUT_VARIABLE _GS_GUI_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
separate_arguments(_GS_OBJC_FLAGS UNIX_COMMAND "${_GS_OBJC_FLAGS}")
separate_arguments(_GS_GUI_LIBS UNIX_COMMAND "${_GS_GUI_LIBS}")

set(_GS_DEFS "")
set(_GS_INCLUDES "")
set(_GS_OBJC_OPTIONS "")
foreach(_flag IN LISTS _GS_OBJC_FLAGS)
  if(_flag MATCHES "^-D(.+)")
    list(APPEND _GS_DEFS "${CMAKE_MATCH_1}")
  elseif(_flag MATCHES "^-I(.+)" AND NOT _flag STREQUAL "-I.")
    list(APPEND _GS_INCLUDES "${CMAKE_MATCH_1}")
  elseif(_flag MATCHES "^-f(objc-runtime|constant-string-class)=" OR _flag STREQUAL "-fblocks" OR _flag STREQUAL "-fno-strict-aliasing")
    list(APPEND _GS_OBJC_OPTIONS "${_flag}")
  endif()
endforeach()
list(REMOVE_DUPLICATES _GS_DEFS)

set(_GS_LIBS "")
set(_GS_LINK_OPTIONS "")
foreach(_flag IN LISTS _GS_GUI_LIBS)
  if(_flag MATCHES "^-l" OR _flag MATCHES "^-L")
    list(APPEND _GS_LIBS "${_flag}")
  elseif(_flag MATCHES "^-fobjc-runtime=" OR _flag STREQUAL "-fblocks")
    list(APPEND _GS_LINK_OPTIONS "${_flag}")
  endif()
endforeach()

list(JOIN _GS_OBJC_OPTIONS " " _GS_OBJC_OPTIONS_STR)
string(APPEND CMAKE_OBJC_FLAGS " ${_GS_OBJC_OPTIONS_STR} -Wno-import -Wno-protocol")

file(GLOB _GS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_*.m")
list(FILTER _GS_SOURCES EXCLUDE REGEX "iupcocoa_(fdwatch|help|keepawake|location|notify|sensor|sharedlg|singleinstance|tray)\\.m$")
list(APPEND _GS_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/IupCocoaTabBarView.m"
)
set_source_files_properties(${_GS_SOURCES} PROPERTIES LANGUAGE OBJC)

list(APPEND _GS_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_fdwatch.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_help.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_keepawake.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_location.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_notify.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_portal.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_sensor.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_sharedlg.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_singleinstance.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_sni.c"
)

set(IUP_DRIVER_SOURCES ${_GS_SOURCES})

set(IUP_DRIVER_COMPILE_DEFINITIONS
  IUP_USE_COCOA
  GNUSTEP
  IUPDBUS_USE_DLOPEN
  IUPX11_USE_DLOPEN
  ${_GS_DEFS}
)

set(IUP_DRIVER_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/unix"
  ${_GS_INCLUDES}
)

set(IUP_DRIVER_LINK_LIBRARIES ${_GS_LIBS} dispatch opal gnustep-corebase ${CMAKE_DL_LIBS} m)

set(IUP_DRIVER_COMPILE_OPTIONS "")
set(IUP_DRIVER_LINK_OPTIONS ${_GS_LINK_OPTIONS})

set(IUP_PC_REQUIRES "")
list(JOIN _GS_LIBS " " IUP_PC_LIBS_PRIVATE)
string(APPEND IUP_PC_LIBS_PRIVATE " -ldispatch -lopal -lgnustep-corebase -ldl -lm")
