cmake_minimum_required(VERSION 3.1)
project(ScanImageTiff VERSION 1.0.1 DESCRIPTION "tiff file io for tiffs written with scanImage")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---------- libtiff -----------
find_package(TIFF REQUIRED)
# ---------- python ------------
find_package (Python3 REQUIRED COMPONENTS Interpreter Development NumPy)
# ---------- pybind11 -----------
find_package(pybind11 CONFIG REQUIRED)
# ----------- boost -------------
find_package( Boost REQUIRED system filesystem regex date_time )

# ---------- includes -----------
include_directories( ${TIFF_INCLUDE_DIRS} )
include_directories( ${Python3_INCLUDE_DIRS} )
include_directories(( ${Python3_NumPy_INCLUDE_DIRS}))
include_directories( ${pybind11_INCLUDE_DIR})
include_directories( ${Boost_INCLUDE_DIRS} )

# add_library(scanimagetiffio SHARED 
#     src/ScanImageTiff.cpp 
#     src/LogFileLoader.cpp )



# target_link_libraries(scanimagetiffio
#     PUBLIC
#     carma
#     ${TIFF_LIBRARIES}
#     ${Python3_LIBRARIES}
#     ${Boost_LIBRARIES}
#     )
# # remove the "lib" from start of the library name
# SET_TARGET_PROPERTIES(scanimagetiffio PROPERTIES PREFIX "")

add_library(ScanImageTiff SHARED
    src/ScanImageTiff.cpp 
    src/LogFileLoader.cpp )

target_include_directories(ScanImageTiff
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/extern/carma/include
)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/include/extern/carma)
target_link_libraries(ScanImageTiff
    PUBLIC
    ${TIFF_LIBRARIES}
    ${Boost_LIBRARIES}
    ${Python3_LIBRARIES}
    pybind11::pybind11
    PRIVATE
    carma
    )

set_target_properties(ScanImageTiff PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER ${CMAKE_SOURCE_DIR}/include/ScanImageTiff.h )
configure_file(ScanImageTiff.pc.in ScanImageTiff.pc @ONLY)

# install
install(TARGETS ScanImageTiff
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    INCLUDES
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_BINARY_DIR}/ScanImageTiff.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)