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
# LTO breaks EM_JS on Emscripten.
if(_ipo_supported AND NOT EMSCRIPTEN)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON)
endif()

# --- Strip installed binaries for Release builds ---

if(CMAKE_BUILD_TYPE MATCHES "^(Release|MinSizeRel)$")
  set(CMAKE_INSTALL_DO_STRIP TRUE)
endif()

# --- Options ---

if(EMSCRIPTEN)
  set(_IUP_BACKEND_DEFAULT "wasm")
elseif(ANDROID)
  set(_IUP_BACKEND_DEFAULT "android")
elseif(APPLE)
  set(_IUP_BACKEND_DEFAULT "cocoa")
elseif(WIN32)
  set(_IUP_BACKEND_DEFAULT "win32")
elseif(HAIKU)
  set(_IUP_BACKEND_DEFAULT "haiku")
else()
  set(_IUP_BACKEND_DEFAULT "gtk3")
endif()

set(IUP_BACKEND "${_IUP_BACKEND_DEFAULT}" CACHE STRING
  "GUI backend: gtk3, gtk4, gtk2, win32, winui, cocoa, cocoatouch, motif, qt6, qt5, efl, fltk, android, haiku, wasm")
set_property(CACHE IUP_BACKEND PROPERTY STRINGS
  gtk3 gtk4 gtk2 win32 winui cocoa cocoatouch motif qt6 qt5 efl fltk android haiku wasm)

option(IUP_BUILD_GL        "Build iupgl (OpenGL canvas)"     OFF)
option(IUP_BUILD_WEB       "Build iupweb (Web browser)"      OFF)
option(IUP_BUILD_PLOT      "Build iupplot (Plot control)"    OFF)
option(IUP_BUILD_CTRL      "Build iupctrl (Matrix/Controls)" OFF)
option(IUP_BUILD_EXAMPLES  "Build example programs"          OFF)
option(IUP_USE_XEMBED      "Use XEmbed tray protocol instead of SNI (GTK3/Motif on Linux)" OFF)
option(IUP_BUILD_FRAMEWORK "Build iup as a .framework bundle (requires IUP_BACKEND=cocoatouch)" OFF)
option(IUP_EMBED_MANIFEST  "Embed the IUP application manifest into built executables (Windows)" ON)
set(IUP_FRAMEWORK_BUILD "1" CACHE STRING "CFBundleVersion for the framework bundle")

if(APPLE)
  enable_language(OBJC)
endif()

if(WIN32 AND IUP_EMBED_MANIFEST)
  enable_language(RC)
  set(IUP_MANIFEST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/iup.manifest")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/iup.manifest.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/iup.manifest.rc"
    @ONLY
  )
endif()

