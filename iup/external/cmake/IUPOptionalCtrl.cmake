# iupctrl - Matrix and advanced controls

file(GLOB _CTRL_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/srcctrl/*.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/srcctrl/matrix/*.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/srcctrl/matrixex/*.c"
)

add_library(iupctrl ${_CTRL_SOURCES})
add_library(IUP::iupctrl ALIAS iupctrl)

target_include_directories(iupctrl
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/srcctrl
    ${CMAKE_CURRENT_SOURCE_DIR}/srcctrl/matrix
    ${CMAKE_CURRENT_SOURCE_DIR}/srcctrl/matrixex
)

target_compile_definitions(iupctrl PRIVATE IUPCONTROLS_BUILD_LIBRARY)
target_link_libraries(iupctrl PUBLIC IUP::iup)

set_target_properties(iupctrl PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  POSITION_INDEPENDENT_CODE ON
)

get_target_property(_iupctrl_type iupctrl TYPE)
if(_iupctrl_type STREQUAL "SHARED_LIBRARY")
  set_target_properties(iupctrl PROPERTIES
    C_VISIBILITY_PRESET hidden
  )
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/iupctrl.pc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/iupctrl.pc"
  @ONLY
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/iupctrl.pc"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
