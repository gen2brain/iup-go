cmake_minimum_required(VERSION 3.20)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" _IUP_VERSION_RAW)
string(STRIP "${_IUP_VERSION_RAW}" _IUP_VERSION_RAW)
set(_IUP_VERSION "${_IUP_VERSION_RAW}")

project(IUP
  VERSION ${_IUP_VERSION}
  LANGUAGES C CXX
  DESCRIPTION "IUP - Portable User Interface"
)

# --- Build type ---

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    Debug Release RelWithDebInfo MinSizeRel)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GNUInstallDirs)

# --- LTO for Release builds ---

include(CheckIPOSupported)
check_ipo_supported(RESULT _ipo_supported)
if(_ipo_supported)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON)
endif()

# --- Strip installed binaries for Release builds ---

if(CMAKE_BUILD_TYPE MATCHES "^(Release|MinSizeRel)$")
  set(CMAKE_INSTALL_DO_STRIP TRUE)
endif()

# --- Options ---

if(APPLE)
  set(_IUP_BACKEND_DEFAULT "cocoa")
elseif(WIN32)
  set(_IUP_BACKEND_DEFAULT "win32")
else()
  set(_IUP_BACKEND_DEFAULT "gtk3")
endif()

set(IUP_BACKEND "${_IUP_BACKEND_DEFAULT}" CACHE STRING
  "GUI backend: gtk3, gtk4, gtk2, win32, winui, cocoa, motif, qt6, qt5, efl, fltk")
set_property(CACHE IUP_BACKEND PROPERTY STRINGS
  gtk3 gtk4 gtk2 win32 winui cocoa motif qt6 qt5 efl fltk)

option(IUP_BUILD_GL       "Build iupgl (OpenGL canvas)"     OFF)
option(IUP_BUILD_WEB      "Build iupweb (Web browser)"      OFF)
option(IUP_BUILD_PLOT     "Build iupplot (Plot control)"    OFF)
option(IUP_BUILD_CTRL     "Build iupctrl (Matrix/Controls)" OFF)
option(IUP_BUILD_EXAMPLES "Build example programs"          OFF)
option(IUP_USE_XEMBED     "Use XEmbed tray protocol instead of SNI (GTK3/Motif on Linux)" OFF)

if(APPLE)
  enable_language(OBJC)
endif()

# --- Core sources (platform-independent) ---

