// glw_imgui
// Copyright (C) 2016 Iakov Sumygin - BSD license

#pragma once

#include <vector>

namespace imgui {
struct Rollout;
struct toolbar_t {
	toolbar_t(Rollout* r = nullptr)
		: rollout(r), left(nullptr), right(nullptr), div(0), horz(false) {}
	~toolbar_t() {}
	toolbar_t* left;
	toolbar_t* right;
	float	  div;
	bool	   horz;
	int		   w, h; // temporary information
	Rollout*   rollout;
	toolbar_t* add_rollout(Rollout* r, float _div, bool _horz);
};
struct div_drag {
	div_drag() {}
	div_drag(float* _div, int _size) : div(_div), size(_size) {}
	float* div;
	int	size;
	void shift(int shift);
};
void visit_rollout_node(std::vector<Rollout*>& rollouts, toolbar_t* n, int x, int y, int w, int h);
div_drag find_div(int mx, int my, toolbar_t* n, int x, int y, int w, int h);
toolbar_t* search_rollout_node(toolbar_t* n, const Rollout* search, bool check_tabs = false);
toolbar_t* search_rollout_parent_node(toolbar_t* n, const Rollout* search);
void clear_toolbars(toolbar_t* n);
}
