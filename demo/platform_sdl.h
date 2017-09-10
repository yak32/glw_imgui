// glw_imgui
// Copyright (C) 2016 Iakov Sumygin - BSD license

#ifndef _RENDERER_SDL_H_
#define _RENDERER_SDL_H_

#include "imgui_platform.h"
#include <SDL_mouse.h>

namespace imgui {

void getDisplayScaleFactor(float& x, float& y);

class PlatformSDL : public IPlatform {
public:
	PlatformSDL();
	~PlatformSDL();
	void set_cursor(CURSOR cursor);
	void capture_mouse(bool set);
	void* load_file(const char* path, size_t& buf_size);

private:
	SDL_Cursor* _cursors[CURSOR_COUNT];
};

class RenderSDL : public IRenderer {
public:
	bool create();
	bool begin(uint width, uint height);
	bool end();

	unsigned char* load_image(const char* filename, int* width, int* height, int* channels);
	unsigned int create_texture(unsigned int width, unsigned int height, unsigned int channels,
								void* bmp);
	bool remove_texture(unsigned int texture);
	bool bind_texture(unsigned int texture);
	bool copy_sub_texture(unsigned int target, unsigned int x, unsigned int y, unsigned int width,
						  unsigned int height, void* bmp);

	bool render_mesh(const render_vertex_3d_t* tries, int count, bool b);
	void set_blend_mode(BlendMode mode);
	void set_scissor(int x, int y, int w, int h, bool set);

protected:
	void initialize_render(uint width, uint height);
	void render(int start, int count);

protected:
	std::vector<render_vertex_3d_t> _mesh;
};
}
#endif //_RENDERER_SDL_H_
