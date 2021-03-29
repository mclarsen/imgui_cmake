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

include(./src/blt/SetupBLT.cmake)

################################
# Setup OpenGL
################################
find_package(OpenGL REQUIRED)
set(OPENGL_INCLUDE_DIR ${OPENGL_INCLUDE_DIR}/Headers)
set(OPENGL_gl_LIRBARY ${OPENGL_INCLUDE_DIR}/Libraries/libGL.dylib)
blt_register_library(NAME opengl
                     INCLUDES ${OPENGL_INCLUDE_DIR}}
                     LIBRARIES ${OPENGL_gl_LIBRARY})

################################
# Setup GLFW3
################################
if(NOT GLFW3_DIR)
    MESSAGE(FATAL_ERROR "Rendering needs explicit GLFW3_DIR")
endif()

set(glfw3_DIR "${GLFW3_DIR}/lib/cmake/glfw3")
find_package(glfw3 REQUIRED)

blt_register_library(NAME glfw
                     INCLUDES ${GLFW3_DIR}/include
                     LIBRARIES glfw)

add_subdirectory(src)
