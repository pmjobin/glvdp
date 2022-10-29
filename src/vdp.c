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

#include <vdp.h>

#include <GL/glcorearb.h>
#include <GL/gl3w.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static const GLchar vdp_geometry_glsl[] = {
#include "vdp.geometry.glsl.i"
};

static const GLchar vdp_vertex_glsl[] = {
#include "vdp.vertex.glsl.i"
};

static const GLchar vdp_fragment_glsl[] = {
#include "vdp.fragment.glsl.i"
};

struct vdp_context {
	GLuint program;
	GLuint vao;
	GLuint color_tex;
	GLuint pattern_tex;
	GLuint sprite_tex;
	GLuint plane_tex;
	GLuint hscroll_tex;
	GLuint vscroll_tex;
	GLuint framebuffer_tex;
	GLuint framebuffer_fbo;
};

static GLuint create_shader_from_source(GLenum type, const GLchar *source) {
	GLuint shader = glCreateShader(type);
	GLint size = (GLint)strlen(source);
	glShaderSource(shader, 1, &source, &size);
	glCompileShader(shader);
	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
		GLchar *log = malloc((size_t)size);
		glGetShaderInfoLog(shader, size, &size, log);
		fprintf(stderr, "%s\n", log);
		free(log);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

static GLuint create_program_from_source(const GLenum types[], const GLchar *sources[], GLuint num) {
	GLuint program = glCreateProgram();
	for (GLuint i = 0; i < num; ++i) {
		GLuint shader = create_shader_from_source(types[i], sources[i]);
		if (!shader) {
			glDeleteProgram(program);
			return 0;
		}
		glAttachShader(program, shader);
		glDeleteShader(shader);
	}
	glLinkProgram(program);
	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		GLint size;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &size);
		GLchar *log = malloc((size_t)size);
		glGetProgramInfoLog(program, size, &size, log);
		fprintf(stderr, "%s\n", log);
		glDeleteProgram(program);
		return 0;
	}
	return program;
}

