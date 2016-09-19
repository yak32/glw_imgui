// glw_imgui
// Copyright (C) 2016 Iakov Sumygin - BSD license

#include "toolbars.h"
#include "ui.h"
#include "rollout.h"
#include <math.h>
#include <cstdlib>

namespace imgui {
toolbar_t* toolbar_t::add_rollout(Rollout* r, float _div, bool _horz) {
	div = _div;
	horz = _horz;
	if (div == 1.0f) {
		rollout = r;
		return this;
	}
	if (div > 0) {
		left = new toolbar_t(r);
		right = new toolbar_t(nullptr);
		return right;
	}
	right = new toolbar_t(r);
	left = new toolbar_t(nullptr);
	return left;
}
void visit_rollout_node(std::vector<Rollout*>& rollouts, toolbar_t* n, int x, int y, int w, int h) {
	if (!n)
		return;
	if (n->rollout) {
		// leaf node
		n->rollout->x = x;
		n->rollout->y = y;
		n->rollout->w = w;
		n->rollout->h = h;
		for (int r_id : n->rollout->tabs) {
			Rollout* r = rollouts[r_id];
			r->x = x;
			r->y = y;
			r->w = w;
			r->h = h;
		}
	}
	n->w = w;
	n->h = h;
	int div = 0;
	int dist = n->horz ? w : h;
	if (n->div > 0)
		div = n->div < 1.0f ? (int)(dist * n->div) : (int)(n->div);
	else
		div = n->div > -1.0f ? dist + (int)(dist * n->div) : dist + (int)(n->div);
	int sep = 1;
	if (n->horz) {
		visit_rollout_node(rollouts, n->left, x, y, div, h);
		visit_rollout_node(rollouts, n->right, x + div + sep, y, w - div - sep, h);
	}
	else {
		visit_rollout_node(rollouts, n->left, x, y, w, div);
		visit_rollout_node(rollouts, n->right, x, y + div + sep, w, h - div - sep);
	}
}
div_drag find_div(int mx, int my, toolbar_t* n, int x, int y, int w, int h) {
	div_drag res;
	res.div = nullptr;
	if (!n || mx < x || mx > x + w || my < y || my > y + h)
		return res;

	int div = 0;
	int dist = n->horz ? w : h;
	if (n->div > 0)
		div = n->div < 1.0f ? (int)(dist * n->div) : (int)(n->div);
	else
		div = n->div > -1.0f ? dist + (int)(dist * n->div) : dist + (int)(n->div);

	int sep = 1;
	if (n->horz) {
		if ((!n->left || !n->left->rollout || n->left->rollout->options & RESIZE_AREA) &&
			(!n->right || !n->right->rollout || n->right->rollout->options & RESIZE_AREA) &&
			std::abs(mx - (x + div)) < DEFAULT_PADDING * 2) {
			return div_drag(&n->div, n->w);
		}
		res = find_div(mx, my, n->left, x, y, div, h);
		if (res.div)
			return res;
		res = find_div(mx, my, n->right, x + div + sep, y, w - div - sep, h);
		if (res.div)
			return res;
	}
	else {
		if ((!n->left || !n->left->rollout || n->left->rollout->options & RESIZE_AREA) &&
			(!n->right || !n->right->rollout || n->right->rollout->options & RESIZE_AREA) &&
			std::abs(my - (y + div)) < DEFAULT_PADDING * 2) {
			return div_drag(&n->div, n->h);
		}
		res = find_div(mx, my, n->left, x, y, w, div);
		if (res.div)
			return res;
		res = find_div(mx, my, n->right, x, y + div + sep, w, h - div - sep);
		if (res.div)
			return res;
	}
	return res;
}
toolbar_t* search_rollout_node(toolbar_t* n, const Rollout* search, bool check_tabs) {
	if (!n)
		return nullptr;
	if (n->rollout == search)
		return n;
	if (check_tabs && n->rollout)
		for (auto i : n->rollout->tabs)
			if (i == search->id)
				return n;
	toolbar_t* r = search_rollout_node(n->left, search, check_tabs);
	if (r)
		return r;
	r = search_rollout_node(n->right, search, check_tabs);
	if (r)
		return r;
	return nullptr;
}
toolbar_t* search_rollout_parent_node(toolbar_t* n, const Rollout* search) {
	if (!n)
		return nullptr;
	if (n->left && n->left->rollout == search)
		return n;
	if (n->right && n->right->rollout == search)
		return n;
	toolbar_t* r = search_rollout_parent_node(n->left, search);
	if (r)
		return r;
	r = search_rollout_parent_node(n->right, search);
	if (r)
		return r;
	return nullptr;
}
void div_drag::shift(int shift) {
	*div += (fabs(*div) >= 1.0f) ? (float)shift : (float)shift / size;
}
void clear_toolbars(toolbar_t* n) {
	if (!n)
		return;
	clear_toolbars(n->left);
	clear_toolbars(n->right);
	delete (n->left);
	delete (n->right);
	n->left = nullptr;
	n->right = nullptr;
}
}
