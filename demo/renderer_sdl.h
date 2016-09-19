// glw_imgui
// Copyright (C) 2016 Iakov Sumygin - BSD license

#ifndef _RENDERER_SDL_H_
#define _RENDERER_SDL_H_

#include "irenderer.h"

namespace imgui{
class RenderSDL: public IRenderer{
public:
	bool create();
	bool begin();
	bool end();
	unsigned int create_texture(unsigned int width, unsigned int height, void* bmp, bool font);
	bool bind_texture(unsigned int texture);
	bool render_mesh(const render_vertex_3d_t* tries, int count, bool b);
	void set_blend_mode(BlendMode mode);
};
}
#endif //_RENDERER_SDL_H_
