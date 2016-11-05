// glw_imgui
// Copyright (C) 2016 Iakov Sumygin - BSD license

#ifndef _RENDERER_SDL_H_
#define _RENDERER_SDL_H_

#include "platform.h"
#include <SDL_mouse.h>

namespace imgui{

class PlatformSDL: public IPlatform{
public:
	PlatformSDL();
	~PlatformSDL();
	void set_cursor(CURSOR cursor);
	void capture_mouse(bool set);
private:
	SDL_Cursor* m_cursors[CURSOR_COUNT];
};

class RenderSDL: public IRenderer{
public:
	bool create();
	bool begin();
	bool end();
	unsigned int create_texture(unsigned int width, unsigned int height, void* bmp, bool font);
	bool bind_texture(unsigned int texture);
	bool render_mesh(const render_vertex_3d_t* tries, int count, bool b);
	void set_blend_mode(BlendMode mode);
	void set_scissor(int x, int y, int w, int h, bool set);
};
}
#endif //_RENDERER_SDL_H_
