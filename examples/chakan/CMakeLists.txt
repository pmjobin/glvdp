file(GLOB SRC *.c)

file(GLOB DATA data/*)
stringify(DATA_SRC ${DATA})

add_executable(chakan ${SRC} ${DATA_SRC})
target_link_libraries(chakan gl3w glfw lodepng vdp)
