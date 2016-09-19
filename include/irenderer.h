// glw_imgui
// Copyright (C) 2016 Iakov Sumygin - BSD license

#ifndef _IRENDERER_H_
#define _IRENDERER_H_

#include "ui.h"

namespace imgui{

struct render_vertex_3d_t{
	float x,y,z,u,v;
	unsigned int clr;
};

class IRenderer{
public:
	virtual	bool create() = 0;
	virtual bool begin() = 0;
	virtual bool end() = 0;
	virtual unsigned int create_texture(unsigned int width, unsigned int height, void* bmp, bool font) = 0;
	virtual bool bind_texture(unsigned int texture) = 0;
	virtual bool render_mesh(const render_vertex_3d_t* tries, int count, bool b) = 0;
	virtual void set_blend_mode(BlendMode mode) = 0;
};
}
#endif //_IRENDERER_H_
