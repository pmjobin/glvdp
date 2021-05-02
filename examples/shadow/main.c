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
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vdp.h>

#define countof(a) (sizeof (a) / sizeof (a[0]))

static const uint32_t tiles[] = {
#include "tiles.i"
};

enum {
	PLANE_WIDTH		= 64,
	PLANE_HEIGHT	= 32
};

static vdp_color_t color_table[VDP_COLOR_COUNT];
static uint32_t pattern_table[VDP_PATTERN_COUNT * VDP_PATTERN_WIDTH * VDP_PATTERN_HEIGHT * VDP_PATTERN_BPP / 32];
static vdp_sprite_t sprite_table[VDP_SPRITE_COUNT];
static vdp_cell_t plane_table[VDP_PLANE_COUNT][PLANE_HEIGHT][PLANE_WIDTH];
static uint16_t hscroll_table[2][VDP_HSCROLL_COUNT];
static uint16_t vscroll_table[2][VDP_VSCROLL_COUNT];

static inline int imin(int a, int b) {
	return a < b ? a : b;
}

static inline int imax(int a, int b) {
	return a > b ? a : b;
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
	glfwSwapInterval(1);

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

	memset(color_table, 0, sizeof (color_table));
	memset(pattern_table, 0, sizeof (pattern_table));
	memset(sprite_table, 0, sizeof (sprite_table));
	memset(plane_table, 0, sizeof (plane_table));
	memset(hscroll_table, 0, sizeof (hscroll_table));
	memset(vscroll_table, 0, sizeof (vscroll_table));

	color_table[ 0].rgb = 0x808080;
	color_table[ 1].rgb = 0x000080;
	color_table[17].rgb = 0x800000;
	color_table[33].rgb = 0x008080;
	color_table[46].rgb = 0x008000;
	color_table[47].rgb = 0x008000;
	color_table[49].rgb = 0x008080;

	for (unsigned int i = 0; i < countof (tiles); ++i) {
		pattern_table[i] = __builtin_bswap32(tiles[i]);
	}

	for (unsigned int j = 0; j < 32; ++j) {
		for (int i = 0; i < 64; ++i) {
			plane_table[VDP_PLANE_A][j][i].pattern = 1;
			plane_table[VDP_PLANE_A][j][i].palette = 0;
			plane_table[VDP_PLANE_A][j][i].hflip = (i & 1) != 0;
			plane_table[VDP_PLANE_A][j][i].vflip = (j & 1) != 0;
			plane_table[VDP_PLANE_A][j][i].priority = (i & 32) != 0;

			plane_table[VDP_PLANE_B][j][i].pattern = 1;
			plane_table[VDP_PLANE_B][j][i].palette = 1;
			plane_table[VDP_PLANE_B][j][i].hflip = (i & 1) != 0;
			plane_table[VDP_PLANE_B][j][i].vflip = (j & 1) != 0;
			plane_table[VDP_PLANE_B][j][i].priority = (j & 16) != 0;
		}
	}

	for (unsigned int i = 0; i < 8; ++i) {
		sprite_table[i].x = (i & 3) * 320 / 4 + 320 / 8 - (32 / 2) + 128;
		sprite_table[i].y = (i >> 2) * 224 / 2 + 224 / 4 - (32 / 2) + 128;
		sprite_table[i].hsize = 3;
		sprite_table[i].vsize = 3;
		sprite_table[i].palette = 3;
		sprite_table[i].pattern = 2;
		sprite_table[i].priority = (i & 1) != 0;
		sprite_table[i].link = i + 1;
	}
	sprite_table[8].x = 320 / 2 - (32 / 2) + 128;
	sprite_table[8].y = 224 / 2 - (32 / 2) + 128;
	sprite_table[8].hsize = 3;
	sprite_table[8].vsize = 3;
	sprite_table[8].palette = 2;
	sprite_table[8].pattern = 2;
	sprite_table[8].priority = 0;
	sprite_table[8].link = 0;

	vdp_set_mode(vdp, VDP_MODE_INTENSITY);
	vdp_set_plane_size(vdp, 64, 32);
	vdp_set_colors_sh(vdp, 0, countof (color_table), color_table);
	vdp_set_patterns(vdp, 0, sizeof (tiles) / 32, pattern_table);
	vdp_set_sprites(vdp, 0, VDP_SPRITE_COUNT, sprite_table);
	vdp_set_cells(vdp, VDP_PLANE_A, 0, 0, PLANE_WIDTH, PLANE_HEIGHT, &plane_table[VDP_PLANE_A][0][0]);
	vdp_set_cells(vdp, VDP_PLANE_B, 0, 0, PLANE_WIDTH, PLANE_HEIGHT, &plane_table[VDP_PLANE_B][0][0]);
	vdp_set_cells(vdp, VDP_PLANE_W, 0, 0, PLANE_WIDTH, PLANE_HEIGHT, &plane_table[VDP_PLANE_W][0][0]);
	vdp_set_hscroll(vdp, VDP_PLANE_A, 0, VDP_HSCROLL_COUNT, &hscroll_table[VDP_PLANE_A][0]);
	vdp_set_vscroll(vdp, VDP_PLANE_A, 0, VDP_VSCROLL_COUNT, &vscroll_table[VDP_PLANE_A][0]);
	vdp_set_hscroll(vdp, VDP_PLANE_B, 0, VDP_HSCROLL_COUNT, &hscroll_table[VDP_PLANE_B][0]);
	vdp_set_vscroll(vdp, VDP_PLANE_B, 0, VDP_VSCROLL_COUNT, &vscroll_table[VDP_PLANE_B][0]);

	int xa = -88, ya = 24, xb = -96, yb = 16;

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

		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			xa -= 1;
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			xa += 1;
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			ya -= 1;
		}
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
			ya += 1;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			xb -= 1;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			xb += 1;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			yb -= 1;
		}
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			yb += 1;
		}

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		int zoom = imax(imin(width / VDP_FRAMEBUFFER_WIDTH, height / VDP_FRAMEBUFFER_HEIGHT), 1);
		int vdp_width = zoom * VDP_FRAMEBUFFER_WIDTH;
		int vdp_height = zoom * VDP_FRAMEBUFFER_HEIGHT;
		int vdp_x = (width - vdp_width) / 2;
		int vdp_y = (height - vdp_height) / 2;

		for (unsigned int i = 0; i < VDP_HSCROLL_COUNT; ++i) {
			hscroll_table[VDP_PLANE_A][i] = xa;
			hscroll_table[VDP_PLANE_B][i] = xb;
		}
		for (unsigned int i = 0; i < VDP_VSCROLL_COUNT; ++i) {
			vscroll_table[VDP_PLANE_A][i] = ya;
			vscroll_table[VDP_PLANE_B][i] = yb;
		}

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
