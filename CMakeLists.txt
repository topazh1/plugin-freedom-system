cmake_minimum_required(VERSION 3.22)

# Set deployment target before project
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS deployment version")

project(JUCEPlugins VERSION 1.0.0)

# JUCE location — override with -DJUCE_DIR=/path/to/JUCE
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE" CACHE PATH "Path to JUCE framework")
add_subdirectory(${JUCE_DIR} JUCE)

# Auto-discover plugins
file(GLOB PLUGIN_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/*")
foreach(PLUGIN_DIR ${PLUGIN_DIRS})
    if(IS_DIRECTORY ${PLUGIN_DIR} AND EXISTS "${PLUGIN_DIR}/CMakeLists.txt")
        add_subdirectory(${PLUGIN_DIR})
    endif()
endforeach()