vdp_context_t *vdp_create_context() {
	vdp_context_t *context = calloc(1, sizeof (vdp_context_t));

	// program
	GLenum shader_types[] = { GL_GEOMETRY_SHADER, GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
	const GLchar *shader_sources[] = { vdp_geometry_glsl, vdp_vertex_glsl, vdp_fragment_glsl };
	context->program = create_program_from_source(shader_types, shader_sources, 3);
	if (!context->program) {
		vdp_destroy_context(context);
		return NULL;
	}

	// vertex array object
	glCreateVertexArrays(1, &context->vao);

	// color palette texture
	glCreateTextures(GL_TEXTURE_1D, 1, &context->color_tex);
	glTextureStorage1D(context->color_tex, 1, GL_RGBA8, VDP_COLOR_COUNT * 4);
	glTextureParameteri(context->color_tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(context->color_tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// pattern texture
	glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &context->pattern_tex);
	glTextureStorage3D(context->pattern_tex, 1, GL_R32UI, (VDP_PATTERN_WIDTH * VDP_PATTERN_BPP) / 32, VDP_PATTERN_HEIGHT, VDP_PATTERN_COUNT);
	glTextureParameteri(context->pattern_tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(context->pattern_tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// sprite texture
	glCreateTextures(GL_TEXTURE_1D, 1, &context->sprite_tex);
	glTextureStorage1D(context->sprite_tex, 1, GL_RGBA16UI, VDP_SPRITE_COUNT);
	glTextureParameteri(context->sprite_tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(context->sprite_tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// plane texture
	glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &context->plane_tex);
	glTextureStorage3D(context->plane_tex, 1, GL_R16UI, VDP_PLANE_MAX_WIDTH, VDP_PLANE_MAX_HEIGHT, VDP_PLANE_COUNT);
	glTextureParameteri(context->plane_tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(context->plane_tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// horizontal scroll texture
	glCreateTextures(GL_TEXTURE_1D_ARRAY, 1, &context->hscroll_tex);
	glTextureStorage2D(context->hscroll_tex, 1, GL_R16UI, VDP_HSCROLL_COUNT, 2);
	glTextureParameteri(context->hscroll_tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(context->hscroll_tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// vertical scroll texture
	glCreateTextures(GL_TEXTURE_1D_ARRAY, 1, &context->vscroll_tex);
	glTextureStorage2D(context->vscroll_tex, 1, GL_R16UI, VDP_VSCROLL_COUNT, 2);
	glTextureParameteri(context->vscroll_tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(context->vscroll_tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// framebuffer texture
	glCreateTextures(GL_TEXTURE_2D, 1, &context->framebuffer_tex);
	glTextureStorage2D(context->framebuffer_tex, 1, GL_RGBA8, VDP_FRAMEBUFFER_WIDTH, VDP_FRAMEBUFFER_HEIGHT);
	glTextureParameteri(context->framebuffer_tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(context->framebuffer_tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// framebuffer object
	glCreateFramebuffers(1, &context->framebuffer_fbo);
	glNamedFramebufferTexture(context->framebuffer_fbo, GL_COLOR_ATTACHMENT0, context->framebuffer_tex, 0);
	GLenum fbstatus = glCheckNamedFramebufferStatus(context->framebuffer_fbo, GL_FRAMEBUFFER);
	assert(fbstatus == GL_FRAMEBUFFER_COMPLETE);

	return context;
}

void vdp_destroy_context(vdp_context_t *context) {
	if (context) {
		glDeleteProgram(context->program);
		glDeleteVertexArrays(1, &context->vao);
		glDeleteTextures(1, &context->color_tex);
		glDeleteTextures(1, &context->pattern_tex);
		glDeleteTextures(1, &context->sprite_tex);
		glDeleteTextures(1, &context->plane_tex);
		glDeleteTextures(1, &context->hscroll_tex);
		glDeleteTextures(1, &context->vscroll_tex);
		glDeleteFramebuffers(1, &context->framebuffer_fbo);
		glDeleteTextures(1, &context->framebuffer_tex);
		free(context);
	}
}

void vdp_set_mode(vdp_context_t *context, vdp_mode_t mode) {
	glProgramUniform1ui(context->program, 0, mode == VDP_MODE_INTENSITY);
}

void vdp_set_background_color(vdp_context_t *context, unsigned int i) {
	glProgramUniform1ui(context->program, 1, i);
}

void vdp_set_plane_size(vdp_context_t *context, unsigned int width, unsigned int height) {
	glProgramUniform2ui(context->program, 2, width, height);
}

void vdp_set_window_coord(vdp_context_t *context, int x, int y) {
	glProgramUniform2i(context->program, 3, x, y);
}

void vdp_set_colors(vdp_context_t *context, unsigned int start, unsigned int count, const vdp_color_t *data) {
	glTextureSubImage1D(context->color_tex, 0, (GLint)start, (GLsizei)count, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

void vdp_set_colors_sh(vdp_context_t *context, unsigned int start, unsigned int count, const vdp_color_t *data) {
	vdp_color_t shadow[64];
	vdp_color_t highlight[64];
	for (unsigned int i = 0; i < count && i < 64; ++i) {
		shadow[i].r = data[i].r / 2;
		shadow[i].g = data[i].g / 2;
		shadow[i].b = data[i].b / 2;
		highlight[i].r = data[i].r / 2 + 128;
		highlight[i].g = data[i].g / 2 + 128;
		highlight[i].b = data[i].b / 2 + 128;
	}
	vdp_set_colors(context, start + 0, count, shadow);
	vdp_set_colors(context, start + 64, count, data);
	vdp_set_colors(context, start + 128, count, highlight);
}

void vdp_set_patterns(vdp_context_t *context, unsigned int start, unsigned int count, const uint32_t *data) {
	glTextureSubImage3D(context->pattern_tex, 0, 0, 0, (GLint)start, (VDP_PATTERN_WIDTH * VDP_PATTERN_BPP) / 32, VDP_PATTERN_HEIGHT, (GLsizei)count, GL_RED_INTEGER, GL_UNSIGNED_INT, data);
}

void vdp_set_sprites(vdp_context_t *context, unsigned int start, unsigned int count, const vdp_sprite_t *data) {
	glTextureSubImage1D(context->sprite_tex, 0, (GLint)start, (GLsizei)count, GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, data);
}

void vdp_set_cells(vdp_context_t *context, vdp_plane_t plane, unsigned int x, unsigned int y, unsigned int width, unsigned int height, const vdp_cell_t *data) {
	glTextureSubImage3D(context->plane_tex, 0, (GLint)x, (GLint)y, (GLint)plane, (GLsizei)width, (GLsizei)height, 1, GL_RED_INTEGER, GL_UNSIGNED_SHORT, data);
}

void vdp_set_hscroll(vdp_context_t *context, vdp_plane_t plane, unsigned int start, unsigned int count, const uint16_t *data) {
	glTextureSubImage2D(context->hscroll_tex, 0, (GLint)start, (GLint)plane, (GLsizei)count, 1, GL_RED_INTEGER, GL_UNSIGNED_SHORT, data);
}

void vdp_set_vscroll(vdp_context_t *context, vdp_plane_t plane, unsigned int start, unsigned int count, const uint16_t *data) {
	glTextureSubImage2D(context->vscroll_tex, 0, (GLint)start, (GLint)plane, (GLsizei)count, 1, GL_RED_INTEGER, GL_UNSIGNED_SHORT, data);
}

void vdp_render(vdp_context_t *context) {
	glUseProgram(context->program);
	glBindVertexArray(context->vao);
	glBindFramebuffer(GL_FRAMEBUFFER, context->framebuffer_fbo);
	glBindTextureUnit(0, context->color_tex);
	glBindTextureUnit(1, context->pattern_tex);
	glBindTextureUnit(2, context->sprite_tex);
	glBindTextureUnit(3, context->plane_tex);
	glBindTextureUnit(4, context->hscroll_tex);
	glBindTextureUnit(5, context->vscroll_tex);

	glViewport(0, 0, VDP_FRAMEBUFFER_WIDTH, VDP_FRAMEBUFFER_HEIGHT);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_POINTS, 0, 1);

	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTextureUnit(0, 0);
	glBindTextureUnit(1, 0);
	glBindTextureUnit(2, 0);
	glBindTextureUnit(3, 0);
	glBindTextureUnit(4, 0);
	glBindTextureUnit(5, 0);
}

void vdp_blit(vdp_context_t *context, unsigned int x, unsigned int y, unsigned int width, unsigned int height, vdp_filter_t filter) {
	glBlitNamedFramebuffer(context->framebuffer_fbo, 0, 0, 0, VDP_FRAMEBUFFER_WIDTH, VDP_FRAMEBUFFER_HEIGHT, (GLint)x, (GLint)y, (GLint)(x + width), (GLint)(y + height), GL_COLOR_BUFFER_BIT, filter == VDP_FILTER_BILINEAR ? GL_LINEAR : GL_NEAREST);
}
