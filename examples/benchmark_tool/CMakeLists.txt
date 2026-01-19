# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

add_compile_options(-fdiagnostics-color=always)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
if (DEFINED BSP)
    if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/dependencies.lock.${BSP})
        idf_build_set_property(DEPENDENCIES_LOCK dependencies.lock.${BSP})
    else()
        message(WARNING "dependencies.lock.${BSP} not found; continuing without dependencies lock")
    endif()
else()
    message(STATUS "BSP is not defined; building without dependencies lock")
endif()
project(benchmark_tool)
