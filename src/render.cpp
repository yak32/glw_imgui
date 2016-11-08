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
#include "stb_truetype.h"

#include "ui.h"
#include "platform.h"

static stbtt_bakedchar g_cdata[96]; // ASCII 32..126 is 95 glyphs

namespace imgui {

static const unsigned TEMP_COORD_COUNT = 1000;
static float g_tempCoords[TEMP_COORD_COUNT * 2];
static float g_tempNormals[TEMP_COORD_COUNT * 2];

static render_vertex_3d_t new_coords[TEMP_COORD_COUNT * 2 * 6];

static const int CIRCLE_VERTS = 8 * 4;
static float g_circleVerts[CIRCLE_VERTS * 2];

static unsigned int gWhiteTexture;

static unsigned int g_current_texture;
static unsigned int g_current_font;
static bool g_blend_texture;
static float g_depth =
	0; // ui depth (need to sorting windows properly, especially during moving rollouts)
static const float MAX_UI_DEPTH = 256.0f; // to match z buffer depth

#define RENDERER_ARGB(a, r, g, b)                                                                  \
	((unsigned int)((((a)&0xff) << 24) | (((r)&0xff) << 16) | (((g)&0xff) << 8) | ((b)&0xff)))
#define RENDERER_COLOR_RGBA(r, g, b, a) RENDERER_ARGB(a, r, g, b)
#define COLOR_TO_D3D(c)                                                                            \
	RENDERER_COLOR_RGBA((c & 0xff), ((c >> 8) & 0xff), ((c >> 16) & 0xff), ((c >> 24) & 0xff))

static void render_text(IRenderer* r, float x, float y, float w, float h, const char* text,
						int align, unsigned int col);
static unsigned int load_font(IRenderer* r, const char* path, float font_height);
inline void set(render_vertex_3d_t* v, float x, float y, unsigned int col) {
	v->x = x;
	v->y = y;
	v->z = g_depth;
	v->clr = col;
}
inline void set(render_vertex_3d_t* v, float x, float y, float u, float vv, unsigned int col) {
	v->x = x;
	v->y = y;
	v->z = g_depth;
	v->u = u;
	v->v = vv;
	v->clr = col;
}

static void draw_rect(IRenderer* r, float x, float y, float w, float h, float fth,
					  unsigned int col);
static void draw_rounded_rect(IRenderer* render, float x, float y, float w, float h, float r,
							  float fth, unsigned int col);
static void render_mesh(IRenderer* renderer, const float* coords, float txt_shift_x,
						float txt_shift_y, float txt_scale_x, float txt_scale_y, unsigned numCoords,
						float r, unsigned int col);
bool Ui::render_init(IRenderer* r) {
	for (unsigned i = 0; i < CIRCLE_VERTS; ++i) {
		float a = (float)i / (float)CIRCLE_VERTS * (float)M_PI * 2;
		g_circleVerts[i * 2 + 0] = cosf(a);
		g_circleVerts[i * 2 + 1] = sinf(a);
	}

	unsigned int white = 0xffffffff;

	// create texture
	gWhiteTexture = r->create_texture(1, 1, &white, false);
	return gWhiteTexture != 0;
}
void Ui::render_destroy(IRenderer* r) {
	// g_current_texture.reset();
	// g_current_font.reset();
}
void Ui::render_draw(IRenderer* r, Ui* ui, bool transparency) {
	r->begin();

	int size;
	const gfx_cmd* cmd = ui->get_render_queue(size);

	unsigned int texture = gWhiteTexture;
	r->bind_texture(texture);

	bool bind_font = true;
	for (int i = 0; i < size; ++i, ++cmd) {
		switch (cmd->type) {
		case GFX_CMD_RECT:
			r->bind_texture(gWhiteTexture);
			r->set_blend_mode(BLEND_RECT);
			// if (!g_blend_texture)
			//	r->set_state(RS_BLEND, RF_FALSE);

			if (cmd->rect.r == 0) {
				draw_rect(r, (float)cmd->rect.x + 0.5f, (float)cmd->rect.y + 0.5f,
						  (float)cmd->rect.w - 1, (float)cmd->rect.h - 1, 1.0f, cmd->col);
			}
			else {
				draw_rounded_rect(r, (float)cmd->rect.x + 0.5f, (float)cmd->rect.y + 0.5f,
								  (float)cmd->rect.w - 1, (float)cmd->rect.h - 1,
								  (float)cmd->rect.r, 1.0f, cmd->col);
			}
			break;

		case GFX_CMD_TRIANGLE:
			r->set_blend_mode(BLEND_RECT);
			r->bind_texture(gWhiteTexture);
			if (cmd->flags == 1) {
				const float verts[3 * 2] = {
					(float)cmd->rect.x + 0.5f,
					(float)cmd->rect.y + 0.5f,
					(float)cmd->rect.x + 0.5f + (float)cmd->rect.w - 1,
					(float)cmd->rect.y + 0.5f + (float)cmd->rect.h / 2 - 0.5f,
					(float)cmd->rect.x + 0.5f,
					(float)cmd->rect.y + 0.5f + (float)cmd->rect.h - 1,
				};
				render_mesh(r, verts, 0, 0, 1.0f, 1.0f, 3, 1.0f, cmd->col);
			}
			if (cmd->flags == 2) {
				const float verts[3 * 2] = {
					(float)cmd->rect.x + 0.5f,
					(float)cmd->rect.y + (float)cmd->rect.h - 1,
					(float)cmd->rect.x + 0.5f + (float)cmd->rect.w / 2 - 0.5f,
					(float)cmd->rect.y + 0.5f,
					(float)cmd->rect.x + 0.5f + (float)cmd->rect.w - 1,
					(float)cmd->rect.y + 0.5f + (float)cmd->rect.h - 1,
				};
				render_mesh(r, verts, 0, 0, 1.0f, 1.0f, 3, 1.0f, cmd->col);
			}
			break;

		case GFX_CMD_DEPTH:
// after receiving depth command, setup depth testing (disabled by default)
#ifdef IMGUI_RIGHTHANDED
			g_depth = 1.0f - (float)cmd->depth.z / MAX_UI_DEPTH;
#else
			g_depth = (float)(MAX_UI_LAYER_COUNT - cmd->depth.z) / MAX_UI_DEPTH;
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
			texture = g_current_font;
			r->bind_texture(texture);
			r->set_blend_mode(BLEND_TEXT);
			render_text(r, cmd->text.x, cmd->text.y, cmd->text.width, cmd->text.height,
						cmd->text.text, cmd->text.align, cmd->col);
			break;

			// case GFX_CMD_TEXTURE:
			//	if (cmd->texture.path)
			//		g_current_texture = r->create_texture(
			//			cmd->texture.path,
			//			TLO_BACKUP |
			//				RESOURCE_HOLD); // use RESOURCE_HOLD to save reference in resource
			// manager
			break;

		case GFX_CMD_FONT:
			if (cmd->font.path)
				g_current_font = load_font(r, cmd->font.path, cmd->font.height);
			break;
		}
	}
	r->end();
	m_rqueue.on_render_finished();
}

// simple drawing, without any alpha blending
static void render_quads(IRenderer* r, const float* coords, float txt_shift_x, float txt_shift_y,
						 float txt_scale_x, float txt_scale_y, unsigned numCoords,
						 unsigned int col) {
	if (numCoords > TEMP_COORD_COUNT)
		numCoords = TEMP_COORD_COUNT;

	render_vertex_3d_t* v = &new_coords[0];
	for (unsigned i = 2; i < numCoords; ++i) {
		set(v++, coords[0], coords[1], col);
		set(v++, coords[(i - 1) * 2], coords[(i - 1) * 2 + 1], col);
		set(v++, coords[i * 2], coords[i * 2 + 1], col);
	}
	int k = v - &new_coords[0];
	for (int i = 0; i < k; ++i) {
		new_coords[i].u = (new_coords[i].u - txt_shift_x) / txt_scale_x;
		new_coords[i].v = 1.0f - (new_coords[i].v - txt_shift_y) / txt_scale_y;
	}
	r->render_mesh(new_coords, k, true);
}
static void render_mesh(IRenderer* renderer, const float* coords, float txt_shift_x,
						float txt_shift_y, float txt_scale_x, float txt_scale_y, unsigned numCoords,
						float r, unsigned int col) {
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
		set(v++, coords[i * 2], coords[i * 2 + 1], col);
		set(v++, coords[j * 2], coords[j * 2 + 1], col);
		set(v++, g_tempCoords[j * 2], g_tempCoords[j * 2 + 1], colTrans);

		set(v++, g_tempCoords[j * 2], g_tempCoords[j * 2 + 1], colTrans);
		set(v++, g_tempCoords[i * 2], g_tempCoords[i * 2 + 1], colTrans);
		set(v++, coords[i * 2], coords[i * 2 + 1], col);
	}
	for (unsigned i = 2; i < numCoords; ++i) {
		set(v++, coords[0], coords[1], col);
		set(v++, coords[(i - 1) * 2], coords[(i - 1) * 2 + 1], col);
		set(v++, coords[i * 2], coords[i * 2 + 1], col);
	}
	int k = v - &new_coords[0];
	for (int i = 0; i < k; ++i) {
		new_coords[k].u = (new_coords[k].x - txt_shift_x) / txt_scale_x;
		new_coords[k].v = 1.0f - (new_coords[k].y - txt_shift_y) / txt_scale_y;
		new_coords[k].clr = col;
	}
	renderer->render_mesh(new_coords, k, true);
}

