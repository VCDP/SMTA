cmake_minimum_required( VERSION 2.6.2 FATAL_ERROR )
project( imediapro-linear_transcoder )

option( CMAKE_MFX_HOME "Where is Media SDK?" OFF )

option( ENABLE_SW "Build SW versions of the targets?" OFF )

option( BUILD_SMTA "Build SMTA?" ON )
option( BUILD_LIBXCODE_TEST "Build LibXcode test?" OFF )

if( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
option( ENABLE_DRM "Build DRM versions of the targets?" ON )
option( ENABLE_X11 "Build X11 versions of the targets?" ON )
option( ENABLE_X11_DRI3 "Build X11 DRI3 versions of the targets?" OFF )
option( ENABLE_WAYLAND "Build WAYLAND versions of the targets?" OFF )
endif()

include( CMake/FindFunctions.cmake )
include( CMake/FindGlobals.cmake )
include( CMake/FindMFX.cmake )

if( Linux )
  set(LIBDISPATCH_ENABLE ON)
endif()

if( Windows )
  set( CompilerFlags CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_DEBUG )
  foreach(CompilerFlag ${CompilerFlags})
    string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
  endforeach()
endif()

if( BUILD_SMTA )
message(STATUS "Build for SMTA")
add_definitions(-DENABLE_RAW_CODEC -DENABLE_RAW_DECODE -DSUPPORT_INTERLACE -DSUPPORT_SMTA -DENABLE_HEVC -DMSDK_AVC_FEI -DMSDK_HEVC_FEI)
endif()

set( NOT_CONFIGURED "" CACHE INTERNAL "" )
set( SKIPPING "" CACHE INTERNAL "" )
set( BUILDING "" CACHE INTERNAL "" )

add_subdirectory(libxcode)
add_subdirectory(app)

#report_targets("The following targets were NOT configured:" "${NOT_CONFIGURED}")
#report_targets("The following targets will be ${BoldRed}SKIPPED${EndColor}:" "${SKIPPING}")
#report_targets("The following targets will be ${BoldGreen}BUILT${EndColor}:" "${BUILDING}")
