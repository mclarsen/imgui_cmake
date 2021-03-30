cmake_minimum_required(VERSION 3.7)
################################
# cmake policy selections
################################
# avoid default mac osx rpath settings for cmake 3.0
cmake_policy(SET CMP0042 OLD)
# avoid warnings for project commands w/o VERSION
cmake_policy(SET CMP0048 NEW)

project(imgui_cmake VERSION "0.0.1")

################################
# Setup BLT
################################
if(NOT BLT_CXX_STD)
  set(BLT_CXX_STD "c++11" CACHE STRING "")
endif()

include(./blt/SetupBLT.cmake)
include(./cmake/SetupOpenGL.cmake)
include(./cmake/SetupGLFW.cmake)

#######################################
# Compile the IMGUI library
#######################################
set(imgui_sources imgui/imgui.cpp
                  imgui/imgui_demo.cpp
                  imgui/imgui_draw.cpp
                  imgui/imgui_tables.cpp
                  imgui/imgui_widgets.cpp
                  imgui/backends/imgui_impl_glfw.cpp
                  imgui/backends/imgui_impl_opengl2.cpp)

set(imgui_headers imgui/imconfig.h
                  imgui/imgui.h)

# Some of these add-ons expect to be in the imgui directory
# so we will just add the includes
include_directories(imgui)
#######################################
# implot
#######################################
set(implot_sources implot/implot.cpp
                   implot/implot_demo.cpp
                   implot/implot_items.cpp)

set(implot_headers implot/implot.h)
include_directories(implot)

list(APPEND imgui_sources ${implot_sources})
list(APPEND imgui_headers ${implot_headers})

blt_add_library(
    NAME       imgui
    SOURCES    ${imgui_sources}
    HEADERS    ${imgui_headers}
    DEPENDS_ON opengl glfw)

add_subdirectory(examples)
