cmake_minimum_required(VERSION 3.22)
project(ScanImageTiff VERSION 1.0.3 DESCRIPTION "tiff file io for tiffs written with scanImage" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
option(BUILD_TESTING "Build unit tests" ON)
option(BUILD_PYTHON_BINDINGS "Build Python bindings" ON)
# generate the compile_commands.json file for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

if (WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

configure_file(src/ScanImageTiff_version.cpp.in ScanImageTiff_version.cpp @ONLY)
add_library(ScanImageTiff_version STATIC ScanImageTiff_version.cpp)

include( FetchContent )
# ---------- libtiff latest stable-----------
FetchContent_Declare(
    tiff 
    GIT_REPOSITORY https://gitlab.com/libtiff/libtiff.git
    GIT_TAG v4.7.1
)
option(tiff-docs  "" OFF)
option(tiff-tests ""  OFF)
option(tiff-tools "" OFF)
option(tiff-contrib "" OFF)

FetchContent_MakeAvailable(tiff)

# ---------- includes -----------
include_directories( ${TIFF_INCLUDE_DIRS} )

add_library(${PROJECT_NAME} SHARED 
    src/ScanImageTiff.cpp 
    src/VRDataFiles.cpp
)
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER include/ScanImageTiff.h)
target_link_libraries(${PROJECT_NAME}
    PUBLIC
    armadillo
    tiff
    ScanImageTiff_version
)
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
install(TARGETS ${PROJECT_NAME}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin
	INCLUDES DESTINATION include
    PUBLIC_HEADER DESTINATION include
)

if (BUILD_PYTHON_BINDINGS)
    find_package(Python3 COMPONENTS Interpreter Development NumPy)
    if (NOT Python3_FOUND)
        message(FATAL_ERROR
            "BUILD_PYTHON_BINDINGS=ON requires Python3 Interpreter, Development, and NumPy components. "
            "Install Python development headers and NumPy, or configure with -DBUILD_PYTHON_BINDINGS=OFF."
        )
    endif()
    FetchContent_Declare(
        carma
        GIT_REPOSITORY https://github.com/RUrlus/carma
        GIT_TAG c1db538e13e291ad0e5c95adb96bb315cc121b8f
    )
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11
        GIT_TAG v2.13.6
    )
    FetchContent_MakeAvailable(pybind11 carma)

    pybind11_add_module(scanimagetiffio SHARED src/ScanImageTiffPy.cpp)
    target_link_libraries(scanimagetiffio
        PUBLIC
        ${PROJECT_NAME}
        carma::carma
        ${Python3_LIBRARIES}
    )
    target_include_directories(scanimagetiffio PUBLIC
        ${Python3_INCLUDE_DIRS}
        ${Python3_NumPy_INCLUDE_DIRS}
        "${CMAKE_CURRENT_BINARY_DIR}"
    )
    # remove the "lib" from start of the library name
    set_target_properties(scanimagetiffio PROPERTIES PREFIX "")
    install(TARGETS scanimagetiffio DESTINATION ${Python3_SITEARCH})
endif()
if (BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif()
