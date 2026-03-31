# iupplot - Plot control

file(GLOB _PLOT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/srcplot/*.cpp")

add_library(iupplot ${_PLOT_SOURCES})
add_library(IUP::iupplot ALIAS iupplot)

target_include_directories(iupplot
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/srcplot
)

target_compile_definitions(iupplot PRIVATE IUPPLOT_BUILD_LIBRARY)
target_link_libraries(iupplot PUBLIC IUP::iup)

set_target_properties(iupplot PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  POSITION_INDEPENDENT_CODE ON
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
)

get_target_property(_iupplot_type iupplot TYPE)
if(_iupplot_type STREQUAL "SHARED_LIBRARY")
  set_target_properties(iupplot PROPERTIES
    CXX_VISIBILITY_PRESET hidden
  )
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/iupplot.pc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/iupplot.pc"
  @ONLY
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/iupplot.pc"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
