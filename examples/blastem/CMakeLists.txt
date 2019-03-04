cmake_minimum_required(VERSION 3.12)
project(blastem C)

set(CMAKE_C_STANDARD 99)

set(BLASTEM_SRC_URL "https://www.retrodev.com/repos/blastem/archive/357b4951d9b2.tar.gz")
set(BLASTEM_SRC_ZIP "${CMAKE_CURRENT_SOURCE_DIR}/blastem.src.tar.gz")

include(FindPkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(GLEW REQUIRED glew)

if(NOT EXISTS ${BLASTEM_SRC_ZIP})
	message(STATUS "Downloading '${BLASTEM_SRC_ZIP}'...")
	file(DOWNLOAD ${BLASTEM_SRC_URL} ${BLASTEM_SRC_ZIP} STATUS status)
endif()

if(NOT EXISTS vdp.c)
	message(STATUS "Extracting '${BLASTEM_SRC_ZIP}'...")
	message(STATUS "${CMAKE_COMMAND} -E tar xfz ${BLASTEM_SRC_ZIP} --strip=1")
	execute_process(COMMAND tar xf ${BLASTEM_SRC_ZIP} --strip=1 WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" RESULT_VARIABLE RV)
	if(NOT RV EQUAL 0)
		message(FATAL_ERROR "error: failed to extract '${BLASTEM_SRC_ZIP}'")
	endif()
	execute_process(COMMAND patch WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" INPUT_FILE "vdp.patch" RESULT_VARIABLE RV)
	if(NOT RV EQUAL 0)
		message(FATAL_ERROR "error: failed to apply 'vdp.patch'")
	endif()
endif()

file(COPY default.cfg rom.db gamecontrollerdb.txt menu.bin shaders DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

set(SRC
	blastem.c
	system.c
	genesis.c
	debug.c
	gdb_remote.c
	vdp.c
	glvdp.c
	render_sdl.c
	ppm.c
	png.c
	io.c
	romdb.c
	hash.c
	menu.c
	xband.c
	realtec.c
	i2c.c
	nor.c
	sega_mapper.c
	multi_game.c
	megawifi.c
	net.c
	serialize.c
	terminal.c
	config.c
	tern.c
	util.c
	paths.c
	gst.c
	68kinst.c
	m68k_core.c
	m68k_core_x86.c
	gen.c
	backend.c
	mem.c
	arena.c
	tern.c
	gen_x86.c
	backend_x86.c
	sms.c
	z80inst.c
	z80_to_x86.c
	ym2612.c
	psg.c
	wave.c
	saves.c
	zip.c
	bindings.c
	jcart.c
	controller_info.c
	#nuklear_ui/font.c
	#nuklear_ui/blastem_nuklear.c
	#nuklear_ui/sfnt.c
	zlib/adler32.c
	zlib/compress.c
	zlib/crc32.c
	zlib/deflate.c
	zlib/gzclose.c
	zlib/gzlib.c
	zlib/gzread.c
	zlib/gzwrite.c
	zlib/infback.c
	zlib/inffast.c
	zlib/inflate.c
	zlib/inftrees.c
	zlib/trees.c
	zlib/uncompr.c
	zlib/zutil.c
)

add_executable(blastem ${SRC})
add_compile_definitions(X86_64 DISABLE_NUKLEAR)
target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES} ${GLEW_LIBRARIES} vdp gl3w m dl)
