INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_LORA_RTS_CTS lora_rts_cts)

FIND_PATH(
    LORA_RTS_CTS_INCLUDE_DIRS
    NAMES lora_rts_cts/api.h
    HINTS $ENV{LORA_RTS_CTS_DIR}/include
        ${PC_LORA_RTS_CTS_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    LORA_RTS_CTS_LIBRARIES
    NAMES gnuradio-lora_rts_cts
    HINTS $ENV{LORA_RTS_CTS_DIR}/lib
        ${PC_LORA_RTS_CTS_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/lora_rts_ctsTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LORA_RTS_CTS DEFAULT_MSG LORA_RTS_CTS_LIBRARIES LORA_RTS_CTS_INCLUDE_DIRS)
MARK_AS_ADVANCED(LORA_RTS_CTS_LIBRARIES LORA_RTS_CTS_INCLUDE_DIRS)
