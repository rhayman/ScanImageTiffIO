cmake_minimum_required(VERSION 3.1)
project(ScanImageTiff VERSION 1.0.1 DESCRIPTION "tiff file io for tiffs written with scanImage" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---------- python ------------
find_package (Python3 REQUIRED COMPONENTS Interpreter Development NumPy)

include( FetchContent )
# ---------- libtiff latest stable-----------
FetchContent_Declare(
    tiff 
    GIT_REPOSITORY https://gitlab.com/libtiff/libtiff.git
    GIT_TAG master
)
set(tiff-docs OFF)
set(tiff-tests OFF)
FetchContent_MakeAvailable(tiff)

FetchContent_Declare(
    date_src
    GIT_REPOSITORY https://github.com/HowardHinnant/date.git
    GIT_TAG        v3.0.1  # adjust tag/branch/commit as needed
)
FetchContent_MakeAvailable(date_src)

FetchContent_Declare(
    carma
    GIT_REPOSITORY https://github.com/RUrlus/carma
    GIT_TAG        v0.6.3
)
FetchContent_MakeAvailable(carma)

FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11
    GIT_TAG        v2.10.0
)
FetchContent_MakeAvailable(pybind11)

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
    src/LogFileLoader.cpp
)

target_link_libraries(scanimagetiffio
    PUBLIC
    tiff
    carma::carma
    ${Python3_LIBRARIES}
    date::date
)
# # remove the "lib" from start of the library name
set_target_properties(scanimagetiffio PROPERTIES PREFIX "")
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# install
install(TARGETS scanimagetiffio DESTINATION ${Python3_SITEARCH}/scanimagetiffio)

add_library(${PROJECT_NAME} SHARED 
    src/ScanImageTiff.cpp 
    src/LogFileLoader.cpp
)
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER include/ScanImageTiff.h)
target_link_libraries(${PROJECT_NAME}
    PUBLIC
    carma::carma
    tiff
    ${Python3_LIBRARIES}
    date::date
)
install(TARGETS ${PROJECT_NAME}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin
	INCLUDES DESTINATION include
    PUBLIC_HEADER DESTINATION include
)

# ---------------------- testing ----------------
# ---- this should be moved into the tests folder -----
option(ENABLE_TESTS "Enable tests" OFF)
if (${ENABLE_TESTS})
    message(STATUS "ScanImageTiffIO: Testing is enabled")
    enable_testing()
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG main
    )
    # prevent overriding the parent projects compiler and linker settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(googletest)

    add_executable(unit_tests
        tests/tiffReader_test.cpp
        tests/SITiffIO_test.cpp
        src/ScanImageTiff.cpp
        src/LogFileLoader.cpp
    )
    
    target_link_libraries(unit_tests PUBLIC 
        ${PROJECT_NAME}
        GTest::gtest_main
    )
    include(GoogleTest)
    gtest_discover_tests(unit_tests)
endif()