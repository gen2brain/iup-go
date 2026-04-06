# Cocoa / macOS driver

file(GLOB _COCOA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/iupcocoa_*.m")
list(APPEND _COCOA_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa/IupCocoaTabBarView.m"
)

set_source_files_properties(${_COCOA_SOURCES} PROPERTIES LANGUAGE OBJC)

set(IUP_DRIVER_SOURCES ${_COCOA_SOURCES})

set(IUP_DRIVER_COMPILE_DEFINITIONS "")

set(IUP_DRIVER_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa")

set(IUP_DRIVER_LINK_LIBRARIES
  "-framework AppKit"
  "-framework QuartzCore"
  "-framework SystemConfiguration"
  "-framework UserNotifications"
)

set(IUP_DRIVER_COMPILE_OPTIONS "")
set(IUP_DRIVER_LINK_OPTIONS -fuse-ld=lld)

set(IUP_PC_REQUIRES "")
set(IUP_PC_LIBS_PRIVATE "-framework AppKit -framework QuartzCore -framework SystemConfiguration -framework UserNotifications")
