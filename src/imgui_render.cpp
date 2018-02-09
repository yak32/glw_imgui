//
// Copyright (c) 2009 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
#define _USE_MATH_DEFINES
#include <math.h>

#define STBTT_ifloor(x) ((int)::floor(x))
#define STBTT_iceil(x) ((int)::ceil(x))

// this header should be before mathematics.h

//#define STBTT_malloc(x,y)    malloc(x,y)
//#define STBTT_free(x,y)      free(x,y)
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_memcpy memcpy
#define STBTT_memset memset

#include "imgui_ui.h"
#include "imgui_platform.h"
#include "texture_atlas/texture_atlas.h"
#include "stb/stb_truetype.h"

namespace imgui {

namespace {

const uint TEXTURE_ATLAS_SIZE = 2048;
const unsigned TEMP_COORD_COUNT = 1000;
float g_tempCoords[TEMP_COORD_COUNT * 2];
float g_tempNormals[TEMP_COORD_COUNT * 2];

render_vertex_3d_t new_coords[TEMP_COORD_COUNT * 2 * 6];

const int CIRCLE_VERTS = 8 * 4;
float g_circleVerts[CIRCLE_VERTS * 2];
const float MAX_UI_DEPTH = 256.0f; // to match z buffer depth
}

struct font_t {
	stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs
	uint id;
	uint height;
};

inline void set(render_vertex_3d_t* v, float x, float y, unsigned int col, float depth) {
	v->x = x;
	v->y = y;
	v->u = 0.999f;
	v->v = 0.999f;
	v->z = depth;
	v->clr = col;
}
inline void set(render_vertex_3d_t* v, float x, float y, float u, float vv, unsigned int col,
				float depth) {
	v->x = x;
	v->y = y;
	v->z = depth;
	v->u = u;
	v->v = vv;
	v->clr = col;
}

void draw_rect(IRenderer* r, float x, float y, float w, float h, float texture_x, float texture_y,
			   float texture_width, float texture_height, unsigned int col, float depth,
			   bool flip_texture);

void draw_line(IRenderer* r, float x1, float y1, float x2, float y2, unsigned int col, float depth);

void draw_rounded_rect(IRenderer* render, float x, float y, float w, float h, float r, float fth,
					   unsigned int col, float depth);
void render_mesh(IRenderer* renderer, const float* coords, float txt_shift_x, float txt_shift_y,
				 float txt_scale_x, float txt_scale_y, unsigned numCoords, float r,
				 unsigned int col, float depth);
bool Ui::render_init(IRenderer* r) {
	_renderer = r;

	_texture_atlas = new TextureAtlas;

	for (unsigned i = 0; i < CIRCLE_VERTS; ++i) {
		float a = (float)i / (float)CIRCLE_VERTS * (float)M_PI * 2;
		g_circleVerts[i * 2 + 0] = cosf(a);
		g_circleVerts[i * 2 + 1] = sinf(a);
	}

	unsigned int white = 0xffffffff;

	_white_texture = {0};
	_white_texture.width = 0.0f;
	_white_texture.height = 0.0f;

	// create batch texture
	if (_mode & MODE_BATCH_DRAW_CALLS) {
		_texture_atlas->create(TEXTURE_ATLAS_SIZE, TEXTURE_ATLAS_SIZE);
		_atlas = r->create_texture(TEXTURE_ATLAS_SIZE, TEXTURE_ATLAS_SIZE, 4, nullptr);

		unsigned int x, y;
		_texture_atlas->add_box(1, 1, &x, &y);
		if (!r->copy_sub_texture(_atlas, x, y, 1, 1, &white)) {
			IMGUI_LOG_ERROR("failed to create white texture");
			return false;
		}
		_white_texture.x = x + 0.5f / TEXTURE_ATLAS_SIZE;
		_white_texture.y = y + 0.5f / TEXTURE_ATLAS_SIZE;
		_white_texture.width = _white_texture.height = 0.0f;
		_white_texture.id = _atlas;
		return _atlas != UNDEFINED_TEXTURE;
	}
	_white_texture.id = r->create_texture(1, 1, 4, &white);
	_white_texture.width = 1.0f;
	_white_texture.height = 1.0f;
	return _white_texture.id != UNDEFINED_TEXTURE;
}
bool Ui::render_destroy() {
	if (!_renderer)
		return false;

	if (_mode & MODE_BATCH_DRAW_CALLS) {
		if (_atlas != UNDEFINED_TEXTURE)
			_renderer->remove_texture(_atlas);
	}
	else if (_white_texture.id != UNDEFINED_TEXTURE) {
		_renderer->remove_texture(_white_texture.id);
	}

	delete _texture_atlas;
	_texture_atlas = nullptr;

	for (auto& font : _fonts) delete font.second;

	_fonts.clear();
	return true;
}
bool Ui::bind_texture(const char* path) {
	auto it_texture = _textures.find(path);
	if (it_texture != _textures.end()) {
		_current_texture = it_texture->second;
		return true;
	};
	IRenderer* r = _renderer;

	texture_t texture;
	texture.id = UNDEFINED_TEXTURE;
	if (_mode & MODE_BATCH_DRAW_CALLS) {
		// put image into atlas
		int width, height, channels;
		unsigned int x, y;
		unsigned char* image = r->load_image(path, &width, &height, &channels);
		if (image && channels == 4) {
			_texture_atlas->add_box(width, height, &x, &y);
			r->copy_sub_texture(_atlas, x, y, width, height, image);

			texture.x = (float)x / TEXTURE_ATLAS_SIZE;
			texture.y = (float)y / TEXTURE_ATLAS_SIZE;
			texture.width = (float)width / TEXTURE_ATLAS_SIZE;
			texture.height = (float)height / TEXTURE_ATLAS_SIZE;
		}
	}
	else {
		int width, height, channels;
		unsigned char* image = r->load_image(path, &width, &height, &channels);
		if (!image){
			IMGUI_LOG_ERROR("failed to load texture");
			return false;
		}
		texture.id = r->create_texture(width, height, channels, image);
		texture.x = 0.0f;
		texture.y = 0.0f;
		texture.width = 1.0f;
		texture.height = 1.0f;
	}
	_textures[path] = texture;
	_current_texture = texture;
	return texture.id != UNDEFINED_TEXTURE;
}
void Ui::render_draw(bool transparency) {
	if (!_renderer)
		return; // not yet initialized

	IRenderer* r = _renderer;
	r->begin(_width, _height);

	uint size;
	const gfx_cmd* cmd = get_render_queue(size);
	_current_texture = _white_texture;

	bool bind_font = true;
	for (int i = 0; i < size; ++i, ++cmd) {
		switch (cmd->type) {
		case GFX_CMD_RECT:
			r->bind_texture(_current_texture.id);
			r->set_blend_mode(BLEND_RECT);
			// if (!g_blend_texture)
			//	r->set_state(RS_BLEND, RF_FALSE);

			if (cmd->rect.r == 0) {
				draw_rect(r, (float)cmd->rect.x, (float)cmd->rect.y, (float)cmd->rect.w,
						  (float)cmd->rect.h, _current_texture.x, _current_texture.y,
						  _current_texture.width, _current_texture.height, cmd->col, _depth, false);
			}
			else {
				draw_rounded_rect(r, (float)cmd->rect.x, (float)cmd->rect.y, (float)cmd->rect.w,
								  (float)cmd->rect.h, (float)cmd->rect.r, 1.0f, cmd->col, _depth);
			}
			break;

		case GFX_CMD_LINE:
			r->bind_texture(_current_texture.id);
			r->set_blend_mode(BLEND_RECT);
			// if (!g_blend_texture)
			//	r->set_state(RS_BLEND, RF_FALSE);

			draw_line(r, (float)cmd->line.x1, (float)cmd->line.y1, (float)cmd->line.x2,
							  (float)cmd->line.y2, cmd->col, _depth);
			break;


		case GFX_CMD_TRIANGLE:
			r->set_blend_mode(BLEND_RECT);
			r->bind_texture(_white_texture.id);
			if (cmd->flags == 1) {
				const float verts[3 * 2] = {
					(float)cmd->rect.x,
					(float)cmd->rect.y,
					(float)cmd->rect.x + (float)cmd->rect.w,
					(float)cmd->rect.y + (float)cmd->rect.h / 2,
					(float)cmd->rect.x,
					(float)cmd->rect.y + (float)cmd->rect.h,
				};
				render_mesh(r, verts, 0, 0, 1.0f, 1.0f, 3, 1.0f, cmd->col, _depth);
			}
			if (cmd->flags == 2) {
				const float verts[3 * 2] = {
					(float)cmd->rect.x,
					(float)cmd->rect.y + (float)cmd->rect.h,
					(float)cmd->rect.x + (float)cmd->rect.w / 2,
					(float)cmd->rect.y,
					(float)cmd->rect.x + (float)cmd->rect.w,
					(float)cmd->rect.y + (float)cmd->rect.h,
				};
				render_mesh(r, verts, 0, 0, 1.0f, 1.0f, 3, 1.0f, cmd->col, _depth);
			}
			break;

		case GFX_CMD_DEPTH:
// after receiving depth command, setup depth testing (disabled by default)
#ifdef IMGUI_RIGHTHANDED
			_depth = 1.0f - (float)cmd->depth.z / MAX_UI_DEPTH;
#else
			_depth = (float)(MAX_UI_LAYER_COUNT - cmd->depth.z) / MAX_UI_DEPTH;
#endif
			break;

		case GFX_CMD_SCISSOR:
			if (cmd->flags) {
				r->set_scissor(cmd->rect.x, (cmd->rect.y), cmd->rect.w, cmd->rect.h, true);
			}
			else
				r->set_scissor(0, 0, 0, 0, false);
			break;

		case GFX_CMD_TEXT:
			r->bind_texture(_current_font->second->id);
			r->set_blend_mode(BLEND_TEXT);
			render_text(*_current_font->second, cmd->text.x, cmd->text.y, cmd->text.width,
						cmd->text.height, cmd->text.text, cmd->text.align, cmd->col, _depth);
			break;

		case GFX_CMD_TEXTURE:
			if (cmd->texture.path)
				bind_texture(cmd->texture.path);
			else
				_current_texture = _white_texture;
			// manager
			break;

		case GFX_CMD_FONT:
			if (cmd->font.path) {
				char font_name[256];
				snprintf(font_name, sizeof(font_name), "%s:%f", cmd->font.path, cmd->font.height);
				auto font = _fonts.find(font_name);
				if (font != _fonts.end())
					_current_font = font;
				else {
					font_t* f = new font_t;
					if (load_font(cmd->font.path, cmd->font.height, *f)) {
						_fonts[font_name] = f;
						_current_font = _fonts.find(font_name); // TODO: IMPROVE
					}
					else
						delete f;
				}
			}
			break;
		}
	}
	r->end();
	on_render_finished();
}

// simple drawing, without any alpha blending
void render_quads(IRenderer* r, const float* coords, float texture_x, float texture_y,
				  float texture_scale_x, float texture_scale_y, unsigned numCoords,
				  unsigned int col, float depth) {
	if (numCoords > TEMP_COORD_COUNT)
		numCoords = TEMP_COORD_COUNT;

	render_vertex_3d_t* v = &new_coords[0];
	for (unsigned i = 2; i < numCoords; ++i) {
		set(v++, coords[0], coords[1], col, depth);
		set(v++, coords[(i - 1) * 2], coords[(i - 1) * 2 + 1], col, depth);
		set(v++, coords[i * 2], coords[i * 2 + 1], col, depth);
	}
	int k = (int)(v - &new_coords[0]);
	for (int i = 0; i < k; ++i) {
		new_coords[i].u = (new_coords[i].u - texture_x) / texture_scale_x;
		new_coords[i].v = 1.0f - (new_coords[i].v - texture_y) / texture_scale_y;
	}
	r->render_mesh(new_coords, k, true);
}
void render_mesh(IRenderer* renderer, const float* coords, float txt_shift_x, float txt_shift_y,
				 float txt_scale_x, float txt_scale_y, unsigned numCoords, float r,
				 unsigned int col, float depth) {
	if (numCoords > TEMP_COORD_COUNT)
		numCoords = TEMP_COORD_COUNT;

	for (unsigned i = 0, j = numCoords - 1; i < numCoords; j = i++) {
		const float* v0 = &coords[j * 2];
		const float* v1 = &coords[i * 2];
		float dx = v1[0] - v0[0];
		float dy = v1[1] - v0[1];
		float d = sqrtf(dx * dx + dy * dy);
		if (d > 0) {
			d = 1.0f / d;
			dx *= d;
			dy *= d;
		}
		g_tempNormals[j * 2 + 0] = dy;
		g_tempNormals[j * 2 + 1] = -dx;
	}
	for (unsigned i = 0, j = numCoords - 1; i < numCoords; j = i++) {
		float dlx0 = g_tempNormals[j * 2 + 0];
		float dly0 = g_tempNormals[j * 2 + 1];
		float dlx1 = g_tempNormals[i * 2 + 0];
		float dly1 = g_tempNormals[i * 2 + 1];
		float dmx = (dlx0 + dlx1) * 0.5f;
		float dmy = (dly0 + dly1) * 0.5f;
		float dmr2 = dmx * dmx + dmy * dmy;
		if (dmr2 > 0.000001f) {
			float scale = 1.0f / dmr2;
			if (scale > 10.0f)
				scale = 10.0f;
			dmx *= scale;
			dmy *= scale;
		}
		g_tempCoords[i * 2 + 0] = coords[i * 2 + 0] + dmx * r;
		g_tempCoords[i * 2 + 1] = coords[i * 2 + 1] + dmy * r;
	}

	unsigned int colTrans = col & 0x00ffffff;

	render_vertex_3d_t* v = &new_coords[0];
	for (unsigned i = 0, j = numCoords - 1; i < numCoords; j = i++) {
		// 1st triangle
		set(v++, coords[i * 2], coords[i * 2 + 1], col, depth);
		set(v++, coords[j * 2], coords[j * 2 + 1], col, depth);
		set(v++, g_tempCoords[j * 2], g_tempCoords[j * 2 + 1], colTrans, depth);

		set(v++, g_tempCoords[j * 2], g_tempCoords[j * 2 + 1], colTrans, depth);
		set(v++, g_tempCoords[i * 2], g_tempCoords[i * 2 + 1], colTrans, depth);
		set(v++, coords[i * 2], coords[i * 2 + 1], col, depth);
	}
	for (unsigned i = 2; i < numCoords; ++i) {
		set(v++, coords[0], coords[1], col, depth);
		set(v++, coords[(i - 1) * 2], coords[(i - 1) * 2 + 1], col, depth);
		set(v++, coords[i * 2], coords[i * 2 + 1], col, depth);
	}
	int k = (int)(v - &new_coords[0]);
	for (int i = 0; i < k; ++i) {
		new_coords[k].u = (new_coords[k].x - txt_shift_x) / txt_scale_x;
		new_coords[k].v = 1.0f - (new_coords[k].y - txt_shift_y) / txt_scale_y;
		new_coords[k].clr = col;
	}
	renderer->render_mesh(new_coords, k, true);
}

void draw_rect(IRenderer* r, float x, float y, float w, float h, float texture_x, float texture_y,
			   float texture_width, float texture_height, unsigned int col, float depth,
			   bool flip_texture) {
	render_vertex_3d_t* v = &new_coords[0];
	if (flip_texture) {
		set(v++, x, y, texture_x, texture_y, col, depth);
		set(v++, x + w, y, texture_x + texture_width, texture_y, col, depth);
		set(v++, x + w, y + h, texture_x + texture_width, texture_y + texture_height, col, depth);

		set(v++, x, y, texture_x, texture_y, col, depth);
		set(v++, x + w, y + h, texture_x + texture_width, texture_y + texture_height, col, depth);
		set(v++, x, y + h, texture_x, texture_y + texture_height, col, depth);
		r->render_mesh(new_coords, 6, true);
	}
	else {
		set(v++, x, y, texture_x, texture_y + texture_height, col, depth);
		set(v++, x + w, y, texture_x + texture_width, texture_y + texture_height, col, depth);
		set(v++, x + w, y + h, texture_x + texture_width, texture_y, col, depth);

		set(v++, x, y, texture_x, texture_y + texture_height, col, depth);
		set(v++, x + w, y + h, texture_x + texture_width, texture_y, col, depth);
		set(v++, x, y + h, texture_x, texture_y, col, depth);
		r->render_mesh(new_coords, 6, true);
	}
}
void draw_ellipse(IRenderer* r, float x, float y, float w, float h, float fth, unsigned int col,
				  float depth) {
	float verts[CIRCLE_VERTS * 2];
	const float* cverts = g_circleVerts;
	float* v = verts;
	for (unsigned i = 0; i < CIRCLE_VERTS; ++i) {
		*v++ = x + cverts[i * 2] * w;
		*v++ = y + cverts[i * 2 + 1] * h;
	}
	render_mesh(r, verts, x, y, w, h, CIRCLE_VERTS, fth, col, depth);
}
void draw_rounded_rect(IRenderer* render, float x, float y, float w, float h, float r, float fth,
					   unsigned int col, float depth) {
	const unsigned n = CIRCLE_VERTS / 4;
	float verts[(n + 1) * 4 * 2];
	const float* cverts = g_circleVerts;
	float* v = verts;
	for (unsigned i = 0; i <= n; ++i) {
		*v++ = x + w - r + cverts[i * 2] * r;
		*v++ = y + h - r + cverts[i * 2 + 1] * r;
	}
	for (unsigned i = n; i <= n * 2; ++i) {
		*v++ = x + r + cverts[i * 2] * r;
		*v++ = y + h - r + cverts[i * 2 + 1] * r;
	}
	for (unsigned i = n * 2; i <= n * 3; ++i) {
		*v++ = x + r + cverts[i * 2] * r;
		*v++ = y + r + cverts[i * 2 + 1] * r;
	}
	for (unsigned i = n * 3; i < n * 4; ++i) {
		*v++ = x + w - r + cverts[i * 2] * r;
		*v++ = y + r + cverts[i * 2 + 1] * r;
	}
	*v++ = x + w - r + cverts[0] * r;
	*v++ = y + r + cverts[1] * r;
	render_mesh(render, verts, x, y, w, h, (n + 1) * 4, fth, col, depth);
}

void draw_line(IRenderer* render, float x0, float y0, float x1, float y1, unsigned int col, float depth) {
	
	render_vertex_3d_t* v = new_coords;
	
	// triangles
	set(v++, x0, y0, 0, 0, col, depth);
	set(v++, x1, y1, 0, 0, col, depth);
	set(v++, x1, y1, 0, 0, col, depth);
	set(v++, x0, y0, 0, 0, col, depth);
	unsigned int k = (int)(v - new_coords);
	render->render_mesh(new_coords, k, true);
}
void draw_line(IRenderer* render, float x0, float y0, float x1, float y1, float r, float fth,
			   unsigned int col, float depth) {
	float dx = x1 - x0;
	float dy = y1 - y0;
	float d = sqrtf(dx * dx + dy * dy);
	if (d > 0.0001f) {
		d = 1.0f / d;
		dx *= d;
		dy *= d;
	}
	float t = dx;
	dx = dy;
	dy = -t;
	float verts[4 * 2];
	r -= fth;
	r *= 0.5f;
	if (r < 0.01f)
		r = 0.01f;
	dx *= r;
	dy *= r;
	verts[0] = x0 - dx;
	verts[1] = y0 - dy;
	verts[2] = x0 + dx;
	verts[3] = y0 + dy;
	verts[4] = x1 + dx;
	verts[5] = y1 + dy;
	verts[6] = x1 - dx;
	verts[7] = y1 - dy;
	render_mesh(render, verts, x0, y0, 1.0f, 1.0f, 4, fth, col, depth);
}

// text rendering

// stbtt_bakedchar g_cdata[96]; // ASCII 32..126 is 95 glyphs
// GLuint g_ftex = 0;

// inline unsigned int RGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
// 	return (r) | (g << 8) | (b << 16) | (a << 24);
// }

bool Ui::load_font(const char* path, float font_height, font_t& font) {
	// Load font.
	size_t file_size;
	void* file_buffer = _platform->load_file(path, file_size);
	if (!file_buffer) {
		printf("failed to load file %s\n", path);
		assert(false && "failed to load file");
		return false;
	}
	void* bmap = malloc(512 * 512);
	if (!bmap)
		return false;

	stbtt_BakeFontBitmap((unsigned char*)file_buffer, 0, font_height, (unsigned char*)bmap, 512,
						 512, 32, 96,
						 font.cdata); // no guarantee this fits!

	// create texture
	unsigned int texture = _renderer->create_texture(512, 512, 1, bmap);
	if (texture == 0xffffffff)
		return false;

	free(bmap);
	free(file_buffer);
	font.id = texture;
	font.height = (uint)font_height;
	return true;
}
void get_baked_quad(const stbtt_bakedchar* chardata, int pw, int ph, int char_index, float* xpos,
					float* ypos, stbtt_aligned_quad* q) {
	const stbtt_bakedchar* b = chardata + char_index;
	int round_x = STBTT_ifloor(*xpos + b->xoff);
	int round_y = STBTT_ifloor(*ypos - b->yoff);

	q->x0 = (float)round_x;
	q->y0 = (float)round_y;
	q->x1 = (float)round_x + b->x1 - b->x0;
	q->y1 = (float)round_y - b->y1 + b->y0;

	q->s0 = b->x0 / (float)pw;
	q->t0 = b->y0 / (float)pw;
	q->s1 = b->x1 / (float)ph;
	q->t1 = b->y1 / (float)ph;

	*xpos += b->xadvance;
}
const float g_tabStops[4] = {150, 210, 270, 330};
float get_text_length(const stbtt_bakedchar* chardata, const char* text) {
	float xpos = 0;
	float len = 0;
	while (*text) {
		int c = (unsigned char)*text;
		if (c == '\t') {
			for (int i = 0; i < 4; ++i) {
				if (xpos < g_tabStops[i]) {
					xpos = g_tabStops[i];
					break;
				}
			}
		}
		else if (c >= 32 && c < 128) {
			const stbtt_bakedchar* b = chardata + c - 32;
			int round_x = STBTT_ifloor((xpos + b->xoff) + 0.5);
			len = (float)round_x + b->x1 - b->x0;
			xpos += b->xadvance;
		}
		++text;
	}
	return len;
}
void Ui::render_text(const font_t& font, float x, float y, float w, float h, const char* text,
					 int align, unsigned int col, float depth) {
	if (!text)
		return;

	if (align == ALIGN_CENTER)
		x = x + w / 2 - get_text_length(font.cdata, text) / 2;
	else if (align == ALIGN_RIGHT)
		x = x + w - get_text_length(font.cdata, text) - font.height / 2;
	else
		x += font.height / 2;

	if (align == ALIGN_BOTTOM)
		y += font.height / 4;
	else if (align == ALIGN_TOP)
		y = y + h - font.height / 4;
	else
		y = y + h / 2 - font.height / 4;

	// y += h / 4; // adjust text position

	// glColor4ub(col&0xff, (col>>8)&0xff, (col>>16)&0xff, (col>>24)&0xff);
	render_vertex_3d_t* v = new_coords;
	const float ox = x;
	int k = 0;
	int c = 0;
	while (*text) {
		int c = (unsigned char)*text;
		if (c == '\t') {
			for (int i = 0; i < 4; ++i) {
				if (x < g_tabStops[i] + ox) {
					x = g_tabStops[i] + ox;
					break;
				}
			}
		}
		else if (c >= 32 && c < 128) {
			stbtt_aligned_quad q;
			get_baked_quad(font.cdata, 512, 512, c - 32, &x, &y, &q);
			// triangles
			set(v++, q.x0, q.y0, q.s0, q.t0, col, depth);
			set(v++, q.x1, q.y1, q.s1, q.t1, col, depth);
			set(v++, q.x1, q.y0, q.s1, q.t0, col, depth);
			set(v++, q.x0, q.y0, q.s0, q.t0, col, depth);
			set(v++, q.x0, q.y1, q.s0, q.t1, col, depth);
			set(v++, q.x1, q.y1, q.s1, q.t1, col, depth);
		}
		// else   if (c > 128)
		//	assert(false&&"No yet support for non-english");
		++text;
	}
	k = (int)(v - new_coords);
	_renderer->render_mesh(new_coords, k, true);
}
}
