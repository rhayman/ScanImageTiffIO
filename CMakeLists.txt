cmake_minimum_required(VERSION 3.22)
project(ScanImageTiff VERSION 1.0.3 DESCRIPTION "tiff file io for tiffs written with scanImage" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# generate the compile_commands.json file for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

if (WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

configure_file(src/ScanImageTiff_version.cpp.in ScanImageTiff_version.cpp @ONLY)
add_library(ScanImageTiff_version STATIC ScanImageTiff_version.cpp)
# ---------- python ------------
find_package (Python3 COMPONENTS Interpreter Development NumPy)

include( FetchContent )
# ---------- libtiff latest stable-----------
FetchContent_Declare(
    tiff 
    GIT_REPOSITORY https://gitlab.com/libtiff/libtiff.git
    GIT_TAG master
)
option(tiff-docs  "" OFF)
option(tiff-tests ""  OFF)
option(tiff-tools "" OFF)
option(tiff-contrib "" OFF)

FetchContent_Declare(
    carma
    GIT_REPOSITORY https://github.com/RUrlus/carma
    GIT_TAG stable 
)

FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11
    GIT_TAG master
)

FetchContent_MakeAvailable(tiff pybind11 carma)

# ensure pybind11 is available
FetchContent_GetProperties(pybind11)
if(NOT pybind11_POPULATED)
    FetchContent_Populate(pybind11)
    add_subdirectory(${pybind11_SOURCE_DIR} ${pybind11_BINARY_DIR})
endif()

# ---------- includes -----------
include_directories( ${TIFF_INCLUDE_DIRS} )
include_directories( ${Python3_INCLUDE_DIRS} )
include_directories(( ${Python3_NumPy_INCLUDE_DIRS}))

pybind11_add_module(scanimagetiffio SHARED 
    src/ScanImageTiffPy.cpp
    src/ScanImageTiff.cpp 
    src/VRDataFiles.cpp
)

target_link_libraries(scanimagetiffio
    PUBLIC
    tiff
    armadillo
    carma::carma
    ${Python3_LIBRARIES}
    ScanImageTiff_version
)
target_include_directories(scanimagetiffio PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
# # remove the "lib" from start of the library name
set_target_properties(scanimagetiffio PROPERTIES PREFIX "")

# install
install(TARGETS scanimagetiffio DESTINATION ${Python3_SITEARCH})

add_library(${PROJECT_NAME} SHARED 
    src/ScanImageTiff.cpp 
    src/VRDataFiles.cpp
)
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER include/ScanImageTiff.h)
target_link_libraries(${PROJECT_NAME}
    PUBLIC
    armadillo
    carma::carma
    tiff
    ${Python3_LIBRARIES}
    ScanImageTiff_version
)
install(TARGETS ${PROJECT_NAME}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin
	INCLUDES DESTINATION include
    PUBLIC_HEADER DESTINATION include
)
add_subdirectory(tests)
