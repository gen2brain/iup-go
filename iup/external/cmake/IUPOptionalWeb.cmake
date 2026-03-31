# iupweb - WebBrowser control

set(_WEB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/srcweb/iup_webbrowser.c")
set(_WEB_LIBS "")
set(_WEB_DEFS "")

if(IUP_BACKEND STREQUAL "gtk3" OR IUP_BACKEND STREQUAL "gtk4" OR IUP_BACKEND STREQUAL "gtk2")
  list(APPEND _WEB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/srcweb/iupgtk_webbrowser.c")
  list(APPEND _WEB_DEFS IUPWEB_USE_DLOPEN)

elseif(IUP_BACKEND STREQUAL "cocoa")
  list(APPEND _WEB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/srcweb/iupcocoa_webbrowser.m")
  set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/srcweb/iupcocoa_webbrowser.m"
    PROPERTIES LANGUAGE OBJC
  )
  list(APPEND _WEB_LIBS "-framework WebKit")

elseif(IUP_BACKEND STREQUAL "win32" OR IUP_BACKEND STREQUAL "winui")
  list(APPEND _WEB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/srcweb/iupwin_webbrowser.cpp")

elseif(IUP_BACKEND MATCHES "^qt")
  list(APPEND _WEB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/srcweb/iupqt_webbrowser.cpp")
  if(IUP_BACKEND STREQUAL "qt6")
    find_package(Qt6 REQUIRED COMPONENTS WebEngineCore WebEngineWidgets)
    list(APPEND _WEB_LIBS Qt6::WebEngineCore Qt6::WebEngineWidgets)
  else()
    find_package(Qt5 REQUIRED COMPONENTS WebEngineCore WebEngineWidgets)
    list(APPEND _WEB_LIBS Qt5::WebEngineCore Qt5::WebEngineWidgets)
  endif()
endif()

add_library(iupweb ${_WEB_SOURCES})
add_library(IUP::iupweb ALIAS iupweb)

target_include_directories(iupweb
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/srcweb
    ${IUP_DRIVER_INCLUDE_DIRS}
)

target_compile_definitions(iupweb PRIVATE IUPWEB_BUILD_LIBRARY ${_WEB_DEFS} ${IUP_DRIVER_COMPILE_DEFINITIONS})
target_link_libraries(iupweb PUBLIC IUP::iup PRIVATE ${_WEB_LIBS} ${IUP_DRIVER_LINK_LIBRARIES})

set_target_properties(iupweb PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  POSITION_INDEPENDENT_CODE ON
)

get_target_property(_iupweb_type iupweb TYPE)
if(_iupweb_type STREQUAL "SHARED_LIBRARY")
  set_target_properties(iupweb PROPERTIES
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
  )
endif()

# pkg-config
set(IUPWEB_PC_REQUIRES "")
set(IUPWEB_PC_LIBS_PRIVATE "")
if(IUP_BACKEND STREQUAL "cocoa")
  set(IUPWEB_PC_LIBS_PRIVATE "-framework WebKit")
elseif(IUP_BACKEND STREQUAL "qt6")
  set(IUPWEB_PC_REQUIRES "Qt6WebEngineCore Qt6WebEngineWidgets")
elseif(IUP_BACKEND STREQUAL "qt5")
  set(IUPWEB_PC_REQUIRES "Qt5WebEngineCore Qt5WebEngineWidgets")
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/iupweb.pc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/iupweb.pc"
  @ONLY
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/iupweb.pc"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
