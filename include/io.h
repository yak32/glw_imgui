// glw_imgui
// Copyright (C) 2016 Iakov Sumygin - BSD license

#pragma once
namespace imgui {
class Ui;
bool save_layout(Ui& ui, const char* filename);
bool load_layout(Ui& ui, const char* filename);
}
