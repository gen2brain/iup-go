# CocoaTouch / iOS UIKit driver

file(GLOB _COCOATOUCH_M_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoatouch/*.m")
file(GLOB _COCOATOUCH_C_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoatouch/*.c")

set_source_files_properties(${_COCOATOUCH_M_SOURCES} PROPERTIES LANGUAGE OBJC)

set(IUP_DRIVER_SOURCES ${_COCOATOUCH_M_SOURCES} ${_COCOATOUCH_C_SOURCES})

set(IUP_DRIVER_COMPILE_DEFINITIONS "IUP_USE_COCOATOUCH")

set(IUP_DRIVER_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/cocoatouch")

set(IUP_DRIVER_LINK_LIBRARIES
  "-framework Foundation"
  "-framework UIKit"
  "-framework CoreGraphics"
  "-framework CoreText"
  "-framework QuartzCore"
  "-framework ImageIO"
  "-framework UserNotifications"
  "-framework UniformTypeIdentifiers"
)

set(IUP_DRIVER_COMPILE_OPTIONS "")
set(IUP_DRIVER_LINK_OPTIONS "")

set(IUP_PC_REQUIRES "")
set(IUP_PC_LIBS_PRIVATE "-framework Foundation -framework UIKit -framework CoreGraphics -framework CoreText -framework QuartzCore -framework ImageIO -framework UserNotifications -framework UniformTypeIdentifiers")
