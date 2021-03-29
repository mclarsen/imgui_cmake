set(driver_sources main.cpp
                   imgui/imgui.cpp
                   imgui/imgui_demo.cpp
                   imgui/imgui_draw.cpp
                   imgui/imgui_tables.cpp
                   imgui/imgui_widgets.cpp
                   imgui/backends/imgui_impl_glfw.cpp
                   imgui/backends/imgui_impl_opengl2.cpp
#imgui/imguifilesystem.cpp
                   )
include_directories(imgui)

blt_add_executable(
    NAME       driver
    SOURCES    ${driver_sources}
    DEPENDS_ON opengl glfw)

