cmake_minimum_required(VERSION 3.1)
project(ScanImageTiff VERSION 1.0.1 DESCRIPTION "tiff file io for tiffs written with scanImage" LANGUAGES CXX)
#include(GNUInstallDirs)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(GNUInstallDirs)

# ----------- opencv -----------
find_package( OpenCV REQUIRED core imgproc )
# ---------- libtiff -----------
find_package(TIFF REQUIRED)
# ---------- python ------------
find_package (Python3 REQUIRED COMPONENTS Interpreter Development NumPy)
# ----------- boost -------------
find_package( Boost REQUIRED date_time )

# ---------- includes -----------
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${OpenCV_INCLUDE_DIRS} )
include_directories( ${OpenCV_INCLUDE_DIRS}/opencv4 )
include_directories( ${TIFF_INCLUDE_DIRS} )
include_directories( ${Python3_INCLUDE_DIRS} )
include_directories(( ${Python3_NumPy_INCLUDE_DIRS}))
include_directories( ${Boost_INCLUDE_DIRS} )

LINK_DIRECTORIES(
  ${OpenCV_LIB_DIR}
)

add_library(ScanImageTiff SHARED
    src/ScanImageTiff.cpp 
    src/LogFileLoader.cpp 
    src/Serialization.cpp
)
    
SET_TARGET_PROPERTIES(ScanImageTiff PROPERTIES PREFIX "")    

target_link_libraries(ScanImageTiff
    ${OpenCV_LIBS}
    ${TIFF_LIBRARIES}
    ${Python3_LIBRARIES}
    ${ARMADILLO_LIBRARIES}
    ${Boost_LIBRARIES}
)

set_target_properties(ScanImageTiff PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "include/ScanImageTiff.h"
)


target_include_directories(ScanImageTiff PUBLIC
    $<INSTALL_INTERFACE:include>
    # path of the headers before installation
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

# install
install(TARGETS ScanImageTiff
    RUNTIME_DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE_DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
# pkfconfig file
configure_file(ScanImageTiff.pc.in ScanImageTiff.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/ScanImageTiff.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

# cmake file
configure_package_config_file(ScanImageTiffConfig.cmake.in ScanImageTiffConfig.cmake
    INSTALL_DESINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ScanImageTiff
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ScanImageTiffConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ScanImageTiff
)