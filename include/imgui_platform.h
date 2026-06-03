// glw_imgui
// Copyright (C) 2016 Iakov Sumygin - BSD license

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include "imgui_ui.h"

namespace imgui {

struct render_vertex_3d_t {
	float x, y, z, u, v;
	unsigned int clr;
};

class IPlatform {
public:
	virtual ~IPlatform(){}
	virtual void set_cursor(CURSOR cursor) = 0;
	virtual void capture_mouse(bool set) = 0;
	virtual void* load_file(const char* path, size_t& buf_size) = 0;
	virtual void play_sound(const char* path) = 0;
};

class IRenderer {
public:
	~IRenderer(){}
	virtual bool create() = 0;
	virtual bool begin(uint width, uint height) = 0;
	virtual bool end() = 0;
	virtual unsigned char* load_image(const char* mem, unsigned int mem_size, int* width, int* height,
									  int* channels) = 0;
	virtual unsigned int create_texture(unsigned int width, unsigned int height,
										unsigned int channels, void* bmp) = 0;
	virtual unsigned int load_texture(const char* texture_name) = 0;
	virtual bool copy_sub_texture(unsigned int target, unsigned int x, unsigned int y,
								  unsigned int width, unsigned int height, void* bmp) = 0;
	virtual bool remove_texture(unsigned int texture) = 0;
	virtual bool bind_texture(unsigned int texture, bool wrap) = 0;
	virtual bool render_mesh(const render_vertex_3d_t* tries, int count, bool b) = 0;
	virtual void set_blend_mode(BlendMode mode) = 0;
	virtual void set_scissor(int x, int y, int w, int h, bool set) = 0;
	virtual void draw_rect(
		float transform[16],
		float x, float y, float w, float h, float texture_x, float texture_y,
		float texture_width, float texture_height, unsigned int col, float depth) = 0;
};
}
#endif //_PLATFORM_H_
