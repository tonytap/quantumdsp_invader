cmake_minimum_required(VERSION 3.22)

project(Invader)

set(CMAKE_CXX_STANDARD 17)

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)

include_directories(${CMAKE_SOURCE_DIR}/plugin)
set(LICENSESPRING_DIR ${LIB_DIR}/LicenseSpring)

if (MSVC)
    add_compile_options(/Wall /WX)
else() 
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(JUCE_DIR "$ENV{HOME}/Documents/JUCE")
add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/juce)
add_subdirectory(plugin)