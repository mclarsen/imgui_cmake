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


blt_add_library(
    NAME       imgui
    SOURCES    ${imgui_sources}
    HEADERS    ${imgui_headers}
    DEPENDS_ON opengl glfw)

#######################################
# Compile the IMGUI add_ons
#######################################
# Some of these add-ons expect to be in the imgui directory
# so we will just add the includes
include_directories(imgui)
add_subdirectory(add_ons)

#######################################
# Compile the IMGUI driver program
#######################################
set(driver_sources main.cpp)

blt_add_executable(
    NAME       driver
    SOURCES    ${driver_sources}
    DEPENDS_ON imgui implot)

