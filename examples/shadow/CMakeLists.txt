file(GLOB SRC *.c)

add_executable(shadow ${SRC})
target_link_libraries(shadow gl3w glfw vdp)
