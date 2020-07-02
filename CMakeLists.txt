cmake_minimum_required(VERSION 3.1)
project(ScanImageTiff VERSION 1.0.1 DESCRIPTION "tiff file io for tiffs written with scanImage")
include(GNUInstallDirs)

# ----------- opencv -----------
find_package( OpenCV REQUIRED core imgproc )
find_package(TIFF REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS} ${TIFF_INCLUDE_DIRS})
message(STATUS "TIFF_INCLUDE_DIRS")
message(STATUS ${TIFF_INCLUDE_DIRS})

add_library(ScanImageTiff SHARED src/ScanImageTiff.cpp)
target_link_libraries(ScanImageTiff ${OpenCV_LIBS} ${TIFF_LIBRARIES})

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