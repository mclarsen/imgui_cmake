################################
# Setup OpenGL
################################
find_package(OpenGL REQUIRED)
set(OPENGL_INCLUDE_DIR ${OPENGL_INCLUDE_DIR}/Headers)
set(OPENGL_gl_LIRBARY ${OPENGL_INCLUDE_DIR}/Libraries/libGL.dylib)
blt_register_library(NAME opengl
                     INCLUDES ${OPENGL_INCLUDE_DIR}}
                     LIBRARIES ${OPENGL_gl_LIBRARY})


