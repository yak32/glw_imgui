// glw_imgui
// Copyright (C) 2016 Iakov Sumygin - BSD license

#include "imgui_ui.h"
#include "imgui_platform.h"
#include "imgui_toolbars.h"
#include "imgui_rollout.h"

#include <stdio.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include <assert.h>

#define UI_MULTITHREADED

#ifdef _WIN32
#define snprintf _snprintf
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

#define MIN_SCROLL_AREA_WIDTH 40
#define MIN_SCROLL_AREA_HEIGHT 20

namespace imgui {

static const int MIN_SCROLL_HEIGHT = 10;



// moved
const int DEF_BUTTON_HEIGHT = 16;
const int ROLLOUT_START_WIDGET_ID = 10;

Ui::Ui(uint mode)
		: _width(0), _height(0), _left(false), _double_left(false),
		_keys_state(0), _prev_keys_state(0),
		_mx(-1), _my(-1), _scroll(0), _render_options(0), _item_height(DEF_BUTTON_HEIGHT),
		_active(0), _hot(0), _hot_to_be(0), _prev_enabled_id(0), _is_hot(false), _is_active(false),
		_went_active(false), _search_next_focus(false), _drag_x(0), _drag_y(0), _drag_orig(0),
		_widget_x(0), _widget_y(0), _widget_w(100), _rollout_width(0), _rollout_height(0),
		_rollout_left(0), _rollout_top(0), _inside_current_scroll(false), _area_id(0),
		_character(0), _widget_id(0), _focus(0), _row(0), _drag_item_width(0),
		_options(0), _alpha(255), _text_align(ALIGN_LEFT),
		_padding_left(DEFAULT_PADDING()), _padding_right(DEFAULT_PADDING()),
		_padding_top(DEFAULT_PADDING()), _padding_bottom(DEFAULT_PADDING()),
		_item_padding_left(DEFAULT_PADDING()), _item_padding_right(DEFAULT_PADDING()),
		_item_padding_top(DEFAULT_PADDING()), _item_padding_bottom(DEFAULT_PADDING()),
		_property_width(0.5f), _dragged_rollout_id(NULL),
		_focused_rollout_id(NULL), _cursor(0), _cursor_over_drag(0), _scroll_right(0),
		_scroll_area_top(0), _scroll_val(NULL), _focus_top(0), _focus_bottom(0),
		_scroll_id(0), _inside_scroll_area(false), _scroll_top(0), _scroll_bottom(0), _platform(nullptr),
		_edit_buffer_id(-1),
		_target_side(ROLLOUT_UNDEFINED),
		_target_rollout(NULL),
		_rollout_drag_div(),
		_toolbar_root(NULL), _rollout_last(NULL),
		_rqueue(nullptr), _rqueue_display(nullptr),
		_renderer(nullptr),
		_blend_texture(false),
		_depth(0),	 // ui depth (need to sorting windows properly, especially during moving rollouts)
		_mode(mode),
		_atlas(UNDEFINED_TEXTURE),
		_focus_rollout(nullptr)
{
	memset(_edit_buffer, 0, sizeof(_edit_buffer));
	memset(_drag_item, 0, sizeof(_drag_item));
	// colors
	_colors[BUTTON_COLOR_ACTIVE] = RGBA(128, 128, 128, 196);
	_colors[BUTTON_COLOR] = RGBA(128, 128, 128, 96);
	_colors[BUTTON_COLOR_FOCUSED] = RGBA(128, 128, 128, 120);
	_colors[EDIT_COLOR] = _colors[BUTTON_COLOR];
	_colors[EDIT_COLOR_ACTIVE] = _colors[BUTTON_COLOR_ACTIVE];
	_colors[COLLAPSE_COLOR] = RGBA(0, 0, 0, 196);
	_colors[COLLAPSE_COLOR_ACTIVE] = RGBA(0, 0, 0, 220);
	_colors[TEXT_COLOR_HOT] = RGBA(255, 196, 0, 255);
	_colors[TEXT_COLOR] = RGBA(255, 255, 255, 200);
	_colors[TEXT_COLOR_CHECKED] = RGBA(255, 255, 255, 200);
	_colors[TEXT_COLOR_DISABLED] = RGBA(128, 128, 128, 200);
	_colors[ROLLOUT_COLOR] = RGBA(0, 0, 0, 192);
	_colors[ROLLOUT_CAPTION_COLOR] = RGBA(0, 0, 0, 0); // non visible by default
	_colors[DRAG_COLOR] = RGBA(80, 80, 80, 40);
	_colors[SLIDER_BG] = RGBA(0, 0, 0, 126);
	_toolbar_root = new Toolbar(NULL);
	_rollout_last = _toolbar_root;
	_rqueue = &_rqueues[0];
	_rqueue_display = &_rqueues[1];
	_current_texture.id = UNDEFINED_TEXTURE;
	_white_texture.id = UNDEFINED_TEXTURE;
	_current_font = _fonts.end();

	int size = sizeof(_rqueues[0]);
	int n = 10;
}
Ui::~Ui() {
	destroy();
}
bool Ui::create(IPlatform* p, IRenderer* r){
	_platform = p;
	return render_init(r);
}
void Ui::destroy(){
	if (_toolbar_root) {
		clear_toolbars(_toolbar_root);
		_toolbar_root = nullptr;
		delete (_toolbar_root);
	}
	render_destroy();
}
bool Ui::any_active() const {
	return _active != 0;
}
bool Ui::is_item_active(uint id) const {
	return _active == id;
}
bool Ui::is_item_hot(uint id) const {
	return _hot == id;
}
bool Ui::is_item_focused(uint id) const {
	return _focus == id;
}
bool Ui::was_focused(uint id) const {
	return _focus == id;
}
bool Ui::is_edit_buffer(uint id) const {
	return _edit_buffer_id == id;
}
bool Ui::in_rect(int x, int y, int w, int h, bool checkScroll) const {
	return (!checkScroll || _toolbar_root) && _mx >= x && _mx <= x + w && _my >= y &&
		   _my <= y + h;
}
void Ui::clear_input() {
	// trick to have double click flag in input

	// clear all bits except double click
	_scroll = 0;
	_character = 0;
}
void Ui::clear_active() {
	_active = 0;
	_dragged_rollout_id = NULL;
	set_cursor(CURSOR_DEFAULT);
	// mark all UI for this frame as processed
	clear_input();
}
void Ui::set_active(uint id) {
	_active = id;
	_went_active = true;
}
void Ui::set_hot(uint id) {
	if (_hot != id)
		play_sound(SOUND_MOUSE_HOVER);

	_hot_to_be = id;
}
void Ui::play_sound(SOUNDS s) {
	// nothing yet
}
void Ui::set_focused(uint id) {
	_focus = id;
}
void Ui::set_edit_buffer_id(uint id) {
	_edit_buffer_id = id;
}
bool Ui::button_logic(uint id, bool over) {
	if (_options & INPUT_DISABLED)
		return false;

	bool was_focused = false;
	//if ((key_pressed(KEY_MOUSE_LEFT) || key_pressed(KEY_ENTER)) && is_item_focused(id)) {
	//	set_focused(0);
	//	was_focused = true;
	//}

	bool res = false;
	// process down
	if (!any_active()) {
		if (over)
			set_hot(id);
		if ( is_item_hot(id) && key_pressed(KEY_MOUSE_LEFT) ){
				set_active(id);
				if (!was_focused)
					set_focused(id);
		}
		// keyboard control
		if (is_item_focused(id) && key_pressed(KEY_ENTER)) {
			set_active(id);
		}
	}
	// if button is active, then react on left up
	if (is_item_active(id)) {
		_is_active = true;
		if (over)
			set_hot(id);
		if (key_released(KEY_MOUSE_LEFT)) {
			if (is_item_hot(id)) {
				res = true;
			}
			clear_active();
			play_sound(SOUND_CLICK);
		}
		if (key_released(KEY_ENTER)) {
			if (is_item_focused(id)) {
				res = true;
			}
			clear_active();
			play_sound(SOUND_CLICK);
		}
	}
	if (is_item_hot(id))
		_is_hot = true;
	return res;
}
bool Ui::edit_logic(uint id, bool was_focused, bool enabled, char* text, int buffer_len,
					bool* edit_finished) {
	if (edit_finished)
		*edit_finished = false;

	if (_options & INPUT_DISABLED)
		return false;

	if (!is_item_focused(id) && is_edit_buffer(id)) {
		// control have lost focus -> copy internal buffer into text
		strncpy(text, _edit_buffer, buffer_len);
		_edit_buffer[0] = 0;
		set_edit_buffer_id(0);

		// editing is complete
		if (edit_finished)
			*edit_finished = true;
		return true;
	}
	if (is_item_focused(id)) {
		// we can use edit buffer
		if (_edit_buffer_id == 0) {
			// control received focus -> copy text into internal buffer
			// strncpy(_edit_buffer, text, sizeof(_edit_buffer) / sizeof(_edit_buffer[0]));
			_edit_buffer_id = id;
			_edit_buffer[0] = 0;
		}
		else {
			// we still can not edit
			if (!is_edit_buffer(id))
				return false;

			strncpy(text, _edit_buffer, buffer_len);
		}
	}
	else
		return false;

	if (!buffer_len)
		return false;

	size_t len = strlen(_edit_buffer);
	if (key_released(KEY_ENTER)) {
		// return key - remove last character
		if (len > 0)
			_edit_buffer[len - 1] = 0;

		strncpy(text, _edit_buffer, buffer_len);
		return false;
	}
	if (key_released(KEY_ENTER)) {
		if (edit_finished) {
			*edit_finished = is_item_focused(id);
			IMGUI_LOG_INFO("on enter pressed, id: %d, edit_finished: %d", id, *edit_finished);
		}
		return true;
	}

	if (_character && (int)len < buffer_len - 1) {
		_edit_buffer[len] = _character;
		_edit_buffer[len + 1] = 0;
		strncpy(text, _edit_buffer, buffer_len);

		// value is changed
		return true;
	}
	return false;
}
bool Ui::combo_button_logic(uint id, bool over) {
	if (_options & INPUT_DISABLED)
		return false;

	bool res = false;
	// process down
	if (!any_active()) {
		if (over)
			set_hot(id);
		if (is_item_hot(id) && (key_pressed(KEY_MOUSE_LEFT) || key_pressed(KEY_ENTER))) {
			set_active(id);
		}
	}
	// if button is active, then react on left up
	if (is_item_active(id)) {
		_is_active = true;
		if (over)
			set_hot(id);
		if (key_released(KEY_MOUSE_LEFT)) {
			if (is_item_hot(id)) {
				res = true;
				set_focused(id);
			}
			clear_active();
			play_sound(SOUND_CLICK);
		}
	}
	if (is_item_hot(id))
		_is_hot = true;
	return res;
}

bool Ui::key_pressed(Key key) const {
	return !(_prev_keys_state & key) && (_keys_state & key);
}
bool Ui::key_released(Key key) const {
	return (_prev_keys_state & key) && !(_keys_state & key);
}

bool Ui::update_input(int mx, int my, int scroll, uint character, uint keys_state) {
	if (!_platform)
	{
		assert(false && "platform is not set");
		return false;
	}
	bool left = (keys_state & KEY_MOUSE_LEFT) != 0;
	bool double_left = false;
	//if (keys_state & MBUT_LEFT_DBL)
	//	double_left = true;
	_mx = mx;
	_my = my;
	_left = left;

	_double_left = double_left;

	_character = character;

	_prev_keys_state = _keys_state;
	_keys_state = keys_state;
	_scroll = scroll;

	if (key_pressed(KEY_MOUSE_LEFT))
		_platform->capture_mouse(true);
	else if (_left)
		_platform->capture_mouse(false);
	return true;
}

bool Ui::begin_frame(uint width, uint height, int mx, int my, int scroll,
		uint character, uint keys_state) {
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_rqueue->ready_to_render())
			return false;
	}

	_width = width;
	_height = height;
	update_input(mx, _height-my, scroll, character, keys_state);

	_hot = _hot_to_be;
	_hot_to_be = 0;

	// _render_options = 0;

	_went_active = false;
	_is_active = false;
	_is_hot = false;
	_search_next_focus = false;
	_prev_enabled_id = 0;

	_widget_x = 0;
	_widget_y = 0;
	_widget_w = 0;
	_rollout_width = 0;
	_rollout_left = 0;
	_options = 0;

	_area_id = 1;
	_widget_id = 0;
	_row = 0;
	_alpha = 255;
	_cursor_over_drag = false;

	// recalculate rollouts based on size of frame
	visit_rollout_node(_rollouts, _toolbar_root, 0, 0, width, height);

	// if we have dragged rollout, setup depth testing
	set_depth(0);
	return true;
}
void Ui::end_frame() {
	if (_dragged_rollout_id) {
		// user is dragging rollout, display move symbol
		size_t size = _rollouts.size();
		for (size_t i = 0; i < size; ++i) {
			Rollout* r = _rollouts[i];
			if (r == _dragged_rollout_id || !(r->options & ROLLOUT_ATTACHED))
				continue;
			if (in_rect(r->x, r->y, r->w, r->h, false)) {
				_target_side =
					rollout_move(_dragged_rollout_id, r, r->x + r->w / 2, r->y + r->h / 2);
				if (_target_side) {
					// user selected area to insert rollout
					_target_rollout = r;
				}
			}
		}
	}
	if (!_cursor_over_drag && !_left)
		set_cursor(CURSOR_DEFAULT); // restore to default cursor only of left button is not pressed

	clear_input();
	_rqueue->on_frame_finished();
}
void Ui::on_render_finished(){
	std::lock_guard<std::mutex> lock(_mutex);
	if (_rqueue->ready_to_render()) {
		// we can swap buffers
		_rqueue->prepare_render();
		_rqueue_display->on_frame_finished();
		std::swap(_rqueue, _rqueue_display); // swap render buffers to display new queue
		_rqueue->reset_gfx_cmd_queue(); // former display queue can be reset
	}
}
void Ui::cleanup() {
	_rollouts.clear();
	clear_toolbars(_toolbar_root);
	_rollout_last = _toolbar_root;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// static const int _item_height = 20;

const int Ui::SLIDER_HEIGHT() const {
	return _item_height + _item_height / 3;
}
const int Ui::SLIDER_MARKER_WIDTH() const {
	return _item_height / 3 * 2;
}
const int Ui::CHECK_SIZE() const {
	return _item_height / 2;
}
const int Ui::DEFAULT_SPACING() const {
	return _item_padding_left;
}
const int Ui::SCROLL_AREA_PADDING() const {
	return _item_height / 2;
}
const int Ui::INTEND_SIZE() const {
	return _item_height;
}
const int Ui::AREA_HEADER() const {
	return _item_height * 2;
}
const int Ui::TOOLBAR_HEADER() const {
	return _item_height / 4;
}
const int Ui::DEF_ROUND() const {
	return _item_height / 5;
}
const int Ui::DEFAULT_PADDING() const {
	return _item_height / 2;
}
const int DEFAULT_ROLLOUT_DEPTH = 0;
const int FOCUSED_ROLLOUT_DEPTH = 2;
const int FLOATING_ROLLOUT_DEPTH = 1;
const int ATTACHED_ROLLOUT_DEPTH = 0;
const int MOVE_SIGN_DEPTH = 3; // should be less then MAX_UI_LAYER_COUNT (10)

static const char* cursorMap[] = {"default", "resize_horz", "resize_vert", "resize_corner"};

bool Ui::begin_rollout(Rollout* pr, bool focused) {
	if (!pr)
		return false;
	Rollout& r = *pr;
	int area_header = AREA_HEADER();

	bool toolbar = (r.options & TOOLBAR) != 0;
	bool rollout_attached = (r.options & ROLLOUT_ATTACHED) != 0;

	r.process_animations(*this);

	// if toolbar
	if (toolbar)
		area_header = TOOLBAR_HEADER();

	int h;
	if (r.minimized)
		h = area_header + r.scroll;
	else
		h = r.h;

	_area_id++;
	_widget_id = 0;
	_rqueue->set_alpha(r.alpha);
	_scroll_id = get_control_id(_widget_id);
	_options = r.options;

	bool draw_scroll = true;
	if (r.scroll == SCROLL_DISABLED || _options & DISABLE_SCROLL ) {
		r.scroll = 0;
		draw_scroll = false;
	}
	if ( !(_options & DISPLAY_SCROLL_BARS) )
		draw_scroll = false;

	_rollout_left = r.x + _padding_left;
	_rollout_top = r.y + _padding_top;
	_rollout_width = r.w - _padding_left - _padding_right;
	if (draw_scroll)
		_rollout_width = r.w - SCROLL_AREA_PADDING() * 2 - _padding_left - _padding_right;
	_rollout_height = h - _padding_bottom - _padding_top;
	_widget_x = _rollout_left;
	_widget_y = r.y + h - area_header + r.scroll;
	_widget_w = _rollout_width;
	// _row = _options & TOOLBAR ? 1 : 0;
	_row = 0;

	int x = r.x, y = r.y, w = r.w;

	_scroll_right = _rollout_left + _rollout_width;
	_scroll_top = y - area_header + h;
	_scroll_bottom = y + _padding_bottom;
	_scroll_val = &r.scroll;

	_scroll_area_top = _widget_y;

	_focus_top = y - area_header;
	_focus_bottom = y - area_header + h;

	_inside_scroll_area = in_rect(x, y, w, h, false);
	_inside_current_scroll = _inside_scroll_area;

	bool ret_val = false;
	int caption_y = y + h - area_header + _item_height / 2 - 2;
	int caption_height = toolbar ? 0: _item_height + 3;

	// setup rollout depth
	if (_focused_rollout_id == pr)
		r.z = FOCUSED_ROLLOUT_DEPTH;
	else if (rollout_attached)
		r.z = ATTACHED_ROLLOUT_DEPTH;
	else
		r.z = FLOATING_ROLLOUT_DEPTH;

	if (!(r.options & ROLLOUT_HOLLOW)) {
		_rqueue->add_depth(r.z);
		_rqueue->add_rect(x, y, w, h, _colors[ROLLOUT_COLOR]);
		if (key_pressed(KEY_MOUSE_LEFT) && in_rect(x, y, w, h)) {
			// click on rollout -> increase depth
			_focused_rollout_id = pr;
		}

		bool cursor_over_drag;
		Rollout* draggedRolloutId = _dragged_rollout_id;
		// resize area
		if (!(_options & TOOLBAR) && _options & RESIZE_AREA && !r.minimized) {
			// dragging area
			int drag_id = -1;
			int ydiff = 0, xdiff = 0;
			// if area is active, then react on left up
			if (draggedRolloutId && key_released(KEY_MOUSE_LEFT)) {
				Rollout* drollout = _dragged_rollout_id;
				if (_target_side == ROLLOUT_LEFT) {
					insert_rollout(drollout, 0.5f, true, _target_rollout);
				}
				else if (_target_side == ROLLOUT_TOP) {
					insert_rollout(drollout, -0.5f, false, _target_rollout);
				}
				else if (_target_side == ROLLOUT_RIGHT) {
					insert_rollout(drollout, -0.5f, true, _target_rollout);
				}
				else if (_target_side == ROLLOUT_BOTTOM) {
					insert_rollout(drollout, 0.5f, false, _target_rollout);
				}
				else if (_target_side == ROLLOUT_CENTER) {
					insert_rollout(drollout, 0.0f, false, _target_rollout);
				}
				if (_target_side == ROLLOUT_LEFT_FIXED) {
					int neww = drollout->w > _target_rollout->w / 2 ? _target_rollout->w / 2
																	 : drollout->w;
					insert_rollout(drollout, (float)neww, true, _target_rollout);
				}
				else if (_target_side == ROLLOUT_TOP_FIXED) {
					int newh = drollout->h > _target_rollout->h / 2 ? _target_rollout->h / 2
																	 : drollout->h;
					insert_rollout(drollout, -(float)newh, false, _target_rollout);
				}
				else if (_target_side == ROLLOUT_RIGHT_FIXED) {
					int neww = drollout->w > _target_rollout->w / 2 ? _target_rollout->w / 2
																	 : drollout->w;
					insert_rollout(drollout, -(float)neww, true, _target_rollout);
				}
				else if (_target_side == ROLLOUT_BOTTOM_FIXED) {
					int newh = drollout->h > _target_rollout->h / 2 ? _target_rollout->h / 2
																	 : drollout->h;
					insert_rollout(drollout, (float)newh, false, _target_rollout);
				}
				clear_active(); // clear state to prevent
			}
			if (_options & DRAG_AREA && !r.minimized) {
				if (r.tabs.empty()) {
					if (system_drag(x, caption_y - caption_height / 2 + 5, w, caption_height, xdiff,
									ydiff, cursor_over_drag)) {
						r.x += xdiff;
						r.y += ydiff;
						if (key_pressed(KEY_MOUSE_LEFT)) {
							_target_side = ROLLOUT_UNDEFINED;
							_target_rollout = NULL;
							_dragged_rollout_id = pr;
							detach_rollout(&r); // if rollout attached, detach it
						}
					}
				}
			}
			cursor_over_drag = false;
			CURSOR cursor = CURSOR_DEFAULT;
			if (system_drag(x, y, SCROLL_AREA_PADDING() * 2, h, xdiff, ydiff, cursor_over_drag)) {
				if (!rollout_attached) {
					r.x += xdiff;
					r.w -= xdiff;
				}
				else if (_rollout_drag_div.div) {
					_rollout_drag_div.shift(xdiff);
				}
			}
			if (cursor_over_drag) {
				if (!rollout_attached ||
					find_div(_mx, _my, _toolbar_root, 0, 0, _width, _height, DEFAULT_PADDING())
						.div) {
					cursor = CURSOR_RESIZE_HORZ;
					_cursor_over_drag = true;
				}
			}
			if (system_drag(x + w - SCROLL_AREA_PADDING(), y, SCROLL_AREA_PADDING() * 2, h, xdiff,
							ydiff, cursor_over_drag)) {
				if (!rollout_attached) {
					r.w += xdiff;
				}
				else if (_rollout_drag_div.div) {
					_rollout_drag_div.shift(xdiff);
				}
			}
			if (cursor_over_drag) {
				if (!rollout_attached ||
					find_div(_mx, _my, _toolbar_root, 0, 0, _width, _height, DEFAULT_PADDING())
						.div) {
					cursor = CURSOR_RESIZE_HORZ;
					_cursor_over_drag = true;
				}
			}
			if (system_drag(x, y + h - SCROLL_AREA_PADDING() * 2 + 5, w,
							SCROLL_AREA_PADDING() * 2 - 5, xdiff, ydiff, cursor_over_drag)) {
				if (!rollout_attached)
					r.h += ydiff;
				else if (_rollout_drag_div.div)
					_rollout_drag_div.shift(ydiff);
			}
			if (cursor_over_drag) {
				if (!rollout_attached ||
					find_div(_mx, _my, _toolbar_root, 0, 0, _width, _height, DEFAULT_PADDING())
						.div) {
					cursor = CURSOR_RESIZE_VERT;
					_cursor_over_drag = true;
				}
			}
			if (system_drag(x, y - SCROLL_AREA_PADDING(), w, SCROLL_AREA_PADDING() * 2, xdiff,
							ydiff, cursor_over_drag)) {
				if (!rollout_attached) {
					r.y += ydiff;
					r.h -= ydiff;
				}
				else if (_rollout_drag_div.div)
					_rollout_drag_div.shift(ydiff);
			}
			if (cursor_over_drag) {
				if (!rollout_attached ||
					find_div(_mx, _my, _toolbar_root, 0, 0, _width, _height, DEFAULT_PADDING())
						.div) {
					cursor = CURSOR_RESIZE_VERT;
					_cursor_over_drag = true;
				}
			}
			int padd = SCROLL_AREA_PADDING() * 2;
			if (system_drag(x + w - padd, y - padd, padd * 2, padd * 2, xdiff, ydiff,
							cursor_over_drag)) {
				if (!rollout_attached) {
					r.w += xdiff;
					r.y += ydiff;
					r.h -= ydiff;
				}
			}
			if (cursor_over_drag) {
				if (!rollout_attached) {
					cursor = CURSOR_RESIZE_CORNER;
					_cursor_over_drag = true;
				}
			}
			if (!_left && cursor != CURSOR_DEFAULT)
				set_cursor(cursor);
		}
		// caption and close buttons
		int xdiff, ydiff;
		if (!toolbar) {
			if (!r.tabs.empty()) {
				// several rollouts in tabbed interface
				int width = _widget_w / r.tabs.size();
				int xx = _widget_x;
				for (int rollout_id : r.tabs) {
					Rollout* tr = _rollouts[rollout_id];
					if (system_tab(tr->name.c_str(), xx, caption_y, width - 2, caption_height,
								   tr == &r, xdiff, ydiff)) {
						if (tr != &r) {
							for (int r_id : r.tabs) _rollouts[r_id]->alpha = 0;
							tr->alpha = 255;
						}
					}
					if (xdiff || ydiff) {
						detach_rollout(tr);
						break;
					}
					// close button
					if (tr == &r || in_rect(xx, caption_y, width - 2, caption_height)) {
						int bx = xx + width - int(_item_height * 1.5f); // button position
						if (system_button("x", bx, y + h - area_header + _item_height / 2,
										  (int)(_item_height * 1.2f), _item_height, true)) {
							detach_rollout(tr);
							hide_rollout(tr);
							if (tr->is_visible())
								ret_val = true;
							break;
						}
					}
					xx += width;
				}
			}
			else if (caption_height){
				_rqueue->add_rect(x, caption_y, w, caption_height, _colors[ROLLOUT_CAPTION_COLOR]);
				_rqueue->add_text(x, y + h - area_header + _item_height / 2 - 2, _widget_w,
								  _item_height, ALIGN_LEFT, r.name.c_str(),
								  RGBA(255, 255, 255, 128));
				// close button
				if (system_button("x", x + w - (int)(_item_height * 2.0f),
								  y + h - area_header + _item_height / 2, _item_height,
								  _item_height, true)) {
					hide_rollout(pr);
					ret_val = true;
				}
			}
		}
	}
	// no clipping if dragging item is possible
	// if ( !(_options & DRAG_ITEM) )
	_rqueue->add_scissor(_rollout_left, _scroll_bottom, _scroll_right - _rollout_left,
						 _scroll_top - _scroll_bottom);

	// dragging control
	_widget_id = ROLLOUT_START_WIDGET_ID;

	if (&r == _focus_rollout && !r.focused) {
		// set focus to first item of the rollout
		r.focused = true;
		_search_next_focus = true;
	}

	// return _inside_scroll_area;
	return ret_val;
}
void Ui::end_rollout() {
	// prevent focus searching at the end of rollout
	_search_next_focus = false;

	// Disable scissoring.
	_rqueue->add_scissor(-1, -1, -1, -1);
	if (!(_options & DISABLE_SCROLL)) {

		// Draw scroll bar
		int x = _scroll_right + SCROLL_AREA_PADDING() / 2;
		int y = _scroll_bottom;
		int w = SCROLL_AREA_PADDING() * 2;
		int h = _scroll_top - _scroll_bottom;
		if (h > 0) {
			int stop = _scroll_area_top;
			int sbot = _widget_y;
			int sh = stop - sbot; // The scrollable area height.

			float barHeight = (float)h / (float)sh;
			if (barHeight < 1) {
				float barY = (float)(y - sbot) / (float)sh;
				if (barY < 0)
					barY = 0;
				if (barY > 1)
					barY = 1;

				// Handle scroll bar logic.
				uint hid = _scroll_id;
				int hx = x;
				int hy = y + (int)(barY * h);
				int hw = w;
				int hh = (int)(barHeight * h);

				if (_options & DISPLAY_SCROLL_BARS) {
					const int range = h - (hh - 1);
					bool over = in_rect(hx, hy, hw, hh);
					button_logic(hid, over);
					if (is_item_active(hid)) {
						float u = (float)(hy - y) / (float)range;
						if (_went_active) {
							_drag_y = _my;
							_drag_orig = u;
						}
						if (_drag_y != _my) {
							u = _drag_orig + (_my - _drag_y) / (float)range;
							if (u < 0)
								u = 0;
							if (u > 1)
								u = 1;
							*_scroll_val = (int)((1 - u) * (sh - h));
						}
					}
					// BG

					_rqueue->add_rect(x, y, w, h, RGBA(0, 0, 0, 196));
					// Bar
					hh = std::max(hh, 10);
					if (is_item_active(hid))
						_rqueue->add_rect(hx, hy, hw, hh, RGBA(255, 196, 0, 196));
					else
						_rqueue->add_rect(hx, hy, hw, hh, is_item_hot(hid) ? RGBA(255, 196, 0, 96)
							: RGBA(255, 255, 255, 64));
				}
			}

			// Handle mouse scrolling.
			if (_inside_scroll_area) { // && !any_active())
				if (_scroll) {
					*_scroll_val += 20 * _scroll;
					if (*_scroll_val < 0)
						*_scroll_val = 0;
					if (*_scroll_val > (sh - h))
						*_scroll_val = (sh - h);
				}

			}
//			else
//				*_scroll_val = SCROLL_DISABLED; // memorize state - no scroll
		}
	}
	_inside_current_scroll = false;
	_widget_id = 0;
	_row = 0;
}
uint Ui::get_control_id(uint widget_id) const {
	return (_area_id << 16) | widget_id;
}
bool Ui::start_control(bool enabled, int& x, int& y, int& w, int& h, uint& id, bool& over,
					   bool& was_focused) {

	id = get_control_id(_widget_id++);

	x = _widget_x;
	y = _widget_y - _item_height;
	w = _widget_w;
	h = _item_height;
	if (!_row)
		_widget_y -= _item_height + DEFAULT_SPACING();
	else
		_widget_x += w + DEFAULT_SPACING();

	if (_widget_id > 10 && (y + h < _scroll_bottom || y > _scroll_top))
		return false;

	over = enabled && in_rect(x, y, w, h);
	was_focused = is_item_focused(id);

	if (enabled && _search_next_focus && !was_focused) {
		// tab pressed -> set focus to current control
		_search_next_focus = false;
		set_focused(id);
	}
	else if (is_item_focused(id) ) {
		if (key_released(KEY_UP) ) {
			// don't allow to move focus up, if first control is focused
			if ( _prev_enabled_id >= get_control_id(ROLLOUT_START_WIDGET_ID))
				set_focused(_prev_enabled_id);
		}
		if (key_released(KEY_DOWN)) {
			// TAB pressed -> change focus to next id
			// focus will be automatically moved to next edit control (look edit_logic)
			_search_next_focus = true;
		}
	}
	if (enabled)
		_prev_enabled_id = id;
	return true;
}
color Ui::text_color_hot(uint id, bool enabled, bool focused) {
	if (enabled)
		return (is_item_hot(id) || focused) ? _colors[TEXT_COLOR_HOT] : _colors[TEXT_COLOR];
	return _colors[TEXT_COLOR_DISABLED];
}
color Ui::text_color(uint id, bool enabled) {
	return enabled ? _colors[TEXT_COLOR] : _colors[TEXT_COLOR_DISABLED];
}
color Ui::button_color(uint id, bool enabled) {
	if (is_item_active(id))
		return _colors[BUTTON_COLOR_ACTIVE];

	return is_item_focused(id) ? _colors[BUTTON_COLOR_FOCUSED]: _colors[BUTTON_COLOR];
}
color Ui::edit_color(uint id, bool enabled) {
	if (!is_item_focused(id))
		return is_item_active(id) ? _colors[EDIT_COLOR_ACTIVE] : _colors[EDIT_COLOR];

	return RGBA(_colors[EDIT_COLOR_ACTIVE], 150);
}
bool Ui::button(const char* text, bool enabled) {
	int x, y, w, h;
	uint id;
	bool over, was_focused;
	if (!start_control(enabled, x, y, w, h, id, over, was_focused))
		return false;

	bool res = enabled && button_logic(id, over);
	_rqueue->add_rounded_rect(x, y, w, h, DEF_ROUND(), button_color(id));
	_rqueue->add_text(x, y, w, h, _text_align, text, text_color_hot(id, enabled));
	return res;
}
bool Ui::button(const char* text, int x, int y, int w, int h, bool enabled) {
	uint id = get_control_id(_widget_id++);

	x += _rollout_left;
	y += _scroll_area_top;

	bool over = enabled && in_rect(x, y, w, h);
	bool res = enabled && button_logic(id, over);

	_rqueue->add_rounded_rect(x, y, w, h, DEF_ROUND(), button_color(id));
	_rqueue->add_text(x, y, _widget_w, _item_height, _text_align, text,
					  text_color(id, enabled));
	return res;
}
bool Ui::system_button(const char* text, int x, int y, int w, int h, bool enabled) {
	uint id = get_control_id(_widget_id++);
	bool over = enabled && in_rect(x, y, w, h);
	bool res = enabled && button_logic(id, over);
	_rqueue->add_text(x, y, w, _item_height, _text_align, text, text_color(id, enabled));
	return res;
}
bool Ui::system_tab(const char* text, int x, int y, int w, int h, bool checked, int& xmove,
					int& ymove) {
	xmove = ymove = 0;
	uint id = get_control_id(_widget_id++);

	bool over = in_rect(x, y, w, h);
	bool res = button_logic(id, over);

	if (over && key_pressed(KEY_MOUSE_LEFT)) {
		_drag_x = _mx;
		_drag_y = _my;
		_rollout_drag_div =
			find_div(_mx, _my, _toolbar_root, 0, 0, _width, _height, DEFAULT_PADDING());
	}
	if (is_item_active(id)) {
		xmove = _mx - _drag_x;
		ymove = _my - _drag_y;
		_drag_x = _mx;
		_drag_y = _my;
	}
	if (is_item_hot(id) || checked)
		_rqueue->add_rect(x, y, w, h, _colors[ROLLOUT_CAPTION_COLOR]);

	_rqueue->add_text(x, y, w, _item_height, _text_align, text, text_color_hot(id, checked));
	return res;
}
bool Ui::system_drag(int x, int y, int w, int h, int& xdiff, int& ydiff, bool& over) {
	// first widget id after area scroll - area drag control (caption)
	int id = get_control_id(_widget_id++);

	over = in_rect(x, y, w, h, false);
	if (over && key_pressed(KEY_MOUSE_LEFT)) {
		_drag_x = _mx;
		_drag_y = _my;
		_rollout_drag_div =
			find_div(_mx, _my, _toolbar_root, 0, 0, _width, _height, DEFAULT_PADDING());
	}
	bool res = button_logic(id, over);
	if (is_item_active(id)) {
		_rqueue->add_rect(x, y, w, h, _colors[DRAG_COLOR]);
		xdiff = _mx - _drag_x;
		ydiff = _my - _drag_y;
		_drag_x = _mx;
		_drag_y = _my;
	}
	return is_item_active(id);
}
bool Ui::item(const char* text, bool selected, bool enabled) {
	int x, y, w, h;
	uint id;
	bool over, was_focused;
	if (!start_control(enabled, x, y, w, h, id, over, was_focused))
		return false;

	bool res = enabled && button_logic(id, over);
	if (is_item_active(id) && !_drag_item[0] && !over && (_options & DRAG_ITEM)) {
		// start drag
		strncpy(_drag_item, text, sizeof(_drag_item) / sizeof(_drag_item[0]));
		_drag_item_width = w;
	}
	if (is_item_hot(id))
		_rqueue->add_rect(x, y, w, h, RGBA(255, 196, 0, is_item_active(id) ? 196 : 96));
	else if (selected)
		_rqueue->add_rect(x, y, w, h, RGBA(255, 196, 0, 70));

	_rqueue->add_text(x, y, _widget_w, _item_height, _text_align, text,
					  text_color(id, enabled));
	return res;
}
bool Ui::combo_item(const char* text, bool enabled) {
	int x, y, w, h;
	uint id;
	bool over, was_focused;
	if (!start_control(enabled, x, y, w, h, id, over, was_focused))
		return false;

	bool res = combo_button_logic(id, over);
	if (res) {
		// combo item clicked - clear active id,
		// to do not affect another controls
		set_focused(0);
	}
	if (is_item_hot(id))
		_rqueue->add_rect(x + w / 2, y, w / 2, h, RGBA(255, 196, 0, is_item_active(id) ? 196 : 96));

	_rqueue->add_text(x + w / 2, y, _widget_w, _item_height, _text_align, text,
					  text_color(id, enabled));
	return res;
}
// same as item, but displays only last part of filepath
bool Ui::file_item(const char* text, char slash, bool selected, bool enabled) {
	int x, y, w, h;
	uint id;
	bool over, was_focused;
	if (!start_control(enabled, x, y, w, h, id, over, was_focused))
		return false;

	bool res = enabled && button_logic(id, over);

	if (is_item_active(id) && !_drag_item[0] && !over && (_options & DRAG_ITEM)) {
		// start drag
		strncpy(_drag_item, text, sizeof(_drag_item) / sizeof(_drag_item[0]));
		_drag_item_width = w;
	}

	if (is_item_hot(id))
		_rqueue->add_rect(x, y, w, h, RGBA(255, 196, 0, is_item_active(id) ? 196 : 96));
	else if (selected)
		_rqueue->add_rect(x, y, w, h, RGBA(255, 196, 0, 70));

	// find slash symbol
	const char* filename = text;
	int len = strlen(text) - 1;
	for (; len >= 0; --len)
		if (text[len] == slash) {
			filename = &text[len + 1];
			break;
		}
	_rqueue->add_text(x, y, _widget_w, _item_height, _text_align, filename,
					  text_color(id, enabled));
	return res;
}

bool Ui::item_dropped(char* text, uint buffer_len, int& mouse_x, int& mouse_y) {
	if (!text || !buffer_len) {
		assert(false);
		return false;
	}

	if (!_drag_item[0]) {
		text[0] = 0;
		return false;
	}

	// report some information about dragging
	strncpy(text, _drag_item, buffer_len);
	mouse_x = _mx;
	mouse_y = _my;

	if (_drag_item[0] && !_left) {
		// stop drag process
		_drag_item[0] = 0;
		return true;
	}

	int x = _mx;
	int y = _my;
	int w = _drag_item_width;
	int h = _item_height;

	// render item

	// switch off clipping optimization
	// TODO: hack, find another way
	uint id = _widget_id;
	_widget_id = 0;

	_rqueue->add_rect(x, y, w, h, RGBA(255, 196, 0, 96));
	_rqueue->add_scissor(x, y, w - _item_height, h);
	_rqueue->add_text(x + w - _item_height / 2, y, _widget_w, _item_height, ALIGN_RIGHT,
					  _drag_item, _colors[TEXT_COLOR]);
	_rqueue->add_scissor(-1, -1, -1, -1); // disable scissor

	// restore
	_widget_id = id;
	return false;
}

bool Ui::check(const char* text, bool checked, bool enabled) {
	int x, y, w, h;
	uint id;
	bool over, was_focused;
	if (!start_control(enabled, x, y, w, h, id, over, was_focused))
		return false;

	bool res = enabled && button_logic(id, over);

	const int cx = x;
	const int cy = y + CHECK_SIZE() / 2;
	_rqueue->add_rounded_rect(x + CHECK_SIZE() - 3, cy - 3, CHECK_SIZE() + 6, CHECK_SIZE() + 6,
							  DEF_ROUND(), button_color(id, enabled));

	if (checked) {
		uint check_clr;
		if (enabled)
			check_clr = RGBA(255, 255, 255, is_item_active(id) ? 255 : 200);
		else
			check_clr = _colors[TEXT_COLOR_DISABLED];

		_rqueue->add_rounded_rect(x + CHECK_SIZE(), cy, CHECK_SIZE(), CHECK_SIZE(), DEF_ROUND(),
								  check_clr);
	}
	_rqueue->add_text(x + _item_height, y, _widget_w, _item_height, _text_align, text,
					  text_color_hot(id, enabled));
	return res;
}
bool Ui::button_check(const char* text, bool checked, bool enabled) {
	int x, y, w, h;
	uint id;
	bool over, was_focused;
	if (!start_control(enabled, x, y, w, h, id, over, was_focused))
		return false;

	bool res = enabled && button_logic(id, over);

	// check behavior, but button look
	_rqueue->add_rounded_rect(x, y, w, h, DEF_ROUND(), is_item_active(id) || checked
														   ? _colors[BUTTON_COLOR_ACTIVE]
														   : _colors[BUTTON_COLOR]);
	uint text_clr;
	if (is_item_focused(id))
		text_clr = _colors[TEXT_COLOR_HOT];
	else if (checked)
		text_clr = _colors[TEXT_COLOR_CHECKED];
	else
		text_clr = text_color_hot(id, enabled, checked);

	_rqueue->add_text(x, y, _widget_w, _item_height, _text_align, text, text_clr);
	return res;
}
void Ui::row(uint count) {
	// set new widget width
	int remainder = _row ? (_rollout_width - _widget_w) : _rollout_width;
	_widget_w = (remainder + DEFAULT_SPACING()) / count - DEFAULT_SPACING();
	_row++;
}
void Ui::set_widget_width(int width) {
	_widget_w = width;
}
int Ui::get_widget_width() const {
	return _widget_w;
}
void Ui::end_row() {
	// restore widget width
	if (_row == 1) {
		_widget_w = _rollout_width;
		_widget_x = _rollout_left;
		_widget_y -= _item_height + DEFAULT_SPACING();
	}
	_row--;
}
bool Ui::collapse(const char* text, bool checked, bool enabled) {
	int x, y, w, h;
	uint id;
	bool over, was_focused;
	if (!start_control(enabled, x, y, w, h, id, over, was_focused))
		return false;

	const int cx = x + CHECK_SIZE() / 2;
	const int cy = y + _item_height / 4;
	bool res = enabled && button_logic(id, over);
	// _rqueue->add_rect(x, y, w, h, is_item_active(id) ? _colors[COLLAPSE_COLOR_ACTIVE]
	//												 : _colors[COLLAPSE_COLOR]);
	unsigned char clr = enabled ? clr = 255 : clr = 128;
	;
	if (checked)
		_rqueue->add_triangle(cx, cy, CHECK_SIZE(), CHECK_SIZE(), 1,
							  RGBA(clr, clr, clr, is_item_active(id) ? 255 : 200));
	else
		_rqueue->add_triangle(cx, cy, CHECK_SIZE(), CHECK_SIZE(), 2,
							  RGBA(clr, clr, clr, is_item_active(id) ? 255 : 200));

	_rqueue->add_text(x + CHECK_SIZE(), y, w, h, _text_align, text, text_color_hot(id, enabled));
	return res;
}
bool Ui::combo(const char* text, const char* value, bool enabled) {
	int x, y, w, h;
	uint id;
	bool over, was_focused;
	if (!start_control(enabled, x, y, w, h, id, over, was_focused))
		return false;

	const int cx = x + CHECK_SIZE() / 2;
	const int cy = y + _item_height / 4;
	bool res = combo_button_logic(id, over);
	bool focused = is_item_focused(id);
	if (res && was_focused)
		set_focused(0); // clear focus, of combo was clicked second time

	unsigned char clr = enabled ? clr = 255 : clr = 128;
	if (!focused)
		_rqueue->add_triangle(cx, cy, CHECK_SIZE(), CHECK_SIZE(), 1,
							  RGBA(clr, clr, clr, is_item_active(id) ? 255 : 200));
	else
		_rqueue->add_triangle(cx, cy, CHECK_SIZE(), CHECK_SIZE(), 2,
							  RGBA(clr, clr, clr, is_item_active(id) ? 255 : 200));

	_rqueue->add_rounded_rect(x + w / 2, y, w / 2, h, DEF_ROUND(), button_color(id, enabled));
	_rqueue->add_text(x + CHECK_SIZE(), y, _widget_w, _item_height, _text_align, text,
					  text_color(id, true));
	_rqueue->add_text(x + w / 2, y, _widget_w, _item_height, _text_align, value,
					  text_color_hot(id, enabled, focused));
	return was_focused;
}
bool Ui::button_collapse(const char* text, bool enabled) {
	int x, y, w, h;
	uint id;
	bool over, was_focused;
	if (!start_control(enabled, x, y, w, h, id, over, was_focused))
		return false;

	bool res = enabled && button_logic(id, over);
	const int cx = x + _item_height / 2;
	const int cy = y + _item_height / 3;

	if (was_focused && res)
		set_focused(0); // close combo

	bool focused = is_item_focused(id);
	unsigned char clr;
	clr = enabled ? 255 : 128;
	_rqueue->add_rounded_rect(x, y, w, h, DEF_ROUND(), button_color(id, enabled));
	_rqueue->add_text(x + _item_height, y, _widget_w, _item_height, _text_align, text,
					  text_color_hot(id, enabled));
	if (!focused)
		_rqueue->add_triangle(cx, cy, _item_height / 2, _item_height / 2, 1,
							  RGBA(clr, clr, clr, is_item_active(id) ? 255 : 200));
	else
		_rqueue->add_triangle(cx, cy, _item_height / 2, _item_height / 2, 2,
							  RGBA(clr, clr, clr, is_item_active(id) ? 255 : 200));
	return focused;
}

bool Ui::file(const char* text, const char* value, bool enabled) {
	int x, y, w, h;
	uint id;
	bool over, was_focused;
	if (!start_control(enabled, x, y, w, h, id, over, was_focused))
		return false;

	bool res = enabled && button_logic(id, over);

	bool focused = is_item_focused(id);
	const int cx = x - CHECK_SIZE() / 2;
	const int cy = y - CHECK_SIZE() / 2;

	_rqueue->add_text(x, y, _widget_w, _item_height, _text_align, text,
					  text_color_hot(id, enabled));

	// file value
	_rqueue->add_rounded_rect(x + w / 2, y, w / 2, h, DEF_ROUND(), button_color(id, enabled));
	_rqueue->add_text(x + w / 2, y, _widget_w, _item_height, _text_align, value,
					  text_color_hot(id, enabled, focused));

	// clear focus
	if (focused)
		set_focused(0);

	return focused;
}

bool Ui::color_edit(const char* text, color clr, bool enabled, bool is_property) {
	int x, y, w, h;
	uint id;
	bool over, was_focused;
	if (!start_control(enabled, x, y, w, h, id, over, was_focused))
		return false;

	bool res = enabled && button_logic(id, over);
	bool focused = is_item_focused(id);

	int twidth = 0;
	int vwidth = w;
	if (is_property) {
		twidth = (int)(_widget_w * _property_width);
		vwidth = (int)(w * (1.0f - _property_width));
		_rqueue->add_text(x, y, twidth, _item_height, _text_align, text,
						  text_color_hot(id, true));
	}
	const uint red = clr & 0xff;
	const uint green = (clr >> 8) & 0xff;
	const uint blue = (clr >> 16) & 0xff;

	_rqueue->add_rounded_rect(
		x + twidth, y, vwidth, h, DEF_ROUND(),
		RGBA(red, green, blue, is_item_hot(id) || is_item_active(id) ? 180 : 255));

	// clear focus
	if (focused)
		set_focused(0);

	return focused;
}
void Ui::rectangle(int height, uint color){
	int x, y, w, h;
	uint id;
	bool over, was_focused;
	int button_height = _item_height;
	_item_height = height;
	if (!start_control(false, x, y, w, h, id, over, was_focused))
		return;

	_rqueue->add_rect(x, y, w, h, color);
	_item_height = button_height;
}
void Ui::rectangle(int x, int y, int width, int height, uint color) {
	if (y + height < 0 || y > _height ||
		x + width < 0 ||  x > _width)
		return;
	_rqueue->add_rect(x, y, width, height, color);
}

void Ui::triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint color) {
	// _rqueue->add_triangle(x, y, width, height, color);
}
void Ui::label(const char* text) {
	_widget_id++;
	int x = _widget_x;
	int y = _widget_y - _item_height;
	if (!_row)
		_widget_y -= _item_height;
	else
		_widget_x += _widget_w + DEFAULT_SPACING();
	if (_widget_id > 10 && (y < _scroll_bottom || y > _scroll_top))
		return;
	// _rqueue->add_text(x, y, _text_align, text, RGBA(255,255,255,255));
	_rqueue->add_text(x, y, _widget_w, _item_height, _text_align, text, _colors[TEXT_COLOR]);
}
bool Ui::edit(char* text, int buffer_len, bool* edit_finished, bool enabled) {
	int x, y, w, h;
	uint id;
	bool over, was_focused;
	if (!start_control(enabled, x, y, w, h, id, over, was_focused))
		return false;

	button_logic(id, over);
	bool res = edit_logic(id, was_focused, enabled, text, buffer_len, edit_finished);
	_rqueue->add_rounded_rect(x, y, w, h, DEF_ROUND(), edit_color(id, enabled));
	_rqueue->add_text(x, y, _widget_w, _item_height, _text_align, text,
					  text_color_hot(id, enabled));
	return res;
}
bool Ui::property(const char* name, char* text, int buffer_len, bool* edit_finished, bool enabled) {
	int x, y, w, h;
	uint id;
	bool over, was_focused;
	if (!start_control(enabled, x, y, w, h, id, over, was_focused))
		return false;

	button_logic(id, over);
	bool res = edit_logic(id, was_focused, enabled, text, buffer_len, edit_finished);

	// property name
	int twidth = (int)(_widget_w * _property_width);
	_rqueue->add_text(x, y, twidth, _item_height, _text_align, name,
					  text_color_hot(id, enabled));

	// property value
	int vwidth = (int)(w * (1.0f - _property_width));
	_rqueue->add_rounded_rect(x + twidth, y, vwidth, h, DEF_ROUND(), edit_color(id, enabled));
	_rqueue->add_text(x + twidth, y, vwidth, _item_height, _text_align, text,
					  text_color_hot(id, enabled));
	return res;
}
void Ui::value(const char* text) {
	const int x = _widget_x;
	const int y = _widget_y - _item_height;
	const int w = _widget_w;
	if (!_row)
		_widget_y -= _item_height;
	else
		_widget_x += w + DEFAULT_SPACING();
	if (_widget_id > 10 && (y < _scroll_bottom || y > _scroll_top))
		return;
	_rqueue->add_text(x + _item_height / 2, y, _widget_w, _item_height, ALIGN_RIGHT, text,
					  _colors[TEXT_COLOR]);
}
bool Ui::slider(const char* text, float* val, float vmin, float vmax, float vinc, bool* last_change,
				bool enabled) {
	int x, y, w, h;
	uint id;
	bool over, was_focused_temp;
	if (!start_control(enabled, x, y, w, h, id, over, was_focused_temp))
		return false;

	_rqueue->add_rounded_rect(x, y, w, h, DEF_ROUND(), _colors[SLIDER_BG]);
	const int range = w - SLIDER_MARKER_WIDTH();

	float u = (*val - vmin) / (vmax - vmin);
	if (u < 0)
		u = 0;
	if (u > 1)
		u = 1;
	int m = (int)(u * range);

	over = in_rect(x + m, y, SLIDER_MARKER_WIDTH(), SLIDER_HEIGHT());
	bool was_focused_before = is_item_focused(id);
	bool was_active_before = is_item_active(id);
	bool res = enabled && button_logic(id, over);
	bool was_focused = is_item_focused(id);
	bool isItemActive = is_item_active(id);
	bool valChanged = false;

	if (is_item_active(id)) {
		if (_went_active) {
			_drag_x = _mx;
			_drag_orig = u;
		}
		if (_drag_x != _mx) {
			u = _drag_orig + (float)(_mx - _drag_x) / (float)range;
			if (u < 0)
				u = 0;
			if (u > 1)
				u = 1;
			*val = vmin + u * (vmax - vmin);
			*val = floorf(*val / vinc) * vinc; // Snap to vinc
			m = (int)(u * range);
			valChanged = true;
		}
	}

	if (is_item_active(id))
		_rqueue->add_rounded_rect(x + m, y, SLIDER_MARKER_WIDTH(), SLIDER_HEIGHT(), DEF_ROUND(),
								  RGBA(255, 255, 255, 255));
	else
		_rqueue->add_rounded_rect(x + m, y, SLIDER_MARKER_WIDTH(), SLIDER_HEIGHT(), DEF_ROUND(),
								  is_item_hot(id) ? RGBA(255, 196, 0, 128)
												  : RGBA(255, 255, 255, 64));

	// TODO: fix this, take a look at 'nicenum'.
	int digits = (int)(ceilf(log10f(vinc)));
	char fmt[16];
	snprintf(fmt, 16, "%%.%df", digits >= 0 ? 0 : -digits);
	char msg[128];
	snprintf(msg, 128, fmt, *val);

	_rqueue->add_text(x + _item_height / 2, y, _widget_w, _item_height, _text_align, text,
					  text_color_hot(id, enabled));
	_rqueue->add_text(x + _item_height / 2, y, _widget_w, _item_height, ALIGN_RIGHT, msg,
					  text_color_hot(id, enabled));

	bool _lastchange = was_active_before && (!isItemActive) && was_focused;
	if (last_change)
		*last_change = _lastchange;

	return _lastchange || res || valChanged ||
		   (was_focused && !was_focused_before); // to catch last change in slider value
}
bool Ui::progress(float val, float vmin, float vmax, float vinc, uint color, bool enabled) {
	int x, y, w, h;
	uint id;
	bool over, was_focused;
	if (!start_control(enabled, x, y, w, h, id, over, was_focused))
		return false;

	_rqueue->add_rounded_rect(x, y, w, h, DEF_ROUND(), RGBA(255, 255, 255, 64));

	const int range = w;
	float u = (val - vmin) / (vmax - vmin);
	if (u < 0)
		u = 0;
	if (u > 1)
		u = 1;
	int m = (int)(u * range);

	// if (is_item_active(id))
	//	_rqueue->add_rounded_rect(x+m, y, SLIDER_MARKER_WIDTH(), SLIDER_HEIGHT(), 4,
	// RGBA(255,255,255,255));
	// else
	if (m > 0)
		_rqueue->add_rounded_rect(x, y, m, _item_height, DEF_ROUND(), color);

	return true;
}
void Ui::set_options(size_t _options) {
	_render_options = _options;
	_rqueue->set_render_options(_render_options);
}
void Ui::indent() {
	_widget_x += INTEND_SIZE();
	_rollout_left += INTEND_SIZE();
	_widget_w -= INTEND_SIZE();
	_rollout_width -= INTEND_SIZE();
}
void Ui::unindent() {
	_widget_x -= INTEND_SIZE();
	_rollout_left -= INTEND_SIZE();
	_widget_w += INTEND_SIZE();
	_rollout_width += INTEND_SIZE();
}
void Ui::separator(bool draw_line) {
	if (!_row)
		// _widget_y -= DEFAULT_SPACING()*3;
		_widget_y -= _item_height;
	else
		_widget_x += _item_height;

	if (draw_line) {
		int x = _widget_x;
		int y = _widget_y + _item_height / 2;
		int w = _widget_w;
		int h = _item_height / 4;
		_rqueue->add_rect(x, y, w, h, _colors[BUTTON_COLOR]);
	}
}
void Ui::draw_text(int x, int y, int align, const char* text, uint color) {
	if (_widget_id > 10 && (y < _scroll_bottom || y > _scroll_top))
		return;
	_rqueue->add_text(x, y, _widget_w, _item_height, align, text, color);
}
// return previous button height
uint Ui::set_item_height(uint button_height) {
	uint mem = _item_height;
	if (button_height)
		_item_height = button_height;

	_padding_left = DEFAULT_PADDING();
	_padding_top = DEFAULT_PADDING();
	_padding_right = DEFAULT_PADDING();
	_padding_bottom = DEFAULT_PADDING();
	return mem;
}
uint Ui::get_button_height() const{
	return _item_height;
}
void Ui::set_padding(int left, int top, int right, int bottom) {
	_padding_left = left;
	_padding_top = top;
	_padding_right = right;
	_padding_bottom = bottom;
}
void Ui::set_item_padding(int left, int top, int right, int bottom) {
	_item_padding_left = left;
	_item_padding_top = top;
	_item_padding_right = right;
	_item_padding_bottom = bottom;
}
void Ui::set_property_width(float w) {
	_property_width = w;
}
bool Ui::active_text(int x, int y, int align, const char* text, uint color, bool selected) {
	_widget_id++;
	uint id = _widget_id;

	size_t len = strlen(text);
	int w = _item_height * len;
	int h = _item_height;
	if (_widget_id > 10 && (y < _scroll_bottom || y > _scroll_top))
		return false;
	bool over = in_rect(x, y, w, h, false);
	bool res = button_logic(id, over);

	if (is_item_hot(id) || selected) {
		// convert to upper case
		char up[256];
		for (size_t i = 0; i < len + 1; ++i) up[i] = toupper(text[i]);

		_rqueue->add_text(x, y, _widget_w, _item_height, _text_align, up,
						  _colors[TEXT_COLOR_HOT]);
	}
	else
		_rqueue->add_text(x, y, _widget_w, _item_height, _text_align, text, color);

	return _left && is_item_hot(id);
}
bool Ui::texture(const char* path, const frect& rc, bool blend) {
	_rqueue->add_texture(path, rc, blend);
	return true;
}
bool Ui::texture(const char* path) {
	frect rc;
	rc.top = rc.left = 0.0f;
	rc.right = rc.bottom = 1.0f;
	_rqueue->add_texture(path, rc, false);
	return true;
}
void Ui::end_texture() {
	frect r;
	r.bottom = r.top = r.left = r.right = 0;
	_rqueue->add_texture(0, r, false);
}
bool Ui::font(const char* path, float height) {
	_rqueue->add_font(path, height);
	return true;
}
//check before item
bool Ui::check_rect(int mouse_x, int mouse_y, uint id) const{
	//if (id == CHECK_ITEM_RECT) {
	//	int x = _widget_x;
	//	int y = _widget_y - _item_height;
	//	int w = _widget_w;
	//	int h = _item_height;
	//	return mouse_x >= x && mouse_x <= x + w && mouse_y >= y && mouse_y <= y + h;
	//}
	//else if (id == CHECK_ROLLOUT_RECT) {
	//	int x = _rollout_left;
	//	int y = _rollout_top - _item_height;
	//	int w = _rollout_width;
	//	int h = _rollout_height;
	//	return mouse_x >= x && mouse_x <= x + w && mouse_y >= y && mouse_y <= y + h;
	//}
	return true;
}
void Ui::set_text_align(uint align) {
	_text_align = align;
}
uint Ui::get_text_align() const{
	return _text_align;
}
// bool Ui::custom(CUSTOM_RENDER_CALLBACK callback, int param, bool enabled) {
// 	_widget_id++;
// 	uint id = get_control_id(_widget_id);

