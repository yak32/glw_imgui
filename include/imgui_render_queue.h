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
// Modified by Iakov Sumygin

#ifndef _RENDER_QUEUE_H_
#define _RENDER_QUEUE_H_

#include <stddef.h>
#include <atomic>
namespace imgui {

static const unsigned GFXCMD_QUEUE_SIZE = 1024;
static const unsigned TEXT_POOL_SIZE = 8000;

enum gfx_cmdType {
	GFX_CMD_UNDEFINED = 0,
	GFX_CMD_RECT,
	GFX_CMD_LINE,
	GFX_CMD_TRIANGLE,
	GFX_CMD_TEXT,
	GFX_CMD_DEPTH,
	GFX_CMD_SCISSOR,
	GFX_CMD_TEXTURE,
	GFX_CMD_FONT
};
enum RENDER_OPTIONS { RENDER_OPTIONS_NONROUNDED_RECT = 1 };
struct gfx_rect {
	short x, y, w, h, r;
};
struct gfx_line {
	short x1, y1, x2, y2;
};
struct gfx_text {
	short x, y, width, height, align;
	const char* text;
};
struct gfx_depth {
	short z;
};
struct gfx_font {
	float height;
	const char* path;
};
struct rect {
	rect(float _left, float _top, float _right, float _bottom)
		: left(_left), top(_top), right(_right), bottom(_bottom) {}
	float left, top, right, bottom;
};
struct frect {
	float left, top, right, bottom;
};
struct gfx_texture {
	frect rc;
	const char* path;
};
struct gfx_cmd {
	char type;
	char flags;
	char pad[2];
	unsigned int col;
	union {
		gfx_rect rect;
		gfx_line line;
		gfx_text text;
		gfx_depth depth;
		gfx_font font;
		gfx_texture texture;
	};
};

struct RenderQueue {
	RenderQueue();

	void on_frame_finished();
	void prepare_render();
	const char* alloc_text(const char* text);
	void reset_gfx_cmd_queue();
	void add_scissor(int x, int y, int w, int h);
	void add_rect(int x, int y, int w, int h, unsigned int color);
	void add_rounded_rect(int x, int y, int w, int h, int r, unsigned int color);
	void add_triangle(int x, int y, int w, int h, int flags, unsigned int color);
	void add_line(int x1, int y1, int x2, int y2, unsigned int color);
	void add_depth(int depth);
	void add_text(int x, int y, int width, int height, int align, const char* text,
				  unsigned int color);
	void add_texture(const char* path, const frect& rc, bool blend);
	void add_font(const char* path, float height);

	const gfx_cmd* get_queue() const;
	unsigned int get_size() const;

	bool ready_to_render() const;
	void set_alpha(unsigned int alpha);
	unsigned int get_alpha() const;
	void set_render_options(unsigned int options);

private:
	gfx_cmd _queue[GFXCMD_QUEUE_SIZE];
	unsigned int _size, _mem_size;

	char _text_pool[TEXT_POOL_SIZE];
	unsigned _text_pool_size;

	unsigned int _render_options;
	unsigned int _alpha;

	std::atomic<bool> _ready_to_render;
};
}
#endif //_RENDER_QUEUE_H_
