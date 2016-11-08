// glw_imgui
// Copyright (C) 2016 Iakov Sumygin - BSD license

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include "ui.h"

namespace imgui {

struct render_vertex_3d_t {
	float x, y, z, u, v;
	unsigned int clr;
};

class IPlatform {
public:
	virtual void set_cursor(CURSOR cursor) = 0;
	virtual void capture_mouse(bool set) = 0;
};

class IRenderer {
public:
	virtual bool create() = 0;
	virtual bool begin() = 0;
	virtual bool end() = 0;
	virtual unsigned int create_texture(unsigned int width, unsigned int height, void* bmp,
										bool font) = 0;
	virtual bool bind_texture(unsigned int texture) = 0;
	virtual bool render_mesh(const render_vertex_3d_t* tries, int count, bool b) = 0;
	virtual void set_blend_mode(BlendMode mode) = 0;
	virtual void set_scissor(int x, int y, int w, int h, bool set) = 0;
};
}
#endif //_PLATFORM_H_
