# iupmedia - Audio player

set(_MEDIA_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/srcmedia/iup_media.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/srcmedia/iup_audio.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/srcmedia/iup_miniaudio.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/srcmedia/iup_camera.c"
)
set(_MEDIA_LIBS "")
set(_MEDIA_DEFS "")

if(WIN32)
  list(APPEND _MEDIA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/srcmedia/iupwin_camera.cpp")
  set(_MEDIA_LIBS mfuuid ole32)

elseif(APPLE AND IUP_BACKEND STREQUAL "cocoatouch")
  list(APPEND _MEDIA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/srcmedia/iupapple_camera.m")
  set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/srcmedia/iup_miniaudio.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/srcmedia/iupapple_camera.m"
    PROPERTIES LANGUAGE OBJC
  )
  set(_MEDIA_LIBS "-framework CoreFoundation" "-framework CoreAudio" "-framework AudioToolbox" "-framework AVFoundation" "-framework CoreMedia" "-framework CoreVideo")

elseif(APPLE)
  list(APPEND _MEDIA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/srcmedia/iupapple_camera.m")
  set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/srcmedia/iupapple_camera.m"
    PROPERTIES LANGUAGE OBJC
  )
  set(_MEDIA_LIBS "-framework CoreFoundation" "-framework CoreAudio" "-framework AudioToolbox" "-framework AVFoundation" "-framework CoreMedia" "-framework CoreVideo")

elseif(IUP_BACKEND STREQUAL "android")
  list(APPEND _MEDIA_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/srcmedia/iupandroid_camera.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/srcmedia/iupandroid_camera_jni.c"
  )
  set(_MEDIA_LIBS android)

elseif(IUP_BACKEND STREQUAL "haiku" OR EMSCRIPTEN)
  list(APPEND _MEDIA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/srcmedia/iup_camera_none.c")

else()
  list(APPEND _MEDIA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/srcmedia/iupunix_camera.c")
  find_package(Threads REQUIRED)
  set(_MEDIA_LIBS Threads::Threads ${CMAKE_DL_LIBS} m)
endif()

if(IUP_BUILD_FRAMEWORK)
  target_sources(iup PRIVATE ${_MEDIA_SOURCES})
  target_compile_definitions(iup PRIVATE IUPMEDIA_BUILD_LIBRARY ${_MEDIA_DEFS})
  target_include_directories(iup PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/srcmedia)
  target_include_directories(iup SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/srcmedia/bundled)
  target_link_libraries(iup PRIVATE ${_MEDIA_LIBS})
  return()
endif()

add_library(iupmedia ${_MEDIA_SOURCES})
add_library(IUP::iupmedia ALIAS iupmedia)

target_include_directories(iupmedia
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/iup>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/srcmedia
    ${IUP_DRIVER_INCLUDE_DIRS}
)
target_include_directories(iupmedia SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/srcmedia/bundled)

target_compile_definitions(iupmedia PRIVATE IUPMEDIA_BUILD_LIBRARY ${_MEDIA_DEFS} ${IUP_DRIVER_COMPILE_DEFINITIONS})
target_link_libraries(iupmedia PUBLIC IUP::iup PRIVATE ${_MEDIA_LIBS} ${IUP_DRIVER_LINK_LIBRARIES})

set_target_properties(iupmedia PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  POSITION_INDEPENDENT_CODE ON
)

get_target_property(_iupmedia_type iupmedia TYPE)
if(_iupmedia_type STREQUAL "SHARED_LIBRARY")
  set_target_properties(iupmedia PROPERTIES
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
  )
endif()

# pkg-config
set(IUPMEDIA_PC_LIBS_PRIVATE "")
if(WIN32)
  set(IUPMEDIA_PC_LIBS_PRIVATE "-lmfuuid -lole32")
elseif(APPLE)
  set(IUPMEDIA_PC_LIBS_PRIVATE "-framework CoreFoundation -framework CoreAudio -framework AudioToolbox -framework AVFoundation -framework CoreMedia -framework CoreVideo")
elseif(NOT WIN32 AND NOT IUP_BACKEND STREQUAL "android" AND NOT IUP_BACKEND STREQUAL "haiku" AND NOT EMSCRIPTEN)
  set(IUPMEDIA_PC_LIBS_PRIVATE "-lpthread -ldl -lm")
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/iupmedia.pc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/iupmedia.pc"
  @ONLY
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/iupmedia.pc"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
