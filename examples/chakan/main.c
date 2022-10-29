/* Copyright (c) 2019 Pierre-Marc Jobin
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <lodepng.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vdp.h>

static const unsigned char chakanfg[] = {
#include "chakanfg.png.i"
};

static const unsigned char chakanbg[] = {
#include "chakanbg.png.i"
};

static const unsigned char pentagram[] = {
#include "pentagram.png.i"
};

static vdp_color_t color_table[VDP_COLOR_COUNT];
static uint32_t pattern_table[VDP_PATTERN_COUNT * VDP_PATTERN_WIDTH * VDP_PATTERN_HEIGHT * VDP_PATTERN_BPP / 32];
static vdp_sprite_t sprite_table[VDP_SPRITE_COUNT];
static vdp_cell_t plane_table[VDP_PLANE_COUNT][VDP_PLANE_MAX_HEIGHT][VDP_PLANE_MAX_WIDTH];
static uint16_t hscroll_table[2][VDP_HSCROLL_COUNT];
static uint16_t vscroll_table[2][VDP_VSCROLL_COUNT];

static inline int imin(int a, int b) {
	return a < b ? a : b;
}

static inline int imax(int a, int b) {
	return a > b ? a : b;
}

static void extract_patterns_from_png(uint32_t **patterns, vdp_color_t **colors, const unsigned char *image, size_t size, bool is_sprite) {
	uint32_t *pixels;
	unsigned int width, height;
	LodePNGState state;

	lodepng_state_init(&state);
	state.decoder.color_convert = 0;
	lodepng_decode((unsigned char **)&pixels, &width, &height, &state, image, size);
	memcpy(*colors, state.info_png.color.palette, state.info_png.color.palettesize * sizeof (vdp_color_t));
	*colors += state.info_png.color.palettesize;
	if (is_sprite) {
		for (unsigned int i = 0; i < width; i += 8) {
			for (unsigned int j = 0; j < height; j += 8) {
				for (unsigned int k = 0; k < 8; ++k) {
					*(*patterns)++ = pixels[(i + (j + k) * width) / 8];
				}
			}
		}
	} else {
		for (unsigned int j = 0; j < height; j += 8) {
			for (unsigned int i = 0; i < width; i += 8) {
				for (unsigned int k = 0; k < 8; ++k) {
					*(*patterns)++ = pixels[(i + (j + k) * width) / 8];
				}
			}
		}
	}
	lodepng_state_cleanup(&state);
	free(pixels);
}

static void APIENTRY display_debug_message(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *user) {
	fprintf(stderr, "%s\n", message);
}

int main() {
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
	GLFWwindow* window = glfwCreateWindow(VDP_FRAMEBUFFER_WIDTH * 2, VDP_FRAMEBUFFER_HEIGHT * 2, "VDP demo", NULL, NULL);
	if (!window) {
		fprintf(stderr, "Unable to create GL window, exiting.\n");
		return -1;
	}
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);

	if (gl3wInit()) {
		glfwDestroyWindow(window);
		fprintf(stderr, "Failed to initialize OpenGL, exiting.\n");
		return -1;
	}
	if (!gl3wIsSupported(4, 5)) {
		glfwDestroyWindow(window);
		fprintf(stderr, "OpenGL 4.5 not supported, exiting.\n");
		return -1;
	}
	printf("OpenGL %s, GLSL %s\n", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
	glDebugMessageCallback(display_debug_message, window);

	vdp_context_t *vdp = vdp_create_context();
	if (!vdp) {
		glfwDestroyWindow(window);
		fprintf(stderr, "Unable to create VDP emulator, exiting.\n");
		return -1;
	}

	uint32_t *patterns = pattern_table;
	vdp_color_t *colors = color_table;
	extract_patterns_from_png(&patterns, &colors, chakanfg, sizeof (chakanfg), false);
	extract_patterns_from_png(&patterns, &colors, chakanbg, sizeof (chakanbg), false);
	patterns -= 4 * 4 * 8;
	colors += 16;
	extract_patterns_from_png(&patterns, &colors, pentagram, sizeof (pentagram), true);
	const unsigned int iw = 256 / VDP_PATTERN_WIDTH;
	const unsigned int ih = 256 / VDP_PATTERN_HEIGHT;

	memset(hscroll_table, 0, sizeof (hscroll_table));
	memset(vscroll_table, 0, sizeof (vscroll_table));
	memset(plane_table, 0, sizeof (plane_table));
	memset(sprite_table, 0, sizeof (sprite_table));
	for (unsigned int j = 0; j < VDP_PLANE_MAX_HEIGHT; ++j) {
		for (unsigned int i = 0; i < VDP_PLANE_MAX_WIDTH; ++i) {
			if (i > 32 && i < 64) {
				plane_table[VDP_PLANE_A][j][i].pattern = (i & (iw - 1)) + (j & (ih - 1)) * iw;
				plane_table[VDP_PLANE_A][j][i].palette = 0;
				plane_table[VDP_PLANE_A][j][i].priority = true;
			}

			plane_table[VDP_PLANE_B][j][i].pattern = (i & (iw - 1)) + (j & (ih - 1)) * iw + 1024;
			plane_table[VDP_PLANE_B][j][i].palette = 1;
			plane_table[VDP_PLANE_B][j][i].priority = true;

			bool hf = (i & 4) != 0;
			bool vf = (j & 4) != 0;
			plane_table[VDP_PLANE_W][j][i].pattern = 2048 - 16 + ((hf ? 3 - i : i) & 3) + ((vf ? 3 - j : j) & 3) * 4;
			plane_table[VDP_PLANE_W][j][i].hflip = hf;
			plane_table[VDP_PLANE_W][j][i].vflip = vf;
			plane_table[VDP_PLANE_W][j][i].palette = 3;
			plane_table[VDP_PLANE_W][j][i].priority = true;
		}
	}

	vdp_set_mode(vdp, VDP_MODE_INTENSITY);
	vdp_set_plane_size(vdp, VDP_PLANE_MAX_WIDTH, VDP_PLANE_MAX_HEIGHT);
	vdp_set_colors_sh(vdp, 0, colors - color_table, color_table);
	vdp_set_patterns(vdp, 0, (patterns - pattern_table) / 8, pattern_table);
	vdp_set_sprites(vdp, 0, VDP_SPRITE_COUNT, sprite_table);
	vdp_set_cells(vdp, VDP_PLANE_A, 0, 0, VDP_PLANE_MAX_WIDTH, VDP_PLANE_MAX_HEIGHT, &plane_table[VDP_PLANE_A][0][0]);
	vdp_set_cells(vdp, VDP_PLANE_B, 0, 0, VDP_PLANE_MAX_WIDTH, VDP_PLANE_MAX_HEIGHT, &plane_table[VDP_PLANE_B][0][0]);
	vdp_set_cells(vdp, VDP_PLANE_W, 0, 0, VDP_PLANE_MAX_WIDTH, VDP_PLANE_MAX_HEIGHT, &plane_table[VDP_PLANE_W][0][0]);
	vdp_set_hscroll(vdp, VDP_PLANE_A, 0, VDP_HSCROLL_COUNT, &hscroll_table[VDP_PLANE_A][0]);
	vdp_set_vscroll(vdp, VDP_PLANE_A, 0, VDP_VSCROLL_COUNT, &vscroll_table[VDP_PLANE_A][0]);
	vdp_set_hscroll(vdp, VDP_PLANE_B, 0, VDP_HSCROLL_COUNT, &hscroll_table[VDP_PLANE_B][0]);
	vdp_set_vscroll(vdp, VDP_PLANE_B, 0, VDP_VSCROLL_COUNT, &vscroll_table[VDP_PLANE_B][0]);

	double last_t = glfwGetTime();
	unsigned int frame_count = 0;
	while (!glfwWindowShouldClose(window)) {
		double t = glfwGetTime();
		++frame_count;
		if (t - last_t >= 1.0) {
			char title[128];
			snprintf(title, sizeof (title), "VDP demo (%d fps)", frame_count);
			glfwSetWindowTitle(window, title);
			last_t = t;
			frame_count = 0;
		}

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		int zoom = imax(imin(width / VDP_FRAMEBUFFER_WIDTH, height / VDP_FRAMEBUFFER_HEIGHT), 1);
		int vdp_width = zoom * VDP_FRAMEBUFFER_WIDTH;
		int vdp_height = zoom * VDP_FRAMEBUFFER_HEIGHT;
		int vdp_x = (width - vdp_width) / 2;
		int vdp_y = (height - vdp_height) / 2;

		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		xpos = (xpos - vdp_x) / zoom;
		ypos = (ypos - vdp_y) / zoom;
		//vdp_set_window_coord(vdp, xpos, ypos);

		sprite_table[0].x = xpos - 16 + 128;
		sprite_table[0].y = ypos - 16 + 128;
		sprite_table[0].hsize = 3;
		sprite_table[0].vsize = 3;
		sprite_table[0].hflip = false;
		sprite_table[0].vflip = false;
		sprite_table[0].pattern = 2048 - 16;
		sprite_table[0].palette = 3;
		sprite_table[0].priority = true;
		sprite_table[0].link = 0;

		vdp_set_sprites(vdp, 0, 1, sprite_table);

#if 1
		for (unsigned int i = 0; i < VDP_HSCROLL_COUNT; i += 2) {
			hscroll_table[VDP_PLANE_A][i + 0] = -224;
			hscroll_table[VDP_PLANE_A][i + 1] = -224;
			hscroll_table[VDP_PLANE_B][i + 0] = (uint16_t)(-224 + 8 * sin((double)i / 32 + t / 2));
			hscroll_table[VDP_PLANE_B][i + 1] = (uint16_t)(-224 - 8 * cos((double)i / 32 + t / 2));
		}
		for (unsigned int i = 0; i < VDP_VSCROLL_COUNT; ++i) {
			vscroll_table[VDP_PLANE_A][i] = (uint16_t)(4 * fabs(sin(t)));
			vscroll_table[VDP_PLANE_B][i] = 0; //(uint16_t)(16 * sin((double)i / 32 + t / 5));
		}
#else
		for (unsigned int i = 0; i < VDP_HSCROLL_COUNT; ++i) {
			hscroll_table[0][i] = 224;
			hscroll_table[1][i] = 224 + t * 10;
		}
		for (unsigned int i = 0; i < VDP_VSCROLL_COUNT; ++i) {
			vscroll_table[0][i] = (uint16_t)(4 * fabs(sin(t)));
			vscroll_table[1][i] = i * 4 + 100;
		}
#endif
		vdp_set_hscroll(vdp, VDP_PLANE_A, 0, VDP_HSCROLL_COUNT, &hscroll_table[VDP_PLANE_A][0]);
		vdp_set_hscroll(vdp, VDP_PLANE_B, 0, VDP_HSCROLL_COUNT, &hscroll_table[VDP_PLANE_B][0]);
		vdp_set_vscroll(vdp, VDP_PLANE_A, 0, VDP_VSCROLL_COUNT, &vscroll_table[VDP_PLANE_A][0]);
		vdp_set_vscroll(vdp, VDP_PLANE_B, 0, VDP_VSCROLL_COUNT, &vscroll_table[VDP_PLANE_B][0]);

		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);
		vdp_render(vdp);
		vdp_blit(vdp, vdp_x, vdp_y, vdp_width, vdp_height, VDP_FILTER_NEAREST);

		glfwPollEvents();
		glfwSwapBuffers(window);
	}
	vdp_destroy_context(vdp);
	glfwDestroyWindow(window);

	return 0;
}
