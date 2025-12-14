cmake_minimum_required(VERSION 3.22)

project(Invader)

set(CMAKE_CXX_STANDARD 20)

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)

include(cmake/cpm.cmake)
include_directories(${CMAKE_SOURCE_DIR}/plugin)

CPMAddPackage(
    NAME JUCE
    GITHUB_REPOSITORY juce-framework/JUCE 
    GIT_TAG 8.0.1
    VERSION 8.0.1
    SOURCE_DIR ${LIB_DIR}/juce
)

# Set LicenseSpring directory
set(LICENSESPRING_DIR ${LIB_DIR}/LicenseSpring)

if (MSVC)
    add_compile_options(/Wall /WX)
else() 
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_subdirectory(plugin)