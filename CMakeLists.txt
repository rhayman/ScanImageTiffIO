cmake_minimum_required(VERSION 3.1)
project(ScanImageTiff VERSION 1.0.1 DESCRIPTION "tiff file io for tiffs written with scanImage")
#include(GNUInstallDirs)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ----------- opencv -----------
find_package( OpenCV REQUIRED core imgproc )
# ---------- libtiff -----------
find_package(TIFF REQUIRED)
# ---------- python ------------
find_package (Python3 REQUIRED COMPONENTS Interpreter Development NumPy)
# ---------- pybind11 -----------
find_package(pybind11 CONFIG REQUIRED)
# --------- armadillo -----------
find_package( Armadillo REQUIRED )
if ( ARMADILLO_FOUND )
	include_directories( ${ARMADILLO_INCLUDE_DIRS} )
endif()

# ---------- includes -----------
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${OpenCV_INCLUDE_DIRS} )
include_directories( ${OpenCV_INCLUDE_DIRS}/opencv4 )
include_directories( ${TIFF_INCLUDE_DIRS} )
include_directories( ${Python3_INCLUDE_DIRS} )
include_directories(( ${Python3_NumPy_INCLUDE_DIRS}))
include_directories( ${pybind11_INCLUDE_DIR})

SET(CMAKE_CXX_FLAGS "-std=c++11 -Wall -O3 -fPIC")

LINK_DIRECTORIES(
  ${OpenCV_LIB_DIR}
)

add_library(scanimagetiffio SHARED 
    src/ScanImageTiff.cpp 
    src/LogFileLoader.cpp 
    src/TransformContainer.cpp 
    src/ndarray_converter.cpp)

target_link_libraries(scanimagetiffio ${OpenCV_LIBS} ${TIFF_LIBRARIES} ${Python3_LIBRARIES})
# remove the "lib" from start of the library name
SET_TARGET_PROPERTIES(scanimagetiffio PROPERTIES PREFIX "")

add_library(ScanImageTiff SHARED src/ScanImageTiff.cpp)
target_link_libraries(ScanImageTiff ${OpenCV_LIBS} ${TIFF_LIBRARIES} ${Python3_LIBRARIES} ${ARMADILLO_LIBRARIES} )

set_target_properties(ScanImageTiff PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER include/ScanImageTiff.h)
configure_file(ScanImageTiff.pc.in ScanImageTiff.pc @ONLY)
target_include_directories(ScanImageTiff PUBLIC .)

# install
install(TARGETS ScanImageTiff
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_BINARY_DIR}/ScanImageTiff.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)


