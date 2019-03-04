file(GLOB SRC *.c)

file(GLOB GLSL *.glsl)
stringify(GLSL_SRC ${GLSL})

add_library(vdp STATIC ${SRC} ${GLSL_SRC})
