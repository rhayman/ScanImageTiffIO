cmake_minimum_required(VERSION 3.2)
project(ScanImageTiff VERSION 1.0.1 DESCRIPTION "tiff file io for tiffs written with scanImage" LANGUAGES CXX)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

include(GNUInstallDirs)

option(BUILD_SHARED_LIBS "Build shared library" ON)

add_library(ScanImageTiff SHARED
    src/ScanImageTiff.cpp 
    src/LogFileLoader.cpp 
    src/Serialization.cpp
)

include(GenerateExportHeader)
generate_export_header(ScanImageTiff
    EXPORT_MACRO_NAME ScanImageTiff_API
    EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/include/ScanImageTiff/core/common.h
)

# ----------- opencv -----------
find_package( OpenCV REQUIRED core imgproc )
include_directories( ${OpenCV_INCLUDE_DIRS} )
include_directories( ${OpenCV_INCLUDE_DIRS}/opencv2 )
# ---------- libtiff -----------
find_package(TIFF REQUIRED)
include_directories( ${TIFF_INCLUDE_DIRS} )

include( FetchContent )
FetchContent_Declare( date_src
GIT_REPOSITORY https://github.com/HowardHinnant/date.git
GIT_TAG        v3.0.1  # adjust tag/branch/commit as needed
)
FetchContent_MakeAvailable(date_src)

target_compile_features(ScanImageTiff PUBLIC cxx_std_20)

target_include_directories(ScanImageTiff
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
set_target_properties(ScanImageTiff PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

    
# set_target_properties(ScanImageTiff PROPERTIES PREFIX "")    

target_link_libraries(ScanImageTiff PUBLIC
    ${TIFF_LIBRARIES}    
    ${OpenCV_LIBS}
    PRIVATE
    date::date
)

set_target_properties(ScanImageTiff PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER ${CMAKE_SOURCE_DIR}/include/ScanImageTiff.h
)

target_include_directories(ScanImageTiff PUBLIC
    $<INSTALL_INTERFACE:include>
    ${OpenCV_INCLUDE_DIRS}
    ${TIFF_INCLUDE_DIRS}
    # path of the headers before installation
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

# ------------ install --------------------
install(TARGETS ScanImageTiff
    EXPORT ScanImageTiffTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${LIBLEGACY_INCLUDE_DIRS}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(EXPORT ScanImageTiffTargets 
        FILE ScanImageTiffTargets.cmake
        NAMESPACE ScanImageTiff::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ScanImageTiff
)


# pkfconfig file
configure_file(${CMAKE_SOURCE_DIR}/cmake/ScanImageTiff.pc.in
    ${CMAKE_SOURCE_DIR}/cmake/ScanImageTiff.pc @ONLY
)
install(FILES ${CMAKE_SOURCE_DIR}/cmake/ScanImageTiff.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

# cmake file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/ScanImageTiffConfig.cmake.in
    ${CMAKE_SOURCE_DIR}/cmake/ScanImageTiffConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ScanImageTiff
)

export (TARGETS ScanImageTiff FILE ScanImageTiffTargets.cmake)

install(FILES ${CMAKE_SOURCE_DIR}/include/ScanImageTiff.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


install(FILES
    ${CMAKE_SOURCE_DIR}/cmake/ScanImageTiffConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ScanImageTiff
)