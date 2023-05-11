cmake_minimum_required(VERSION 3.1)

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
        tiffReader_test.cpp
        SITiffIO_test.cpp
        ../src/ScanImageTiff.cpp
        ../src/LogFileLoader.cpp
    )
    
    target_link_libraries(unit_tests PUBLIC 
        ${PROJECT_NAME}
        GTest::gtest_main
    )
    include(GoogleTest)
    gtest_discover_tests(unit_tests)

    # copy the test data to the location of the unit_test binary
    add_custom_command(TARGET unit_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/tests/data"
        $<TARGET_FILE_DIR:unit_tests>)

endif()