static void draw_rect(IRenderer* r, float x, float y, float w, float h, float fth,
					  unsigned int col) {
	float verts[4 * 2] = {
		x, y, x + w, y, x + w, y + h, x, y + h,
	};
	render_quads(r, verts, x, y, w, h, 4, col);
}
static void draw_ellipse(IRenderer* r, float x, float y, float w, float h, float fth,
						 unsigned int col) {
	float verts[CIRCLE_VERTS * 2];
	const float* cverts = g_circleVerts;
	float* v = verts;
	for (unsigned i = 0; i < CIRCLE_VERTS; ++i) {
		*v++ = x + cverts[i * 2] * w;
		*v++ = y + cverts[i * 2 + 1] * h;
	}
	render_mesh(r, verts, x, y, w, h, CIRCLE_VERTS, fth, col);
}
static void draw_rounded_rect(IRenderer* render, float x, float y, float w, float h, float r,
							  float fth, unsigned int col) {
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
	render_mesh(render, verts, x, y, w, h, (n + 1) * 4, fth, col);
}
static void draw_line(IRenderer* render, float x0, float y0, float x1, float y1, float r, float fth,
					  unsigned int col) {
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
	render_mesh(render, verts, x0, y0, 1.0f, 1.0f, 4, fth, col);
}

