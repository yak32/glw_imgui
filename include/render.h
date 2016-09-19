// glw_imgui
// Copyright (C) 2016 Iakov Sumygin - BSD license

#ifndef _IMGUI_RENDER_H_
#define _IMGUI_RENDER_H_

namespace imgui {
class IRenderer;
bool render_init();
void render_destroy();
void render_draw(IRenderer* render, bool transparency = true);
}

#endif // _IMGUI_RENDER_H_