// 	int x = _widget_x;
// 	int y = _widget_y - _item_height;
// 	int w = _widget_w;
// 	int h = _item_height;

// 	if (!_row)
// 		_widget_y -= _item_height + DEFAULT_SPACING();
// 	else
// 		_widget_x += w + DEFAULT_SPACING();

// 	bool over = enabled && in_rect(x, y, w, h);
// 	bool res = enabled && button_logic(id, over);

// 	// call custom drawing function
// 	if (_widget_id > 10 && (y < _scroll_bottom || y > _scroll_top)) {
// 		// rect is outside rollout
// 		callback(param, 0, 0, 0, 0, false, false);
// 		return false;
// 	}
// 	callback(param, x, y, w, h, over, is_item_hot(id));
// 	return res;
// }

void Ui::set_color(ColorScheme color_id, uint clr) {
	if (color_id >= MAX_COLORS) {
		assert(false && "ColorScheme and ColorScheme should be the same");
		return;
	}
	_colors[color_id] = clr;
}
void Ui::set_depth(int depth) {
	_rqueue->add_depth(depth);
}
uint Ui::get_color(ColorScheme id) const{
	if (id >= MAX_COLORS) {
		assert(false && "ColorScheme and ColorScheme should be the same");
		return 0;
	}
	return _colors[id];
}
void Ui::set_cursor(CURSOR cursor) {
	if (!_platform)
	{
		assert(false && "platform is not set");
		return;
	}
	if (cursor != _cursor) {
		_cursor = cursor;
		_platform->set_cursor(cursor);
	}
}
bool Ui::clear_focus() {
	clear_active();
	set_focused(0);
	set_edit_buffer_id(0);
	_edit_buffer[0] = 0;
	_search_next_focus = false;
	return true;
}
void Ui::get_input(int* mouse_x, int* mouse_y, uint* keys_state,
		uint* character, bool& left_pressed, bool& left_released) const{
	*keys_state = _keys_state;
	*mouse_x = _mx;
	*mouse_y = _my;
	*character = _character;

	left_pressed = key_pressed(KEY_MOUSE_LEFT);
	left_released = key_released(KEY_MOUSE_LEFT);
}
const gfx_cmd* Ui::get_render_queue(int& size) {
	size = _rqueue_display->get_size();
	return _rqueue_display->get_queue();
}
Toolbar* Ui::get_root_toolbar() {
	return _toolbar_root;
}
void Ui::set_root_toolbar(Toolbar* t) {
	clear_toolbars(_toolbar_root); // remove current root toolbars
	delete (_toolbar_root);
	_toolbar_root = t;
	_rollout_last = _toolbar_root;
}
Ui::Rollouts& Ui::get_rollouts() {
	return _rollouts;
}
void Ui::set_focus_rollout(Rollout* r) {
	if (_focus_rollout)
		_focus_rollout->focused = false;
	_focus_rollout = r;
}
Rollout* Ui::get_focus_rollout() {
	return _focus_rollout;
}
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
