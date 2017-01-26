// glw_imgui
// Copyright (C) 2016 Iakov Sumygin - BSD license

#pragma once
#include <cstddef>
#include <vector>

namespace imgui {
struct Rollout;
struct Toolbar {
	Toolbar(Rollout* r = NULL)
		: left(NULL), right(NULL), div(0), horz(false), w(0), h(0), rollout(r) {}
	~Toolbar() {}
	Toolbar* left;
	Toolbar* right;
	float div;
	bool horz;
	int w, h; // temporary information
	Rollout* rollout;
	Toolbar* add_rollout(Rollout* r, float _div, bool _horz);
};
struct div_drag {
	div_drag() {}
	div_drag(float* _div, int _size) : div(_div), size(_size) {}
	float* div;
	int size;
	void shift(int shift);
};
void visit_rollout_node(std::vector<Rollout*>& rollouts, Toolbar* n, int x, int y, int w, int h);
div_drag find_div(int mx, int my, Toolbar* n, int x, int y, int w, int h, int padding);
Toolbar* search_rollout_node(Toolbar* n, const Rollout* search, bool check_tabs = false);
Toolbar* search_rollout_parent_node(Toolbar* n, const Rollout* search);
void clear_toolbars(Toolbar* n);
}
