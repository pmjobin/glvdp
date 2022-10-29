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

#version 450 core

layout(location = 0) uniform bool intensity_mode;
layout(location = 1) uniform uint background_color;
layout(location = 2) uniform uvec2 plane_size;
layout(location = 3) uniform ivec2 window;

layout(binding = 0) uniform sampler1D color_table;
layout(binding = 1) uniform usampler2DArray pattern_table;
layout(binding = 2) uniform usampler1D sprite_table;
layout(binding = 3) uniform usampler2DArray plane_table;
layout(binding = 4) uniform usampler1DArray hscroll_table;
layout(binding = 5) uniform usampler1DArray vscroll_table;

out vec4 pixel;

const uvec2 pattern_size = uvec2(8, 8);
const uint priority_mask = 0x40;
const int max_sprite_count = 80;

uvec2 flip(uvec2 p, uvec2 size, bvec2 dir) {
	return mix(p, size - 1 - p, dir);
}

uint patternFetch(uvec2 p, uint cell) {
	uint pattern = bitfieldExtract(cell, 0, 11);
	uint palette = bitfieldExtract(cell, 13, 3);
	uint strip = texelFetch(pattern_table, ivec3(0, p.y & (pattern_size.y - 1), pattern), 0).r;
	return palette * 16 + bitfieldExtract(strip, int(p.x & (pattern_size.x - 1)) * 4 ^ 4, 4);
}

uint planeFetch(uvec2 p, uint layer) {
	uint cell = texelFetch(plane_table, ivec3(p / pattern_size % plane_size, layer), 0).r;
	uvec2 q = flip(p, pattern_size, bvec2(cell & 1u << 11, cell & 1u << 12));
	return patternFetch(q, cell);
}

uint spriteFetch(uvec2 p) {
	int i = 0;
	int link = 0;
	uint color = 0;
	do {
		uvec4 sprite = texelFetch(sprite_table, link, 0);
		link = int(bitfieldExtract(sprite.g, 0, 7));
		uvec2 size = ivec2(bitfieldExtract(sprite.g, 10, 2), bitfieldExtract(sprite.g, 8, 2)) * 8 + 8;
		uvec2 q = flip(p - uvec2(sprite.ar) + 128, size, bvec2(sprite.b & 1u << 11, sprite.b & 1u << 12));
		if (all(greaterThanEqual(q, ivec2(0, 0))) && all(lessThan(q, size))) {
			uint cell = sprite.b + (q.y / pattern_size.y) + (q.x / pattern_size.x) * (size.y / pattern_size.y);
			color = patternFetch(q, cell);
		}
	} while (i++ < max_sprite_count && link != 0 && (color & 0xFu) == 0);
	return color;
}

uvec2 scrollFetch(uvec2 p, int layer) {
	const uint c = pattern_size.x * 2;
	uint x = -texelFetch(hscroll_table, ivec2(p.y, layer), 0).r;
	uint y = texelFetch(vscroll_table, ivec2(max((int(p.x + ((x + c - 1) & (c - 1)) + 1)) / int(c) - 1, 0), layer), 0).r;
	return uvec2(x, y);
}

void main() {
	uvec2 p = uvec2(gl_FragCoord.x, 224 - gl_FragCoord.y);
	uvec2 scroll_a = scrollFetch(p, 0);
	uvec2 scroll_b = scrollFetch(p, 1);
	bool inside_window = window.x > 0 && p.x < window.x || window.x < 0 && p.x >= -window.x || window.y > 0 && p.y < window.y || window.y < 0 && p.y >= -window.y;
	uint color_a = inside_window ? planeFetch(p, 2) : planeFetch(p + scroll_a, 0);
	uint color_b = planeFetch(p + scroll_b, 1);
	uint color_s = spriteFetch(p);
	uint color = background_color;
	uint intensity = intensity_mode ? (color_a | color_b) & priority_mask : priority_mask;
	if ((color_b & 0xF) != 0) {
		color = color_b;
	}
	if ((color_a & 0xF) != 0 && (color_a & priority_mask) >= (color & priority_mask)) {
		color = color_a;
	}
	if ((color_s & 0xF) != 0 && (color_s & priority_mask) >= (color & priority_mask)) {
		if (intensity_mode) {
			if ((color_s & 0x3F) == 0x3E) {
				intensity += priority_mask;
			} else if ((color_s & 0x3F) == 0x3F) {
				intensity = 0;
			} else {
				color = color_s;
				intensity |= (color & 0xF) == 0xE ? priority_mask : color & priority_mask; // Emulate S&H color 14 bug...
			}
		} else {
			color = color_s;
		}
	}
	pixel = texelFetch(color_table, int(color & 0x3F | intensity), 0);
}
