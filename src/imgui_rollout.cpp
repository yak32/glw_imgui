// glw_imgui
// Copyright (C) 2016 Iakov Sumygin - BSD license

#include "imgui_ui.h"
#include "imgui_rollout.h"
#include <assert.h>
#include <algorithm>

namespace imgui {

static const int DEFAULT_ROLLOUT_DEPTH = 0;
static const int FOCUSED_ROLLOUT_DEPTH = 2;
static const int FLOATING_ROLLOUT_DEPTH = 1;
static const int ATTACHED_ROLLOUT_DEPTH = 0;
static const int MOVE_SIGN_DEPTH = 3; // should be less then MAX_UI_LAYER_COUNT (10)

static const int MAX_TABS = 20;
Rollout::Rollout(int _x, int _y, int _w, int _h, int _z)
	: x(_x), y(_y), w(_w), h(_h), z(_z), scroll(0), target_scroll(0), animation_speed(0),
	  alpha(255), animation_type(ANIMATION_NONE), minimized(false), name(""), options(0), id(0),
	  focused(false) {}

void Rollout::set(int _x, int _y, int _w, int _h, int _z, bool _visible, bool _minimized,
				  int _alpha_inc) {
	x = _x;
	y = _y;
	w = _w;
	h = _h;
	z = _z;
	alpha = _visible ? 255 : 0;
	animation_speed = _alpha_inc;
	scroll = 0;
	minimized = _minimized;
}
void Rollout::set(const char* _name, int _options, bool _visible, bool _minimized, int _alpha_inc) {
	x = y = w = h = z = 0;
	alpha = _visible ? 255 : 0;
	animation_speed = _alpha_inc;
	scroll = 0;
	minimized = _minimized;
	name = _name;
	options = _options;
}
int Rollout::top() const {
	return y + h;
}
bool Rollout::is_visible() const {
	return alpha != 0;
}

void Rollout::process_animations(Ui& ui) {
	if (animation_type == Rollout::ANIMATION_NONE)
		return;

	if (animation_type == Rollout::ANIMATION_SHOW) {
		int alpha_speed = (int)(0.016f * 255.0f * 3.0f); // 60 fps - TODO: add timer
		if (alpha_speed > 255)
			alpha_speed = 45;

		alpha += animation_speed * alpha_speed;
		if (alpha <= 0) {
			if (animation_speed < 0)
				animation_speed = 0; // stop decrementing alpha
			alpha = 0;
			ui.detach_rollout(this);
		}
		if (alpha > 255) {
			if (animation_speed > 0)
				animation_speed = 0; // stop decrementing alpha
			alpha = 255;
		}
	}
	else if (animation_type == Rollout::ANIMATION_SCROLL) {
		int speed = (int)(0.016f * 255.0f * 3.0f); // 60 fps - TODO: add timer
		if (speed > 255)
			speed = 45;

		scroll += animation_speed * speed;
		if ((animation_speed < 0.0f && scroll < target_scroll) ||
			(animation_speed > 0.0f && scroll > target_scroll)) {
			scroll = target_scroll;
			animation_type = ANIMATION_NONE;
		}
	}
}
bool Ui::rollout_move_rect(int x, int y, int w, int h) {
	_widget_id++;
	bool over = in_rect(x, y, w, h, false);
	_rqueue->add_rect(x, y, w, h, over ? RGBA(14, 130, 156, 240) : RGBA(14, 130, 156, 180));
	return over;
}

RolloutMoveSide Ui::rollout_move(Rollout* dr, Rollout* r, int x, int y) {
	set_depth(MOVE_SIGN_DEPTH);

	// center
	RolloutMoveSide side = ROLLOUT_UNDEFINED;
	bool selected = false;
	side = ROLLOUT_UNDEFINED;
	int rsize = _item_height + _item_height / 3;
	if (rollout_move_rect(x - rsize, y - rsize, rsize * 2, rsize * 2)) {
		_rqueue->add_rect(r->x, r->y, r->w, r->h, RGBA(14, 99, 156, 180));
		side = ROLLOUT_CENTER;
	}
	// sides
	if (rollout_move_rect(x - rsize * 3 - 2, y - rsize, rsize * 2, rsize * 2)) {
		_rqueue->add_rect(r->x, r->y, r->w / 2, r->h, RGBA(14, 99, 156, 180));
		side = ROLLOUT_LEFT;
	}
	if (rollout_move_rect(x - rsize, y + rsize + 2, rsize * 2, rsize * 2)) {
		_rqueue->add_rect(r->x, r->y + r->h / 2, r->w, r->h / 2, RGBA(14, 99, 156, 180));
		side = ROLLOUT_TOP;
	}
	if (rollout_move_rect(x + rsize + 2, y - rsize, rsize * 2, rsize * 2)) {
		_rqueue->add_rect(r->x + r->w / 2, r->y, r->w / 2, r->h, RGBA(14, 99, 156, 180));
		side = ROLLOUT_RIGHT;
	}
	if (rollout_move_rect(x - rsize, y - rsize * 3 - 2, rsize * 2, rsize * 2)) {
		_rqueue->add_rect(r->x, r->y, r->w, r->h / 2, RGBA(14, 99, 156, 180));
		side = ROLLOUT_BOTTOM;
	}
	if (rollout_move_rect(x - rsize * 4 - 4, y - rsize, rsize, rsize * 2)) {
		int neww = dr->w > r->w / 2 ? r->w / 2 : dr->w;
		_rqueue->add_rect(r->x, r->y, neww, r->h, RGBA(14, 99, 156, 180));
		side = ROLLOUT_LEFT_FIXED;
	}
	if (rollout_move_rect(x - rsize, y + rsize * 3 + 4, rsize * 2, rsize)) {
		int newh = dr->h > r->h / 2 ? r->h / 2 : dr->h;
		_rqueue->add_rect(r->x, r->y + r->h - newh, r->w, newh, RGBA(14, 99, 156, 180));
		side = ROLLOUT_TOP_FIXED;
	}
	if (rollout_move_rect(x + rsize * 3 + 4, y - rsize, rsize, rsize * 2)) {
		int neww = dr->w > r->w / 2 ? r->w / 2 : dr->w;
		_rqueue->add_rect(r->x + r->w - neww, r->y, neww, r->h, RGBA(14, 99, 156, 180));
		side = ROLLOUT_RIGHT_FIXED;
	}
	if (rollout_move_rect(x - rsize, y - rsize * 4 - 4, rsize * 2, rsize)) {
		int newh = dr->h > r->h / 2 ? r->h / 2 : dr->h;
		_rqueue->add_rect(r->x, r->y, r->w, newh, RGBA(14, 99, 156, 180));
		side = ROLLOUT_BOTTOM_FIXED;
	}
	return side;
}

bool Ui::show_rollout(Rollout* r, bool animate) {
	// rollout is closing
	if (r->is_visible())
		return false;
	if (animate) {
		r->animation_speed = 1;
		r->alpha++;
	}
	else
		r->alpha = 255;

	return r->alpha >= 255;
}
bool Ui::hide_rollout(Rollout* r, bool animate) {
	if (!r)
		return false;
	// rollout is closing
	if (!r->is_visible())
		return false;
	if (animate) {
		r->animation_speed = -1;
		r->alpha--;
	}
	else
		r->alpha = 0;
	return r->alpha <= 0;
}

void Ui::scroll_rollout(Rollout* r, int scroll_val, bool animate, float animation_speed) {
	if (r->scroll + scroll_val < 0)
		scroll_val = -r->scroll;
	if (!r) {
		assert(false);
		return;
	}
	if (animate) {
		// finish previous animation
		if (r->animation_type == Rollout::ANIMATION_SCROLL)
			r->scroll = r->target_scroll;

		r->animation_type = Rollout::ANIMATION_SCROLL;
		r->animation_speed = animation_speed;
		r->target_scroll = r->scroll + scroll_val;
	}
	else {
		r->scroll += scroll_val;
	}
}

// insert rollout to another rollout (tabs)
bool Ui::insert_rollout(Rollout* r, float div, bool horz, Rollout* rollout) {
	if (!r)
		return false;
	// insert into toolbar binary tree
	Toolbar* lastNode = _rollout_last;
	if (rollout) {
		if (div == 0.0f) {
			assert(rollout->is_visible()); // rollout to attach should be visible
			// tabbed interface
			if (rollout->tabs.empty())
				rollout->tabs.push_back(rollout->id); // insert yourself to tabs first

			// add rollout to tabs of parent rollout
			rollout->tabs.push_back(r->id);
			//for (int rollout_id : rollout->tabs) _rollouts[rollout_id]->alpha = 0;
			r->alpha = 0;
			r->options |= ROLLOUT_ATTACHED;
			return true;
		}
		lastNode = search_rollout_node(_toolbar_root, rollout, true);
		if (!lastNode) {
			IMGUI_LOG_ERROR("failed to find rollout node");
			return 0;
		}
		Toolbar* mem_n = lastNode->add_rollout(lastNode->rollout, -div,
											   horz); // minus to move prev rollout to the right
		lastNode->rollout = NULL;
		lastNode->div = div;
		lastNode = mem_n;
		div = 1.0f; // reset div
	}
	r->options |= ROLLOUT_ATTACHED;
	_rollout_last = lastNode->add_rollout(r, div, horz);
	return true;
}
Rollout* Ui::create_rollout(const char* name, int options) {
	Rollout* r = new Rollout;
	r->id = _rollouts.size();

	_rollouts.push_back(r);
	r->name = name;
	r->options = options;
	return r;
}
bool Ui::remove_rollout(Rollout* r) {
	Rollouts::iterator i = std::find(_rollouts.begin(), _rollouts.end(), r);
	if (i == _rollouts.end()) {
		IMGUI_LOG_ERROR("failed to remove rollout");
		return true;
	}
	detach_rollout(r);
	delete r;
	return true;
}
Rollout* Ui::find_rollout(const char* name) {
	for (auto r : _rollouts)
		if (r->name == name)
			return r;
	return nullptr;
}
bool Ui::is_rollout_visible(Rollout* r) const {
	return r ? r->is_visible() : false;
}
void Ui::detach_tabbed_rollout(Rollout* r) {
	// rollout can be in tabbed interface
	auto& tabs = r->tabs;
	tabs.erase(std::remove(tabs.begin(), tabs.end(), r->id), tabs.end());
	_rollouts[tabs[0]]->alpha = 255;
	if (tabs.size() == 1) {
		Rollout::tabs_array_t temp;
		r->tabs.swap(temp);
	}

	r->options &= ~ROLLOUT_ATTACHED;
	r->z = FLOATING_ROLLOUT_DEPTH;
}
bool Ui::detach_rollout(Rollout* r) {
	if (!r)
		return false;

	if (!r->tabs.empty()) {
		if (r->tabs.at(0) == r->id) {
			Toolbar* n = search_rollout_node(_toolbar_root, r);
			if (n) {
				// move second tabbed rollout to the node
				std::swap(r->tabs.at(0), r->tabs.at(1));
				n->rollout = _rollouts[r->tabs.at(0)];
				n->rollout->alpha = 255;
			}
		}
		detach_tabbed_rollout(r);
		return true;
	}

	Toolbar* n = search_rollout_parent_node(_toolbar_root, r);
	if (!n)
		return false; // no issue, rollout can be detached
	if (n->left && n->left->rollout == r) {
		delete (n->left);
		n->left = NULL;
		if (n->right) {
			Toolbar* mem = n->right;
			*n = *n->right;
			delete (mem);
		}
	}
	else {
		delete (n->right);
		n->right = NULL;
		if (n->left) {
			Toolbar* mem = n->left;
			*n = *n->left;
			delete (mem);
		}
	}
	_rollout_last = _toolbar_root;
	r->options &= ~ROLLOUT_ATTACHED;
	r->z = FLOATING_ROLLOUT_DEPTH;
	return true;
}
bool Ui::set_rollout_rect(Rollout* r, int x, int y, int w, int h) {
	if (!r)
		return false;
	r->x = x;
	r->y = y;
	r->w = w;
	r->h = h;
	return true;
}
bool Ui::get_rollout_rect(Rollout* r, int& x, int& y, int& w, int& h) {
	if (!r)
		return false;
	x = r->x;
	y = r->y;
	w = r->w;
	h = r->h;
	return true;
}
const char* Ui::get_rollout_name(Rollout* r) const {
	return r ? r->name.c_str() : NULL;
}
bool Ui::scroll_rollout(Rollout* r, int scroll, SCROLL_MODE mode) {
	if (!r)
		return false;

	switch (mode) {
	case SCROLL_START:
		r->scroll = scroll;
		break;
	case SCROLL_END:
		break;
	case SCROLL_CURRENT:
		r->scroll += scroll;
		break;
	}
	return true;
}
bool Ui::hit_test(int x, int y) const {
	size_t size = _rollouts.size();
	for (size_t i = 0; i < size; ++i) {
		Rollout& r = *_rollouts[i];
		if (!(r.options & ROLLOUT_HOLLOW) && r.is_visible() && x >= r.x && x <= r.x + r.w &&
			y >= r.y && y <= r.y + r.h)
			return true;
	}
	return false;
}
}
