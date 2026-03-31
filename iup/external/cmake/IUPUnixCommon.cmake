# Unix common files shared by GTK, GTK4, Motif, Qt, EFL on Unix platforms

set(IUP_UNIX_COMMON_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_notify.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_singleinstance.c"
)

if(NOT IUP_BACKEND MATCHES "^qt")
  list(APPEND IUP_UNIX_COMMON_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_info.c"
  )
endif()

if(IUP_BACKEND STREQUAL "motif" OR IUP_BACKEND STREQUAL "fltk")
  list(APPEND IUP_UNIX_COMMON_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_thread.c"
  )
endif()

if(NOT IUP_BACKEND MATCHES "^qt")
  list(APPEND IUP_UNIX_COMMON_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_help.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/iupunix_portal.c"
  )
endif()
