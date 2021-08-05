cmake_minimum_required(VERSION 3.1)
project(ScanImageTiff VERSION 1.0.1 DESCRIPTION "tiff file io for tiffs written with scanImage" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---------- libtiff -----------
find_package(TIFF REQUIRED)
# ---------- python ------------
find_package (Python3 REQUIRED COMPONENTS Interpreter Development NumPy)
# ----------- boost -------------
find_package( Boost REQUIRED date_time )

# ---------- includes -----------
include_directories( ${TIFF_INCLUDE_DIRS} )
include_directories( ${Python3_INCLUDE_DIRS} )
include_directories(( ${Python3_NumPy_INCLUDE_DIRS}))
include_directories( ${Boost_INCLUDE_DIRS} )

add_library(scanimagetiffio SHARED 
    src/ScanImageTiff.cpp 
    src/LogFileLoader.cpp
)

target_link_libraries(scanimagetiffio
    PRIVATE
    carma
    PUBLIC
    ${TIFF_LIBRARIES}
    ${Python3_LIBRARIES}
    ${Boost_LIBRARIES}
)
# # remove the "lib" from start of the library name
set_target_properties(scanimagetiffio PROPERTIES PREFIX "")

set(USE_ARMA_VERSION 10.6.x)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/include/extern/carma)

# install
install(TARGETS scanimagetiffio DESTINATION ${Python3_SITEARCH}/scanimagetiffio)