file(GLOB IUP_CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/iup_*.c")
list(APPEND IUP_CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/iup.c")

# iup_datepick.c is a core fallback, Win32 and Qt drivers provide their own
if(IUP_BACKEND STREQUAL "win32" OR IUP_BACKEND MATCHES "^qt")
  list(REMOVE_ITEM IUP_CORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/iup_datepick.c"
  )
endif()

# --- Driver ---

set(IUP_DRIVER_SOURCES "")
set(IUP_DRIVER_COMPILE_DEFINITIONS "")
set(IUP_DRIVER_INCLUDE_DIRS "")
set(IUP_DRIVER_LINK_LIBRARIES "")
set(IUP_DRIVER_COMPILE_OPTIONS "")

if(IUP_BACKEND STREQUAL "gtk3" OR IUP_BACKEND STREQUAL "gtk2")
  include(IUPDriverGTK)
elseif(IUP_BACKEND STREQUAL "gtk4")
  include(IUPDriverGTK4)
elseif(IUP_BACKEND STREQUAL "win32")
  include(IUPDriverWin32)
elseif(IUP_BACKEND STREQUAL "winui")
  include(IUPDriverWinUI)
elseif(IUP_BACKEND STREQUAL "cocoa")
  include(IUPDriverCocoa)
elseif(IUP_BACKEND STREQUAL "motif")
  include(IUPDriverMotif)
elseif(IUP_BACKEND STREQUAL "qt6" OR IUP_BACKEND STREQUAL "qt5")
  include(IUPDriverQt)
elseif(IUP_BACKEND STREQUAL "efl")
  include(IUPDriverEFL)
elseif(IUP_BACKEND STREQUAL "fltk")
  include(IUPDriverFLTK)
else()
  message(FATAL_ERROR "Unknown IUP_BACKEND: ${IUP_BACKEND}")
endif()

# --- Build iup library ---

add_library(iup
  ${IUP_CORE_SOURCES}
  ${IUP_DRIVER_SOURCES}
)
add_library(IUP::iup ALIAS iup)

target_include_directories(iup
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${IUP_DRIVER_INCLUDE_DIRS}
)

target_compile_definitions(iup
  PRIVATE
    IUP_BUILD_LIBRARY
    ${IUP_DRIVER_COMPILE_DEFINITIONS}
)

if(IUP_DRIVER_COMPILE_OPTIONS)
  target_compile_options(iup PRIVATE ${IUP_DRIVER_COMPILE_OPTIONS})
endif()

target_link_libraries(iup PRIVATE ${IUP_DRIVER_LINK_LIBRARIES})

set_target_properties(iup PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  POSITION_INDEPENDENT_CODE ON
)

get_target_property(_iup_type iup TYPE)
if(_iup_type STREQUAL "SHARED_LIBRARY")
  set_target_properties(iup PROPERTIES
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
  )
endif()

# Suppress false -Wodr from C _IcontrolData pattern when C++ LTO merges TUs.
if((IUP_BACKEND MATCHES "^qt" OR IUP_BACKEND STREQUAL "fltk") AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_link_options(iup PUBLIC -Wno-odr)
endif()

# --- Optional libraries ---

if(IUP_BUILD_GL)
  include(IUPOptionalGL)
endif()

if(IUP_BUILD_WEB AND NOT IUP_BACKEND STREQUAL "efl" AND NOT IUP_BACKEND STREQUAL "motif" AND NOT IUP_BACKEND STREQUAL "fltk")
  include(IUPOptionalWeb)
endif()

if(IUP_BUILD_CTRL)
  include(IUPOptionalCtrl)
endif()

if(IUP_BUILD_PLOT)
  include(IUPOptionalPlot)
endif()

# --- Examples ---

if(IUP_BUILD_EXAMPLES)
  add_executable(dialog_hello "${CMAKE_CURRENT_SOURCE_DIR}/examples/dialog_hello.c")
  target_link_libraries(dialog_hello PRIVATE IUP::iup)

  add_executable(dialog_hello_plus "${CMAKE_CURRENT_SOURCE_DIR}/examples/dialog_hello_plus.cpp")
  target_link_libraries(dialog_hello_plus PRIVATE IUP::iup)
endif()

# --- Install ---

include(CMakePackageConfigHelpers)

set(_IUP_INSTALL_TARGETS iup)
foreach(_opt_target iupgl iupweb iupctrl iupplot)
  if(TARGET ${_opt_target})
    list(APPEND _IUP_INSTALL_TARGETS ${_opt_target})
  endif()
endforeach()

install(TARGETS ${_IUP_INSTALL_TARGETS}
  EXPORT IUPTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
  PATTERN "vendor.go" EXCLUDE
)

install(EXPORT IUPTargets
  FILE IUPTargets.cmake
  NAMESPACE IUP::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IUP
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/IUPConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/IUPConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IUP
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/IUPConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/IUPConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/IUPConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IUP
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/iup.pc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/iup.pc"
  @ONLY
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/iup.pc"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# --- Source package ---

set(CPACK_PACKAGE_NAME "iup")
set(CPACK_PACKAGE_VERSION "${_IUP_VERSION_RAW}")
set(CPACK_SOURCE_GENERATOR "TGZ;TXZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "iup-${_IUP_VERSION_RAW}")
set(CPACK_SOURCE_IGNORE_FILES
  "/build/"
  "/\\.git/"
  "vendor\\.go$"
)
include(CPack)
