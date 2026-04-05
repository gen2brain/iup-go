# iupgl - OpenGL canvas support

set(_GL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/srcgl/iup_glcanvas.c")
set(_GL_LIBS "")
set(_GL_DEFS "")

if(WIN32)
  list(APPEND _GL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/srcgl/iup_glcanvas_win.c")
  set(_GL_LIBS opengl32)

elseif(APPLE)
  list(APPEND _GL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/srcgl/iup_glcanvas_cocoa.m")
  set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/srcgl/iup_glcanvas_cocoa.m"
    PROPERTIES LANGUAGE OBJC
  )
  set(_GL_LIBS "-framework OpenGL")

elseif(IUP_BACKEND STREQUAL "gtk2" OR IUP_BACKEND STREQUAL "motif")
  list(APPEND _GL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/srcgl/iup_glcanvas_x.c")
  find_package(OpenGL REQUIRED)
  set(_GL_LIBS OpenGL::GL)

else()
  # EGL path: GTK3, GTK4, Qt, EFL (Unix/Linux only)
  # iup_glcanvas_egl.c is a unity file that #includes the correct backend .c
  list(APPEND _GL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/srcgl/iup_glcanvas_egl.c")

  if(IUP_BACKEND MATCHES "^qt")
    if(IUP_BACKEND STREQUAL "qt6")
      find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)
      set(_GL_QT_PRIVATE Qt6::GuiPrivate)
    else()
      find_package(Qt5 REQUIRED COMPONENTS Gui)
      set(_GL_QT_PRIVATE Qt5::GuiPrivate)
    endif()
  endif()

  find_package(PkgConfig REQUIRED)
  pkg_check_modules(WAYLAND_EGL IMPORTED_TARGET wayland-egl)
  find_package(OpenGL REQUIRED COMPONENTS EGL)

  set(_GL_LIBS OpenGL::GL OpenGL::EGL)
  if(WAYLAND_EGL_FOUND)
    list(APPEND _GL_LIBS PkgConfig::WAYLAND_EGL)
  endif()
endif()

add_library(iupgl ${_GL_SOURCES})
add_library(IUP::iupgl ALIAS iupgl)

target_include_directories(iupgl
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/srcgl
    ${IUP_DRIVER_INCLUDE_DIRS}
)

target_compile_definitions(iupgl PRIVATE IUPGL_BUILD_LIBRARY ${_GL_DEFS} ${IUP_DRIVER_COMPILE_DEFINITIONS})
target_link_libraries(iupgl PUBLIC IUP::iup PRIVATE ${_GL_LIBS} ${IUP_DRIVER_LINK_LIBRARIES})

if(_GL_QT_PRIVATE)
  target_link_libraries(iupgl PRIVATE ${_GL_QT_PRIVATE})
endif()

set_target_properties(iupgl PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  POSITION_INDEPENDENT_CODE ON
)

get_target_property(_iupgl_type iupgl TYPE)
if(_iupgl_type STREQUAL "SHARED_LIBRARY")
  set_target_properties(iupgl PROPERTIES
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
  )
endif()

# pkg-config
set(IUPGL_PC_REQUIRES "")
set(IUPGL_PC_LIBS_PRIVATE "")
if(WIN32)
  set(IUPGL_PC_LIBS_PRIVATE "-lopengl32")
elseif(APPLE)
  set(IUPGL_PC_LIBS_PRIVATE "-framework OpenGL")
elseif(IUP_BACKEND STREQUAL "gtk2" OR IUP_BACKEND STREQUAL "motif")
  set(IUPGL_PC_LIBS_PRIVATE "-lGL")
else()
  set(IUPGL_PC_REQUIRES "egl gl")
  if(WAYLAND_EGL_FOUND)
    string(APPEND IUPGL_PC_REQUIRES " wayland-egl")
  endif()
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/iupgl.pc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/iupgl.pc"
  @ONLY
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/iupgl.pc"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
