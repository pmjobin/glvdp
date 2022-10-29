#include <GL/gl3w.h>
#include <SDL.h>
#include "vdp.h"
#include "../../include/vdp.h"

void run_gl_vdp(vdp_context *vdp) {
	static SDL_Window *window = NULL;
	static SDL_GLContext *context = NULL;
	if (!window) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

		window = SDL_CreateWindow("GL VDP", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 448, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
		context = SDL_GL_CreateContext(window);
		SDL_GL_MakeCurrent(window, context);
		gl3wInit();
	}
	SDL_GL_MakeCurrent(window, context);

	static vdp_context_t *glvdp = NULL;
	if (!glvdp) {
		glvdp = vdp_create_context();
	}
	glClearColor(1.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	vdp_color_t colors[64];
	for (size_t i = 0; i < 64; ++i) {
		uint8_t r = vdp->cram[i] >> 1 & 7;
		uint8_t g = vdp->cram[i] >> 5 & 7;
		uint8_t b = vdp->cram[i] >> 9 & 7;
		colors[i].r = r << 5 | r << 2 | r >> 1;
		colors[i].g = g << 5 | g << 2 | g >> 1;
		colors[i].b = b << 5 | b << 2 | b >> 1;
	}
	unsigned int hsize, vsize;
	switch (vdp->regs[REG_SCROLL] & 3) {
	case 0:
		hsize = 32;
		break;
	case 1:
		hsize = 64;
		break;
	default:
		hsize = 128;
		break;
	}
	switch ((vdp->regs[REG_SCROLL] >> 4) & 3) {
	case 0:
		vsize = 32;
		break;
	case 1:
		vsize = 64;
		break;
	default:
		vsize = 128;
		break;
	}
	size_t plane_a = (vdp->regs[REG_SCROLL_A] & 0x38) << 10;
	size_t plane_b = (vdp->regs[REG_SCROLL_B] & 0x7) << 13;
	size_t plane_w = (vdp->regs[REG_WINDOW] & 0x3C) << 10;
	size_t hscroll_addr = (vdp->regs[REG_HSCROLL] & 0x3F) << 10;
	size_t sprite_addr = (vdp->regs[REG_SAT] & 0x7E) << 9;
	int window_h = (vdp->regs[REG_WINDOW_H] & 0x1F) * ((vdp->regs[REG_WINDOW_H] & 0x80) ? -16 : 16);
	int window_v = (vdp->regs[REG_WINDOW_V] & 0x1F) * ((vdp->regs[REG_WINDOW_V] & 0x80) ? -8 : 8);

	uint8_t vdpmem[128 * 1024];
	for (size_t i = 0; i < 64 * 1024; ++i) {
		vdpmem[i] = vdp->vdpmem[i ^ 1];
	}

	uint16_t hscroll[2][VDP_HSCROLL_COUNT];
	uint16_t *hs = (uint16_t *)&vdpmem[hscroll_addr];
	switch (vdp->regs[REG_MODE_3] & 3) {
	case 0:
		for (size_t i = 0; i < VDP_HSCROLL_COUNT; ++i) {
			hscroll[VDP_PLANE_A][i] = hs[0];
			hscroll[VDP_PLANE_B][i] = hs[1];
		}
		break;

	case 2:
		for (size_t i = 0; i < VDP_HSCROLL_COUNT; ++i) {
			hscroll[VDP_PLANE_A][i] = hs[(i & ~7) * 2 + 0];
			hscroll[VDP_PLANE_B][i] = hs[(i & ~7) * 2 + 1];
		}
		break;

	default:
		for (size_t i = 0; i < VDP_HSCROLL_COUNT; ++i) {
			hscroll[VDP_PLANE_A][i] = hs[i * 2 + 0];
			hscroll[VDP_PLANE_B][i] = hs[i * 2 + 1];
		}
		break;
	}

	uint16_t vscroll[2][VDP_VSCROLL_COUNT];
	if (vdp->regs[REG_MODE_3] & 4) {
		for (size_t i = 0; i < 20; ++i) {
			vscroll[VDP_PLANE_A][i] = vdp->vsram[i * 2 + 0];
			vscroll[VDP_PLANE_B][i] = vdp->vsram[i * 2 + 1];
		}
	} else {
		for (size_t i = 0; i < 20; ++i) {
			vscroll[VDP_PLANE_A][i] = vdp->vsram[0];
			vscroll[VDP_PLANE_B][i] = vdp->vsram[1];
		}
	}

	vdp_set_mode(glvdp, (vdp->regs[REG_MODE_4] & BIT_HILIGHT) ? VDP_MODE_INTENSITY : VDP_MODE_NORMAL);
	vdp_set_background_color(glvdp, vdp->regs[REG_BG_COLOR]);
	vdp_set_plane_size(glvdp, hsize, vsize);
	vdp_set_window_coord(glvdp, window_h, window_v);
	vdp_set_colors_sh(glvdp, 0, 64, colors);
	vdp_set_patterns(glvdp, 0, VDP_PATTERN_COUNT, (uint32_t *)&vdp->vdpmem[0]);

	vdp_set_sprites(glvdp, 0, VDP_SPRITE_COUNT, (vdp_sprite_t *)&vdpmem[sprite_addr]);
	vdp_set_cells(glvdp, VDP_PLANE_A, 0, 0, hsize, vsize, (vdp_cell_t *)&vdpmem[plane_a]);
	vdp_set_cells(glvdp, VDP_PLANE_B, 0, 0, hsize, vsize, (vdp_cell_t *)&vdpmem[plane_b]);
	vdp_set_cells(glvdp, VDP_PLANE_W, 0, 0, hsize, vsize, (vdp_cell_t *)&vdpmem[plane_w]);
	vdp_set_hscroll(glvdp, VDP_PLANE_A, 0, VDP_HSCROLL_COUNT, hscroll[VDP_PLANE_A]);
	vdp_set_vscroll(glvdp, VDP_PLANE_A, 0, VDP_VSCROLL_COUNT, vscroll[VDP_PLANE_A]);
	vdp_set_hscroll(glvdp, VDP_PLANE_B, 0, VDP_HSCROLL_COUNT, hscroll[VDP_PLANE_B]);
	vdp_set_vscroll(glvdp, VDP_PLANE_B, 0, VDP_VSCROLL_COUNT, vscroll[VDP_PLANE_B]);
	vdp_render(glvdp);
	vdp_blit(glvdp, 0, 0, 640, 448, VDP_FILTER_NEAREST);
	SDL_GL_SwapWindow(window);
	SDL_GL_MakeCurrent(NULL, NULL);
}
