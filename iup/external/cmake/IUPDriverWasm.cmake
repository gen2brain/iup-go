# WebAssembly driver (Emscripten)

if(NOT EMSCRIPTEN)
  message(FATAL_ERROR "IUP_BACKEND=wasm requires the Emscripten toolchain. Configure with `emcmake cmake ...` or `cmake --preset wasm` (needs EMSDK).")
endif()

file(GLOB _WASM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/wasm/iupwasm_*.c")

set(IUP_DRIVER_SOURCES ${_WASM_SOURCES})

set(IUP_DRIVER_COMPILE_DEFINITIONS IUP_WASM)

set(IUP_DRIVER_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/wasm")

set(IUP_DRIVER_LINK_LIBRARIES "")
set(IUP_DRIVER_COMPILE_OPTIONS "")
set(IUP_DRIVER_LINK_OPTIONS "")

set(IUP_PC_REQUIRES "")
set(IUP_PC_LIBS_PRIVATE "")
