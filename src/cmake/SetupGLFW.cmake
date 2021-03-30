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