function(iup_embed_manifest tgt)
  if(NOT WIN32 OR NOT IUP_EMBED_MANIFEST)
    return()
  endif()
  target_sources(${tgt} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/iup.manifest.rc")
  if(MSVC)
    target_link_options(${tgt} PRIVATE /MANIFEST:NO)
  endif()
endfunction()

# --- Core sources (platform-independent) ---

file(GLOB IUP_CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/iup_*.c")
list(APPEND IUP_CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/iup.c")

# iup_datepick.c is a core fallback; Win32, Qt, Android, iOS and wasm provide their own
if(IUP_BACKEND STREQUAL "win32" OR IUP_BACKEND MATCHES "^qt" OR IUP_BACKEND STREQUAL "android" OR IUP_BACKEND STREQUAL "cocoatouch" OR IUP_BACKEND STREQUAL "wasm")
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
elseif(IUP_BACKEND STREQUAL "cocoatouch")
  include(IUPDriverCocoaTouch)
elseif(IUP_BACKEND STREQUAL "motif")
  include(IUPDriverMotif)
elseif(IUP_BACKEND STREQUAL "qt6" OR IUP_BACKEND STREQUAL "qt5")
  include(IUPDriverQt)
elseif(IUP_BACKEND STREQUAL "efl")
  include(IUPDriverEFL)
elseif(IUP_BACKEND STREQUAL "fltk")
  include(IUPDriverFLTK)
elseif(IUP_BACKEND STREQUAL "android")
  include(IUPDriverAndroid)
elseif(IUP_BACKEND STREQUAL "haiku")
  include(IUPDriverHaiku)
elseif(IUP_BACKEND STREQUAL "wasm")
  include(IUPDriverWasm)
else()
  message(FATAL_ERROR "Unknown IUP_BACKEND: ${IUP_BACKEND}")
endif()

# --- Build iup library ---

if(IUP_BUILD_FRAMEWORK)
  set(_IUP_LIB_TYPE SHARED)
else()
  set(_IUP_LIB_TYPE "")
endif()

add_library(iup ${_IUP_LIB_TYPE}
  ${IUP_CORE_SOURCES}
  ${IUP_DRIVER_SOURCES}
)
add_library(IUP::iup ALIAS iup)

target_include_directories(iup
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/iup>
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

if(IUP_DRIVER_LINK_OPTIONS)
  target_link_options(iup PRIVATE ${IUP_DRIVER_LINK_OPTIONS})
endif()

set_target_properties(iup PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  POSITION_INDEPENDENT_CODE ON
)

# --- iOS .framework bundle ---

if(IUP_BUILD_FRAMEWORK)
  if(NOT IUP_BACKEND STREQUAL "cocoatouch")
    message(FATAL_ERROR "IUP_BUILD_FRAMEWORK requires IUP_BACKEND=cocoatouch")
  endif()

  set(IUP_FRAMEWORK_VERSION "${PROJECT_VERSION}")
  if(CMAKE_OSX_SYSROOT MATCHES "[Ss]imulator")
    set(IUP_FRAMEWORK_PLATFORM "iPhoneSimulator")
  else()
    set(IUP_FRAMEWORK_PLATFORM "iPhoneOS")
  endif()
  if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0")
  endif()
  set(IUP_FRAMEWORK_MIN_IOS "${CMAKE_OSX_DEPLOYMENT_TARGET}")

  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ios/library/Info.plist.in"
    "${CMAKE_CURRENT_BINARY_DIR}/IUP-Info.plist"
    @ONLY
  )

  set(IUP_FRAMEWORK_PUBLIC_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/iup.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/iupcbs.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/iupcbs.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/iupdraw.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/iupkey.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/iupplus.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/iup_config.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/iup_export.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/iup_varg.h"
  )
  if(IUP_BUILD_GL)
    list(APPEND IUP_FRAMEWORK_PUBLIC_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/iupgl.h")
  endif()
  if(IUP_BUILD_WEB)
    list(APPEND IUP_FRAMEWORK_PUBLIC_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/iupweb.h")
  endif()
  if(IUP_BUILD_CTRL)
    list(APPEND IUP_FRAMEWORK_PUBLIC_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/iupcontrols.h")
  endif()
  if(IUP_BUILD_PLOT)
    list(APPEND IUP_FRAMEWORK_PUBLIC_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/iupplot.h")
  endif()

  set_target_properties(iup PROPERTIES
    FRAMEWORK TRUE
    FRAMEWORK_VERSION A
    OUTPUT_NAME IUP
    MACOSX_FRAMEWORK_IDENTIFIER io.github.gen2brain.iup
    MACOSX_FRAMEWORK_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/IUP-Info.plist"
    PUBLIC_HEADER "${IUP_FRAMEWORK_PUBLIC_HEADERS}"
    XCODE_ATTRIBUTE_DEFINES_MODULE YES
  )

  add_custom_command(TARGET iup POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:iup>/Modules
    COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_CURRENT_SOURCE_DIR}/ios/library/module.modulemap"
      $<TARGET_FILE_DIR:iup>/Modules/module.modulemap
  )
endif()

get_target_property(_iup_type iup TYPE)
if(_iup_type STREQUAL "SHARED_LIBRARY")
  set_target_properties(iup PROPERTIES
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
  )
endif()

if(EMSCRIPTEN)
  target_link_options(iup INTERFACE
    "-sEMULATE_FUNCTION_POINTER_CASTS=1"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sNO_EXIT_RUNTIME=1"
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

# Framework mode is consumed via build-framework.sh -> xcodebuild -create-xcframework, not cmake --install.
if(IUP_BUILD_FRAMEWORK)
  return()
endif()

# --- Examples ---

if(IUP_BUILD_EXAMPLES AND EMSCRIPTEN)
  message(STATUS "IUP_BUILD_EXAMPLES ignored on wasm; use wasm/build-wasm.sh to build a runnable app.")
elseif(IUP_BUILD_EXAMPLES)
  add_executable(dialog_hello "${CMAKE_CURRENT_SOURCE_DIR}/examples/dialog_hello.c")
  target_link_libraries(dialog_hello PRIVATE IUP::iup)
  iup_embed_manifest(dialog_hello)

  add_executable(dialog_hello_plus "${CMAKE_CURRENT_SOURCE_DIR}/examples/dialog_hello_plus.cpp")
  target_link_libraries(dialog_hello_plus PRIVATE IUP::iup)
  iup_embed_manifest(dialog_hello_plus)
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
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/iup
  FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
  PATTERN "vendor.go" EXCLUDE
)

if(WIN32)
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/iup.manifest"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/iup
  )
endif()

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
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "iup-${_IUP_VERSION_RAW}")
set(CPACK_SOURCE_IGNORE_FILES
  "/build/"
  "/\\.git/"
  "vendor\\.go$"
)
include(CPack)

# --- Docs package ---
# Standalone config because docs/ lives outside the CMake source tree.
get_filename_component(_IUP_DOCS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../docs" ABSOLUTE)
if(EXISTS "${_IUP_DOCS_DIR}")
  file(CONFIGURE OUTPUT "${CMAKE_BINARY_DIR}/CPackDocsConfig.cmake" @ONLY CONTENT [[
set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_NAME "iup-docs")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "IUP API reference documentation")
set(CPACK_PACKAGE_DESCRIPTION "IUP API reference documentation.")
set(CPACK_PACKAGE_VERSION "@_IUP_VERSION_RAW@")
set(CPACK_PACKAGE_FILE_NAME "iup-docs-@_IUP_VERSION_RAW@")
set(CPACK_PACKAGE_DIRECTORY "@CMAKE_BINARY_DIR@")
set(CPACK_INSTALLED_DIRECTORIES "@_IUP_DOCS_DIR@;.")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 1)
]])
endif()
