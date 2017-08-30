// glw_imgui
// Copyright (C) 2016 Iakov Sumygin - BSD license

#pragma once
namespace imgui {
class Ui;
struct Theme;
bool save_layout(Ui& ui, const char* filename);
bool load_layout(Ui& ui, const char* filename);
bool save_theme(const Theme& theme, const char* filename);
bool load_theme(Theme& theme, const char* filename);
}
