cmake_minimum_required(VERSION 3.1)
project(ScanImageTiff VERSION 1.0.1 DESCRIPTION "tiff file io for tiffs written with scanImage" LANGUAGES CXX)
#include(GNUInstallDirs)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(GNUInstallDirs)

# ----------- opencv -----------
find_package( OpenCV REQUIRED core imgproc )
# ---------- libtiff -----------
find_package(TIFF REQUIRED)


include( FetchContent )
FetchContent_Declare( date_src
GIT_REPOSITORY https://github.com/HowardHinnant/date.git
GIT_TAG        v3.0.1  # adjust tag/branch/commit as needed
)
FetchContent_MakeAvailable(date_src)

# ---------- includes -----------
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${OpenCV_INCLUDE_DIRS} )
include_directories( ${OpenCV_INCLUDE_DIRS}/opencv2 )
include_directories( ${TIFF_INCLUDE_DIRS} )

LINK_DIRECTORIES(
  ${OpenCV_LIB_DIR}
)

add_library(ScanImageTiff SHARED
    src/ScanImageTiff.cpp 
    src/LogFileLoader.cpp 
    src/Serialization.cpp
)
    
SET_TARGET_PROPERTIES(ScanImageTiff PROPERTIES PREFIX "")    

target_link_libraries(ScanImageTiff PUBLIC
    ${TIFF_LIBRARIES}    
    ${OpenCV_LIBS}
    PRIVATE
    date::date
)

set_target_properties(ScanImageTiff PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "include/ScanImageTiff.h"
)

target_include_directories(ScanImageTiff PUBLIC
    $<INSTALL_INTERFACE:include>
    ${OpenCV_INCLUDE_DIRS}
    ${TIFF_INCLUDE_DIRS}
    # path of the headers before installation
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

# ------------ install --------------------
install(TARGETS ScanImageTiff EXPORT ScanImageTiffTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# pkfconfig file
configure_file(ScanImageTiff.pc.in ScanImageTiff.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/ScanImageTiff.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

# cmake file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ScanImageTiffConfig.cmake.in ScanImageTiffConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ScanImageTiff
    NO_SET_AND_CHECK_MACRO
)

export (TARGETS ScanImageTiff FILE ScanImageTiffTargets.cmake)

install(FILES include/ScanImageTiff.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install (EXPORT ScanImageTiffTargets 
        FILE ScanImageTiffTargets.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ScanImageTiff)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ScanImageTiffConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ScanImageTiff
)