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

#include "imgui_render_queue.h"
#include <string.h>

namespace imgui {

void set_gfx_rect(gfx_rect& r, int x, int y, int w, int h, int _r = 0) {
	r.x = (short)x;
	r.y = (short)y;
	r.w = (short)w;
	r.h = (short)h;
	r.r = (short)_r;
}
unsigned int apply_alpha_state(unsigned int color, unsigned int alpha) {
	int a = ((color >> 24) * alpha) >> 8;
	color &= 0x00ffffff;
	return color | a << 24;
}
unsigned char get_alpha(unsigned int color) {
	return (unsigned char)(color >> 24);
}

RenderQueue::RenderQueue() : _size(0), _mem_size(0), _ready_to_render(false){
	_render_options = 0;
	_alpha = 255;
	// _render_options(RENDER_OPTIONS_NONROUNDED_RECT)
}
void RenderQueue::set_render_options(int options){
	_render_options = options;
}
// to support multithreaded rendering, 2 render queue is added
// one is used for rendering, another for logic. buffers are swapped
// after rendering is finished
bool RenderQueue::ready_to_render() const {
	return _ready_to_render;
}
void RenderQueue::set_alpha(unsigned int alpha) {
	_alpha = alpha;
}
void RenderQueue::on_frame_finished() {
	if (_size != GFXCMD_QUEUE_SIZE) {
		_mem_size = _size;
		_size = GFXCMD_QUEUE_SIZE; // prevent queue update	during next frame
		_ready_to_render = true;
	}
}
void RenderQueue::prepare_render() {
	_size = _mem_size;
	_ready_to_render = false;
}
const char* RenderQueue::alloc_text(const char* text) {
	unsigned len = (unsigned)strlen(text) + 1;
	if (_text_pool_size + len >= TEXT_POOL_SIZE)
		return 0;
	char* dst = &_text_pool[_text_pool_size];
	memcpy(dst, text, len);
	_text_pool_size += len;
	return dst;
}
void RenderQueue::reset_gfx_cmd_queue() {
	_mem_size = _size = 0;
	_text_pool_size = 0;
	_ready_to_render = false;
}
void RenderQueue::add_scissor(int x, int y, int w, int h) {
	if (_size >= GFXCMD_QUEUE_SIZE)
		return;
	gfx_cmd& cmd = _queue[_size++];
	cmd.type = GFX_CMD_SCISSOR;
	cmd.flags = x < 0 ? 0 : 1; // on/off flag.
	cmd.col = 0;
	set_gfx_rect(cmd.rect, x, y, w, h);
}
void RenderQueue::add_rect(int x, int y, int w, int h, unsigned int color) {
	if (_size >= GFXCMD_QUEUE_SIZE)
		return;

	gfx_cmd& cmd = _queue[_size++];
	cmd.type = GFX_CMD_RECT;
	cmd.flags = 0;
	cmd.col = apply_alpha_state(color, _alpha);
	set_gfx_rect(cmd.rect, x, y, w, h);
}
void RenderQueue::add_rounded_rect(int x, int y, int w, int h, int r, unsigned int color) {
	if (_size >= GFXCMD_QUEUE_SIZE)
		return;

	gfx_cmd& cmd = _queue[_size++];
	cmd.type = GFX_CMD_RECT;
	cmd.flags = 0;
	cmd.col = apply_alpha_state(color, _alpha);

	if (_render_options & RENDER_OPTIONS_NONROUNDED_RECT)
		set_gfx_rect(cmd.rect, x, y, w, h);
	else
		set_gfx_rect(cmd.rect, x, y, w, h, r);
}
void RenderQueue::add_triangle(int x, int y, int w, int h, int flags, unsigned int color) {
	if (_size >= GFXCMD_QUEUE_SIZE)
		return;

	gfx_cmd& cmd = _queue[_size++];
	cmd.type = GFX_CMD_TRIANGLE;
	cmd.flags = (char)flags;
	cmd.col = apply_alpha_state(color, _alpha);
	set_gfx_rect(cmd.rect, x, y, w, h);
}
void RenderQueue::add_depth(int depth) {
	if (_size >= GFXCMD_QUEUE_SIZE)
		return;

	gfx_cmd& cmd = _queue[_size++];
	cmd.type = GFX_CMD_DEPTH;
	cmd.flags = 0;
	cmd.depth.z = (short)depth;
}
void RenderQueue::add_text(int x, int y, int width, int height, int align, const char* text,
						   unsigned int color) {
	if (_size >= GFXCMD_QUEUE_SIZE)
		return;
	gfx_cmd& cmd = _queue[_size++];
	cmd.type = GFX_CMD_TEXT;
	cmd.flags = 0;
	cmd.col = apply_alpha_state(color, _alpha);
	cmd.text.x = (short)x;
	cmd.text.y = (short)y;
	cmd.text.width = (short)width;
	cmd.text.height = (short)height;
	cmd.text.align = (short)align;
	cmd.text.text = alloc_text(text);
}
void RenderQueue::add_texture(const char* path, const frect& rc, bool blend) {
	if (_size >= GFXCMD_QUEUE_SIZE)
		return;

	gfx_cmd& cmd = _queue[_size++];
	cmd.type = GFX_CMD_TEXTURE;
	cmd.flags = blend ? 1 : 0;
	cmd.texture.path = path ? alloc_text(path) : 0;
	cmd.texture.rc.left = rc.left;
	cmd.texture.rc.right = rc.right;
	cmd.texture.rc.bottom = rc.bottom;
	cmd.texture.rc.top = rc.top;
}
void RenderQueue::add_font(const char* path, float height) {
	if (_size >= GFXCMD_QUEUE_SIZE)
		return;

	gfx_cmd& cmd = _queue[_size++];
	cmd.type = GFX_CMD_FONT;
	cmd.flags = path ? 1 : 0;
	cmd.font.path = path ? alloc_text(path) : 0;
	cmd.font.height = height;
}
const gfx_cmd* RenderQueue::get_queue() const {
	return _queue;
}
size_t RenderQueue::get_size() const {
	return _size;
}
}
