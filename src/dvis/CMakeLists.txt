include(../cmake/SetupDray.cmake)
set(dvis_sources dvis.cpp )

blt_add_executable(
    NAME       devil_vision
    SOURCES    ${dvis_sources}
    DEPENDS_ON imgui dray)
