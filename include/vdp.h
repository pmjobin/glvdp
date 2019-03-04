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

#pragma once

#include <stdint.h>

typedef struct vdp_context vdp_context_t;

enum {
	VDP_FRAMEBUFFER_WIDTH = 320,
	VDP_FRAMEBUFFER_HEIGHT = 224,
	VDP_COLOR_COUNT = 64,
	VDP_PATTERN_WIDTH = 8,
	VDP_PATTERN_HEIGHT = 8,
	VDP_PATTERN_BPP = 4,
	VDP_PATTERN_COUNT = 2048,
	VDP_PLANE_MAX_WIDTH = 128,
	VDP_PLANE_MAX_HEIGHT = 128,
	VDP_PLANE_COUNT = 3,
	VDP_SPRITE_COUNT = 128,
	VDP_HSCROLL_COUNT = 256,
	VDP_VSCROLL_COUNT = VDP_FRAMEBUFFER_WIDTH / VDP_PATTERN_WIDTH / 2,
};

typedef enum vdp_mode {
	VDP_MODE_NORMAL,
	VDP_MODE_INTENSITY
} vdp_mode_t;

typedef enum vdp_plane {
	VDP_PLANE_A,
	VDP_PLANE_B,
	VDP_PLANE_W
} vdp_plane_t;

typedef enum vdp_filter {
	VDP_FILTER_NEAREST,
	VDP_FILTER_BILINEAR
} vdp_filter_t;

typedef union vdp_color {
	struct {
		uint8_t r;
		uint8_t g;
		uint8_t b;
	};
	uint32_t rgb;
} vdp_color_t;

typedef struct vdp_cell {
	uint16_t pattern  : 11;
	uint16_t hflip    : 1;
	uint16_t vflip    : 1;
	uint16_t palette  : 2;
	uint16_t priority : 1;
} vdp_cell_t;

typedef struct vdp_sprite {
	uint16_t y;
	uint16_t link     : 8;
	uint16_t vsize    : 2;
	uint16_t hsize    : 2;
	uint16_t          : 4;
	uint16_t pattern  : 11;
	uint16_t hflip    : 1;
	uint16_t vflip    : 1;
	uint16_t palette  : 2;
	uint16_t priority : 1;
	uint16_t x;
} vdp_sprite_t;

#ifdef __cplusplus
extern "C" {
#endif

vdp_context_t *vdp_create_context();
void vdp_destroy_context(vdp_context_t *context);

void vdp_set_mode(vdp_context_t *context, vdp_mode_t mode);
void vdp_set_background_color(vdp_context_t *context, unsigned int i);
void vdp_set_plane_size(vdp_context_t *context, unsigned int width, unsigned int height);
void vdp_set_window_coord(vdp_context_t *context, int x, int y);

void vdp_set_colors(vdp_context_t *context, unsigned int start, unsigned int count, const vdp_color_t *data);
void vdp_set_colors_sh(vdp_context_t *context, unsigned int start, unsigned int count, const vdp_color_t *data);
void vdp_set_patterns(vdp_context_t *context, unsigned int start, unsigned int count, const uint32_t *data);
void vdp_set_sprites(vdp_context_t *context, unsigned int start, unsigned int count, const vdp_sprite_t *data);
void vdp_set_cells(vdp_context_t *context, vdp_plane_t plane, unsigned int x, unsigned int y, unsigned int width, unsigned int height, const vdp_cell_t *data);
void vdp_set_hscroll(vdp_context_t *context, vdp_plane_t plane, unsigned int start, unsigned int count, const uint16_t *data);
void vdp_set_vscroll(vdp_context_t *context, vdp_plane_t plane, unsigned int start, unsigned int count, const uint16_t *data);

void vdp_render(vdp_context_t *context);
void vdp_blit(vdp_context_t *context, unsigned int x, unsigned int y, unsigned int width, unsigned int height, vdp_filter_t filter);

#ifdef __cplusplus
}
#endif