// text rendering

// static stbtt_bakedchar g_cdata[96]; // ASCII 32..126 is 95 glyphs
// static GLuint g_ftex = 0;

// inline unsigned int RGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
// 	return (r) | (g << 8) | (b << 16) | (a << 24);
// }

unsigned char ttf_buffer[1 << 20];

static unsigned int load_font(IRenderer* r, const char* path, float font_height) {
	// Load font.
	void* bmap = malloc(512 * 512);
	if (!bmap)
		return 0xffffffff;

	fread(ttf_buffer, 1, 1 << 20, fopen(path, "rb"));
	stbtt_BakeFontBitmap(ttf_buffer, 0, font_height, (unsigned char*)bmap, 512, 512, 32, 96,
						 g_cdata); // no guarantee this fits!

	// create texture
	unsigned int texture = r->create_texture(512, 512, bmap, true);
	free(bmap);
	return texture;
}
static void getBaked_quad(stbtt_bakedchar* chardata, int pw, int ph, int char_index, float* xpos,
						  float* ypos, stbtt_aligned_quad* q) {
	stbtt_bakedchar* b = chardata + char_index;
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
static const float g_tabStops[4] = {150, 210, 270, 330};
static float getText_length(stbtt_bakedchar* chardata, const char* text) {
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
			stbtt_bakedchar* b = chardata + c - 32;
			int round_x = STBTT_ifloor((xpos + b->xoff) + 0.5);
			len = round_x + b->x1 - b->x0 + 0.5f;
			xpos += b->xadvance;
		}
		++text;
	}
	return len;
}
static void render_text(IRenderer* r, float x, float y, float w, float h, const char* text,
						int align, unsigned int col) {
	if (!text)
		return;
	if (align == ALIGN_CENTER)
		x = x + w / 2 - getText_length(g_cdata, text) / 2;
	else if (align == ALIGN_RIGHT)
		x = x + w - getText_length(g_cdata, text) - h / 2;
	else
		x += h / 2;

	y += h / 4; // adjust text position

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
			getBaked_quad(g_cdata, 512, 512, c - 32, &x, &y, &q);
			// triangles
			set(v++, q.x0, q.y0, q.s0, q.t0, col);
			set(v++, q.x1, q.y1, q.s1, q.t1, col);
			set(v++, q.x1, q.y0, q.s1, q.t0, col);
			set(v++, q.x0, q.y0, q.s0, q.t0, col);
			set(v++, q.x0, q.y1, q.s0, q.t1, col);
			set(v++, q.x1, q.y1, q.s1, q.t1, col);
		}
		// else   if (c > 128)
		//	assert(false&&"No yet support for non-english");
		++text;
	}
	k = v - new_coords;
	r->render_mesh(new_coords, k, true);
}
}
