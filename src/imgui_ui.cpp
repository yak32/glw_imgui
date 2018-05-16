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

Ui::Ui(uint mode)
	: _width(0), _height(0), _left(false), _double_left(false), _keys_state(0), _prev_keys_state(0),
	  _mx(-1), _my(-1), _scroll(0), _render_options(0), _item_height(DEF_BUTTON_HEIGHT), _active(0),
	  _hot(0), _hot_to_be(0), _prev_enabled_id(0), _is_hot(false), _is_active(false),
	  _went_active(false), _search_next_focus(false), _drag_x(0), _drag_y(0), _drag_orig(0),
	  _widget_x(0), _widget_y(0), _widget_w(100), _rollout_width(0), _rollout_height(0),
	  _rollout_left(0), _rollout_top(0), _inside_current_scroll(false), _area_id(0), _character(0),
	  _widget_id(0), _focus(0), _row(0), _drag_item_width(0), _options(0), _alpha(255),
	  _text_align(ALIGN_LEFT), _padding_left(default_padding()), _padding_right(default_padding()),
	  _padding_top(default_padding()), _padding_bottom(default_padding()),
	  _item_padding_left(default_padding()), _item_padding_right(default_padding()),
	  _item_padding_top(default_padding()), _item_padding_bottom(default_padding()),
	  _property_width(0.5f), _dragged_rollout_id(NULL), _focused_rollout_id(NULL), _cursor(0),
	  _cursor_over_drag(0), _scroll_right(0), _scroll_area_top(0), _scroll_val(NULL), _focus_top(0),
	  _focus_bottom(0), _inside_scroll_area(false), _scroll_top(0),
	  _scroll_bottom(0), _platform(nullptr), _edit_buffer_id(0), _target_side(ROLLOUT_UNDEFINED),
	  _target_rollout(NULL), _rollout_drag_div(), _toolbar_root(NULL), _rollout_last(NULL),
	  _rqueue(nullptr), _rqueue_display(nullptr), _renderer(nullptr), _blend_texture(false),
	  _depth(0), // ui depth (need to sorting windows properly, especially during moving rollouts)
	  _mode(mode), _atlas(UNDEFINED_TEXTURE), _focus_rollout(nullptr), _rollout_root(nullptr) {
	memset(_edit_buffer, 0, sizeof(_edit_buffer));
	memset(_drag_item, 0, sizeof(_drag_item));
	// colors
	_theme.button_color_active = RGBA(128, 128, 128, 196);
	_theme.button_color = RGBA(128, 128, 128, 96);
	_theme.button_color_focused = RGBA(128, 128, 128, 120);
	_theme.edit_color = _theme.button_color;
	_theme.edit_color_active = _theme.button_color_active;
	_theme.collapse_color = RGBA(0, 0, 0, 196);
	_theme.collapse_color_active = RGBA(0, 0, 0, 220);
	_theme.text_color_hot = RGBA(255, 196, 0, 255);
	_theme.text_color = RGBA(255, 255, 255, 200);
	_theme.text_color_checked = RGBA(255, 255, 255, 200);
	_theme.text_color_disabled = RGBA(128, 128, 128, 200);
	_theme.rollout_color = RGBA(0, 0, 0, 192);
	_theme.rollout_caption_color = RGBA(0, 122, 204, 200); // non visible by default
	_theme.drag_color = RGBA(80, 80, 80, 40);
	_theme.slider_bg = RGBA(0, 0, 0, 126);
	_toolbar_root = new Toolbar(NULL);
	_rollout_last = _toolbar_root;
	_rqueue = &_rqueues[0];
	_rqueue_display = &_rqueues[1];
	_current_texture.id = UNDEFINED_TEXTURE;
	_white_texture.id = UNDEFINED_TEXTURE;
	_current_font = _fonts.end();

	int size = sizeof(_rqueues[0]);

	_rollout_root = create_rollout("root", ROLLOUT_HOLLOW | WND_STYLE);
	insert_rollout(_rollout_root, 1, true, NULL);
}
Ui::~Ui() {
	destroy();
}
bool Ui::create(IPlatform* p, IRenderer* r) {
	_platform = p;
	return render_init(r);
}
void Ui::destroy() {
	if (_toolbar_root) {
		clear_toolbars(_toolbar_root);
		delete (_toolbar_root);
		_toolbar_root = nullptr;
	}
	size_t size = _rollouts.size();
	for (size_t i = 0; i < size; ++i)
		delete _rollouts[i];

	_rollout_root = nullptr;
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
	return (!checkScroll || _toolbar_root) && _mx >= x && _mx <= x + w && _my >= y && _my <= y + h;
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
	// if ((key_pressed(KEY_MOUSE_LEFT) || key_pressed(KEY_ENTER)) && is_item_focused(id)) {
	//	set_focused(0);
	//	was_focused = true;
	//}

	bool res = false;
	// process down
	if (!any_active()) {
		if (over)
			set_hot(id);
		if (is_item_hot(id) && key_pressed(KEY_MOUSE_LEFT)) {
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
	if (key_released(KEY_RETURN)) {
		// return key - remove last character
		bool ret = false;
		if (len > 0) {
			_edit_buffer[len - 1] = 0;
			ret = true;
		}
		strncpy(text, _edit_buffer, buffer_len);
		return ret;
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
	if (!_platform) {
		assert(false && "platform is not set");
		return false;
	}
	bool left = (keys_state & KEY_MOUSE_LEFT) != 0;
	bool double_left = false;
	// if (keys_state & MBUT_LEFT_DBL)
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

bool Ui::begin_frame(uint width, uint height, int mx, int my, int scroll, uint character,
					 uint keys_state) {
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_rqueue->ready_to_render())
			return false;
	}

	_width = width;
	_height = height;
	update_input(mx, _height - my, scroll, character, keys_state);

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
	_widget_id = 0;
	_row = 0;
	_alpha = 255;
	_cursor_over_drag = false;

	// recalculate rollouts based on size of frame
	visit_rollout_node(_rollouts, _toolbar_root, 0, 0, width, height);

	// if we have dragged rollout, setup depth testing
	set_depth(0);

	// lets render root rollout
	begin_rollout(get_root_rollout(), false);
	end_rollout();
	return true;
}
void Ui::end_frame() {
	if (_dragged_rollout_id) {
		// user is dragging rollout, display move symbol
		size_t size = _rollouts.size();
		for (size_t i = 0; i < size; ++i) {
			Rollout* r = _rollouts[i];
			if (!r->is_visible() || r == _dragged_rollout_id || !(r->options & ROLLOUT_ATTACHED))
				continue;

			if (in_rect(r->x, r->y, r->w, r->h, false)) {
				_target_side = rollout_move(_dragged_rollout_id, r, r->x + r->w / 2, r->y + r->h / 2);
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
void Ui::on_render_finished() {
	std::lock_guard<std::mutex> lock(_mutex);
	if (_rqueue->ready_to_render()) {
		// we can swap buffers
		_rqueue->prepare_render();
		_rqueue_display->on_frame_finished();
		std::swap(_rqueue, _rqueue_display); // swap render buffers to display new queue
		_rqueue->reset_gfx_cmd_queue();		 // former display queue can be reset
	}
}
void Ui::cleanup() {
	_rollouts.clear();
	clear_toolbars(_toolbar_root);
	_rollout_last = _toolbar_root;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// static const int _item_height = 20;

const int Ui::slider_height() const {
	return _item_height + _item_height / 3;
}
const int Ui::slider_marker_width() const {
	return _item_height / 3 * 2;
}
const int Ui::check_size() const {
	return _item_height / 2;
}
const int Ui::default_spacing() const {
	return _item_padding_left;
}
const int Ui::padding() const {
	return _item_height / 2;
}
const int Ui::intend_size() const {
	return _item_height;
}
const int Ui::area_header() const {
	return _item_height * 2;
}
const int Ui::toolbar_header() const {
	return _item_height / 4;
}
const int Ui::default_round() const {
	return _item_height / 5;
}
const int Ui::default_padding() const {
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
	int areaheader = area_header();

	bool toolbar = (r.options & TOOLBAR) != 0;
	bool rollout_attached = (r.options & ROLLOUT_ATTACHED) != 0;

	r.process_animations(*this);

	// if toolbar
	if (toolbar)
		areaheader = toolbar_header();

	int h;
	if (r.minimized)
		h = areaheader + r.scroll;
	else
		h = r.h;

	_area_id = r.id;
	_widget_id = 0;
	_rqueue->set_alpha(r.alpha);
	_options = r.options;
	_alpha = r.alpha;

	bool draw_scroll = true;
	if (r.scroll == SCROLL_DISABLED || _options & DISABLE_SCROLL) {
		r.scroll = 0;
		draw_scroll = false;
	}
	if (!(_options & DISPLAY_SCROLL_BARS))
		draw_scroll = false;

	_rollout_left = r.x + _padding_left;
	_rollout_top = r.y + _padding_top;
	_rollout_width = r.w - _padding_left - _padding_right;
	if (draw_scroll)
		_rollout_width = r.w - padding() * 2 - _padding_left - _padding_right;
	_rollout_height = h - _padding_bottom - _padding_top;
	_widget_x = _rollout_left;
	_widget_y = r.y + h - areaheader + r.scroll;
	_widget_w = _rollout_width;
	// _row = _options & TOOLBAR ? 1 : 0;
	_row = 0;

	int x = r.x, y = r.y, w = r.w;

	_scroll_right = _rollout_left + _rollout_width;
	_scroll_top = y + h - areaheader;
	_scroll_bottom = y + _padding_bottom;
	_scroll_val = &r.scroll;

	_scroll_area_top = _widget_y;

	_focus_top = y - areaheader;
	_focus_bottom = y - areaheader + h;

	_inside_scroll_area = in_rect(x, y, w, h, false);
	_inside_current_scroll = _inside_scroll_area;

	bool ret_val = false;
	int caption_y = y + h - areaheader + _item_height / 2 - 2;
	int caption_height = toolbar ? 0 : _item_height + 3;

	// setup rollout depth
	if (_focused_rollout_id == &r)
		r.z = FOCUSED_ROLLOUT_DEPTH;
	else if (rollout_attached)
		r.z = ATTACHED_ROLLOUT_DEPTH;
	else
		r.z = FLOATING_ROLLOUT_DEPTH;

	if (r.is_visible()) {
		if (!(r.options & ROLLOUT_HOLLOW)) {
			_rqueue->add_depth(r.z);
			_rqueue->add_rect(x, y, w, h, _theme.rollout_color);

			if (key_pressed(KEY_MOUSE_LEFT) && in_rect(x, y, w, h)) {
				// click on rollout -> increase depth
				_focused_rollout_id = &r;
			}
			process_rollout_resize(r, x, y, w, h, caption_y, caption_height);
		}
		// caption and close buttons
		if (!toolbar)
			if (!render_caption(r, x, y, w, h, caption_y, caption_height, areaheader))
				ret_val = false;
	}

	// no clipping if dragging item is possible
	// if ( !(_options & DRAG_ITEM) )
	_rqueue->add_scissor(_rollout_left, _scroll_bottom, _scroll_right - _rollout_left,
						 _scroll_top - _scroll_bottom);

	// dragging control
	_widget_id = ROLLOUT_CONTROLS_COUNT;

	if (&r == _focus_rollout && !r.focused) {
		// set focus to first item of the rollout
		r.focused = true;
		_search_next_focus = true;
	}
	return ret_val;
}

bool Ui::render_caption(Rollout& r, int x, int y, int w, int h, int caption_y, int caption_height, int area_header){
	bool ret_val = false;
	if (r.tabs.empty()) {
		if (&r != get_root_rollout()) {
			_rqueue->add_rect(x, caption_y, w, caption_height, _theme.rollout_caption_color);
			_rqueue->add_text(x, y + h - area_header + _item_height / 2 - 2, _widget_w,
				_item_height, ALIGN_LEFT, r.name.c_str(),
				RGBA(255, 255, 255, 128));
			// close button
			if (system_button(get_control_id(ROLLOUT_CLOSE_WINDOW_ID),
				"x", x + w - (int)(_item_height * 2.0f),
				y + h - area_header + _item_height / 2, _item_height,
				_item_height, true)) {
				hide_rollout(&r);
				ret_val = true;
			}
		}
	}
	else if (caption_height) {
        if (!render_rollout_tabs(r, x, y, h, caption_y, caption_height, area_header))
            ret_val = false;
	}
	return ret_val;
}
void Ui::switch_tab(Rollout* r) {
	Toolbar* n = search_rollout_node(_toolbar_root, r);
	if (n) {
		n->rollout->alpha = 0;
		n->rollout->tabs.swap(r->tabs);
		n->rollout = r;
		r->alpha = 255;
	}
}

int Ui::render_rollout_tabs(Rollout& r, int x, int y, int h, int caption_y, int caption_height, int area_header) {
	int ret_val = false;
	int xdiff = 0, ydiff = 0;

	int count = (int)r.tabs.size();
	bool tabs_with_root = false;
	for (int rollout_id : r.tabs)
		if (rollout_id == get_root_rollout()->id)
			count--;

	// several rollouts in tabbed interface
	int width = _widget_w / count;
	int xx = _widget_x;
	for (int rollout_id : r.tabs) {
		Rollout* tab_rollout = _rollouts[rollout_id];
		if (tab_rollout == get_root_rollout())
			continue;

		bool cursor_over_drag = false;
		CURSOR cursor = CURSOR_DEFAULT;

		// we reuse ROLLOUT_MOVE_ROLLOUT_ID to be able to continue dragging of rollout after detach
        uint tab_control_id = get_control_id(tab_rollout->id, ROLLOUT_MOVE_ROLLOUT_ID);
		if (system_tab(tab_control_id,
						tab_rollout->name.c_str(), xx, caption_y, width - 2, caption_height,
						tab_rollout == &r, xdiff, ydiff, cursor_over_drag))
		{
			if (key_pressed(KEY_MOUSE_LEFT)){
				r.alpha = 0;
				tab_rollout->alpha = 255;
				r.tabs.swap(tab_rollout->tabs);
				Toolbar* n = search_rollout_node(_toolbar_root, &r);
				if (n)
					n->rollout = tab_rollout;
			}
            if (abs(xdiff) > 4 || abs(ydiff) > 4){
                tab_rollout->x += xdiff;
                tab_rollout->y += ydiff;
                _target_side = ROLLOUT_UNDEFINED;
                _target_rollout = NULL;
                _dragged_rollout_id = tab_rollout;
				if (tab_rollout->is_attached())
					detach_rollout(tab_rollout); // if rollout attached, detach it
				break; // tabs array is changed, break the loop
            }
		}

		// close button
		if (in_rect(xx, caption_y, width - 2, caption_height)) {
			int bx = xx + width - int(_item_height * 1.5f); // button position
			if (system_button(get_control_id(tab_rollout->id, ROLLOUT_CLOSE_WINDOW_ID),
								"x", bx, y + h - area_header + _item_height / 2,
								(int)(_item_height * 1.2f), _item_height, true))
			{
				detach_rollout(tab_rollout);
				hide_rollout(tab_rollout);
				if (tab_rollout->is_visible())
					ret_val = true;
				break;
			}
		}
		xx += width;
	}
	return ret_val;
}
void Ui::process_rollout_resize(Rollout& r, int x, int y, int w, int h, int caption_y, int caption_height){
	Rollout* draggedRolloutId = _dragged_rollout_id;

	bool rollout_attached = (r.options & ROLLOUT_ATTACHED) != 0;

	// resize area
	if ((_options & TOOLBAR) || !(_options & RESIZE_AREA) || r.minimized)
		return;

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
		else if (_target_side == ROLLOUT_CENTER && _target_rollout != get_root_rollout()) {
			insert_rollout(drollout, 0.0f, false, _target_rollout);
		}
		if (_target_side == ROLLOUT_LEFT_FIXED) {
			int neww =
				drollout->w > _target_rollout->w / 2 ? _target_rollout->w / 2 : drollout->w;
			insert_rollout(drollout, (float)neww, true, _target_rollout);
		}
		else if (_target_side == ROLLOUT_TOP_FIXED) {
			int newh =
				drollout->h > _target_rollout->h / 2 ? _target_rollout->h / 2 : drollout->h;
			insert_rollout(drollout, -(float)newh, false, _target_rollout);
		}
		else if (_target_side == ROLLOUT_RIGHT_FIXED) {
			int neww =
				drollout->w > _target_rollout->w / 2 ? _target_rollout->w / 2 : drollout->w;
			insert_rollout(drollout, -(float)neww, true, _target_rollout);
		}
		else if (_target_side == ROLLOUT_BOTTOM_FIXED) {
			int newh =
				drollout->h > _target_rollout->h / 2 ? _target_rollout->h / 2 : drollout->h;
			insert_rollout(drollout, (float)newh, false, _target_rollout);
		}
		clear_active(); // clear state to prevent
	}

	bool cursor_over_drag = false;
	CURSOR cursor = CURSOR_DEFAULT;

	if (r.tabs.empty() && _options & DRAG_AREA && !r.minimized) {

		// we reuse ROLLOUT_MOVE_ROLLOUT_ID to be able to continue dragging of rollout after detach
		if (system_drag(get_control_id(ROLLOUT_MOVE_ROLLOUT_ID), x, caption_y - caption_height / 2 + 5, w, caption_height, xdiff,
						ydiff, cursor_over_drag)) {
			r.x += xdiff;
			r.y += ydiff;
			if (key_pressed(KEY_MOUSE_LEFT)) {
				_target_side = ROLLOUT_UNDEFINED;
				_target_rollout = NULL;
				_dragged_rollout_id = &r;
				detach_rollout(&r); // if rollout attached, detach it
			}
		}
	}

	if (system_drag(get_control_id(ROLLOUT_RESIZE_LEFT_ID), x, y, padding() * 2, h, xdiff, ydiff, cursor_over_drag)) {
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
			find_div(_mx, _my, _toolbar_root, 0, 0, _width, _height, default_padding())
				.div) {
			cursor = CURSOR_RESIZE_HORZ;
			_cursor_over_drag = true;
		}
	}

	if (system_drag(get_control_id(ROLLOUT_RESIZE_RIGHT_ID), x + w - padding(), y, padding() * 2, h, xdiff,
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
			find_div(_mx, _my, _toolbar_root, 0, 0, _width, _height, default_padding())
				.div) {
			cursor = CURSOR_RESIZE_HORZ;
			_cursor_over_drag = true;
		}
	}

	if (system_drag(get_control_id(ROLLOUT_RESIZE_TOP_ID), x, y + h - padding() * 2 + 5, w,
					padding() * 2 - 5, xdiff, ydiff, cursor_over_drag)) {
		if (!rollout_attached)
			r.h += ydiff;
		else if (_rollout_drag_div.div)
			_rollout_drag_div.shift(ydiff);
	}
	if (cursor_over_drag) {
		if (!rollout_attached ||
			find_div(_mx, _my, _toolbar_root, 0, 0, _width, _height, default_padding())
				.div) {
			cursor = CURSOR_RESIZE_VERT;
			_cursor_over_drag = true;
		}
	}

	if (system_drag(get_control_id(ROLLOUT_RESIZE_BOTTOM_ID), x, y - padding(), w, padding() * 2, xdiff,
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
			find_div(_mx, _my, _toolbar_root, 0, 0, _width, _height, default_padding())
				.div) {
			cursor = CURSOR_RESIZE_VERT;
			_cursor_over_drag = true;
		}
	}

	int padd = padding() * 2;
	if (system_drag(get_control_id(ROLLOUT_RESIZE_CORNER_ID), x + w - padd, y - padd, padd * 2, padd * 2, xdiff, ydiff,
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
void Ui::end_rollout() {
	// prevent focus searching at the end of rollout
	_search_next_focus = false;

	// Disable scissoring.
	_rqueue->add_scissor(-1, -1, -1, -1);
	if (!(_options & DISABLE_SCROLL) && _alpha) {
		// Draw scroll bar
		int x = _scroll_right + padding() / 2;
		int y = _scroll_bottom;
		int w = padding() * 2;
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
				uint hid = get_control_id(ROLLOUT_SCROLL_ID);
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
						_rqueue->add_rect(hx, hy, hw, hh, is_item_hot(hid)
															  ? RGBA(255, 196, 0, 96)
															  : RGBA(255, 255, 255, 64));
				}
				// Handle mouse scrolling.
				if (_inside_scroll_area) { // && !any_active())
					if (_scroll) {
						*_scroll_val += 20 * _scroll;
						if (*_scroll_val < 0)
							*_scroll_val = 0;
						if (*_scroll_val >(sh - h))
							*_scroll_val = (sh - h);
					}
				}
			}
		}
	}
	_inside_current_scroll = false;
	_widget_id = 0;
	_row = 0;
	_rqueue->set_alpha(255);
}
uint Ui::get_control_id(uint widget_id) const {
	return (_area_id << 16) | widget_id;
}
uint Ui::get_control_id(uint area_id, uint widget_id) const {
	return (area_id << 16) | widget_id;
}
bool Ui::start_control(bool enabled, int& x, int& y, int& w, int& h, uint& id, bool& over,
					   bool& was_focused) {
	if (!_alpha)
		return false; // rollout is invisible

	id = get_control_id(_widget_id++);

	x = _widget_x;
	y = _widget_y - _item_height;
	w = _widget_w;
	h = _item_height;
	if (!_row)
		_widget_y -= _item_height + default_spacing();
	else
		_widget_x += w + default_spacing();

	if (_widget_id > ROLLOUT_CONTROLS_COUNT && (y+h < _scroll_bottom || y > _scroll_top))
		return false;

	over = enabled && in_rect(x, y, w, h);
	was_focused = is_item_focused(id);

	if (enabled && _search_next_focus && !was_focused) {
		// tab pressed -> set focus to current control
		_search_next_focus = false;
		set_focused(id);
	}
	else if (is_item_focused(id)) {
		if (key_released(KEY_UP) || key_released(KEY_LEFT)) {
			// don't allow to move focus up, if first control is focused
			if (_prev_enabled_id >= get_control_id(ROLLOUT_CONTROLS_COUNT))
				set_focused(_prev_enabled_id);
		}
		if (key_released(KEY_DOWN) || key_released(KEY_RIGHT)) {
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
		return (is_item_hot(id) || focused) ? _theme.text_color_hot : _theme.text_color;
	return _theme.text_color_disabled;
}
color Ui::text_color(uint id, bool enabled) {
	return enabled ? _theme.text_color : _theme.text_color_disabled;
}
color Ui::button_color(uint id, bool enabled) {
	if (is_item_active(id))
		return _theme.button_color_active;

	return is_item_focused(id) ? _theme.button_color_focused : _theme.button_color;
}
color Ui::edit_color(uint id, bool enabled) {
	if (!is_item_focused(id))
		return is_item_active(id) ? _theme.edit_color_active : _theme.edit_color;

	return RGBA(_theme.edit_color_active, 150);
}
bool Ui::button(const char* text, bool enabled) {
	int x, y, w, h;
	uint id;
	bool over, was_focused;
	if (!start_control(enabled, x, y, w, h, id, over, was_focused))
		return false;

	bool res = enabled && button_logic(id, over);
	_rqueue->add_rounded_rect(x, y, w, h, default_round(), button_color(id));
	_rqueue->add_text(x, y, w, h, _text_align, text, text_color_hot(id, enabled));
	return res;
}

bool Ui::button(const char* text, int x, int y, int w, int h, bool enabled) {
	if (!_alpha)
		return false; // rollout is invisible

	uint id = get_control_id(_widget_id++);

	x += _rollout_left;
	y += _scroll_area_top;

	bool over = enabled && in_rect(x, y, w, h);
	bool res = enabled && button_logic(id, over);

	_rqueue->add_rounded_rect(x, y, w, h, default_round(), button_color(id));
	_rqueue->add_text(x, y, _widget_w, _item_height, _text_align, text, text_color(id, enabled));
	return res;
}

bool Ui::graph(const float* values, unsigned int size, unsigned int height, float minimum_value, float maximum_value, bool enabled) {
	int x, y, w, h;
	uint id;
	bool over, was_focused;

	if (!values){
		assert(false && "Ui::graph(): values are null");
		return false;
	}

	uint old_item_height = _item_height;
	_item_height = height;

	if (!start_control(enabled, x, y, w, h, id, over, was_focused)){
		_item_height = old_item_height;
		return false;
	}

	bool res = enabled && button_logic(id, over);
	
	float scale_vert = (float)h/(maximum_value-minimum_value);
	float scale_horz = (float)w/size;
	int y_start = -(int)(minimum_value * scale_vert);

	for (unsigned int i=0;i<size;++i){
		float v = values[i];
		int xx = (unsigned int)(i*scale_horz);
		int yy = (unsigned int)(v*scale_vert);
		_rqueue->add_rect(x+xx, y, ceil(scale_horz), yy+y_start, button_color(id));
		//_rqueue->add_line(x+xx1, y+yy1, x+xx2, y+yy1, button_color(id));
	}
	_item_height = old_item_height;
	return res;
}

bool Ui::system_button(uint id, const char* text, int x, int y, int w, int h, bool enabled) {
	bool over = enabled && in_rect(x, y, w, h);
	bool res = enabled && button_logic(id, over);
	_rqueue->add_text(x, y, w, _item_height, _text_align, text, text_color(id, enabled));
	return res;
}
bool Ui::system_drag(uint id, int x, int y, int w, int h, int& xdiff, int& ydiff, bool& over) {
	// first widget id after area scroll - area drag control (caption)
	over = in_rect(x, y, w, h, false);
	if (over && key_pressed(KEY_MOUSE_LEFT)) {
		_drag_x = _mx;
		_drag_y = _my;
		_rollout_drag_div =
			find_div(_mx, _my, _toolbar_root, 0, 0, _width, _height, default_padding());
	}
	bool res = button_logic(id, over);
	if (is_item_active(id)) {
		_rqueue->add_rect(x, y, w, h, _theme.drag_color);
		xdiff = _mx - _drag_x;
		ydiff = _my - _drag_y;
		_drag_x = _mx;
		_drag_y = _my;
	}
	return is_item_active(id);
}
bool Ui::system_tab(uint id, const char* text, int x, int y, int w, int h, bool checked, int& xdiff, int& ydiff, bool& over) {
	// first widget id after area scroll - area drag control (caption)
	over = in_rect(x, y, w, h, false);
	if (over && key_pressed(KEY_MOUSE_LEFT)) {
		_drag_x = _mx;
		_drag_y = _my;
		_rollout_drag_div =
			find_div(_mx, _my, _toolbar_root, 0, 0, _width, _height, default_padding());
	}
	bool res = button_logic(id, over);
	if (is_item_active(id) && (abs(_mx - _drag_x) > 15 || abs(_my - _drag_y) > 15)) {
		xdiff = _mx - _drag_x;
		ydiff = _my - _drag_y;
		_drag_x = _mx;
		_drag_y = _my;
	}
	if (is_item_hot(id) || checked)
		_rqueue->add_rect(x, y, w, h, _theme.rollout_caption_color);

	_rqueue->add_text(x, y, w, _item_height, _text_align, text, text_color_hot(id, true, checked));
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

	_rqueue->add_text(x, y, _widget_w, _item_height, _text_align, text, text_color(id, enabled));
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
	int len = (int)strlen(text) - 1;
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
	_rqueue->add_text(x + w - _item_height / 2, y, _widget_w, _item_height, ALIGN_RIGHT, _drag_item,
					  _theme.text_color);
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
	const int cy = y + check_size() / 2;
	_rqueue->add_rounded_rect(x + check_size() - 3, cy - 3, check_size() + 6, check_size() + 6,
							  default_round(), button_color(id, enabled));

	if (checked) {
		uint check_clr;
		if (enabled)
			check_clr = RGBA(255, 255, 255, is_item_active(id) ? 255 : 200);
		else
			check_clr = _theme.text_color_disabled;

		_rqueue->add_rounded_rect(x + check_size(), cy, check_size(), check_size(), default_round(),
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
	_rqueue->add_rounded_rect(x, y, w, h, default_round(), is_item_active(id) || checked
														   ? _theme.button_color_active
														   : _theme.button_color);
	uint text_clr;
	if (is_item_focused(id))
		text_clr = _theme.text_color_hot;
	else if (checked)
		text_clr = _theme.text_color_checked;
	else
		text_clr = text_color_hot(id, enabled, checked);

	_rqueue->add_text(x, y, _widget_w, _item_height, _text_align, text, text_clr);
	return res;
}
void Ui::row(uint count) {
	// set new widget width
	int remainder = _row ? (_rollout_width - _widget_w) : _rollout_width;
	_widget_w = (remainder + default_spacing()) / count - default_spacing();
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
		_widget_y -= _item_height + default_spacing();
	}
	_row--;
}
bool Ui::collapse(const char* text, bool checked, bool enabled) {
	int x, y, w, h;
	uint id;
	bool over, was_focused;
	if (!start_control(enabled, x, y, w, h, id, over, was_focused))
		return false;

	const int cx = x + check_size() / 2;
	const int cy = y + check_size() / 2;
	bool res = enabled && button_logic(id, over);
	// _rqueue->add_rect(x, y, w, h, is_item_active(id) ? _theme.collapse_color_active
	//												 : _theme.collapse_color);
	unsigned char clr = enabled ? clr = 255 : clr = 128;
	;
	if (checked)
		_rqueue->add_triangle(cx, cy, check_size(), check_size(), 1,
							  RGBA(clr, clr, clr, is_item_active(id) ? 255 : 200));
	else
		_rqueue->add_triangle(cx, cy, check_size(), check_size(), 2,
							  RGBA(clr, clr, clr, is_item_active(id) ? 255 : 200));

	_rqueue->add_text(x + check_size(), y, w, h, _text_align, text, text_color_hot(id, enabled));
	return res;
}
bool Ui::combo(const char* text, const char* value, bool enabled) {
	int x, y, w, h;
	uint id;
	bool over, was_focused;
	if (!start_control(enabled, x, y, w, h, id, over, was_focused))
		return false;

	const int cx = x + check_size() / 2;
	const int cy = y + _item_height / 4;
	bool res = combo_button_logic(id, over);
	bool focused = is_item_focused(id);
	if (res && was_focused)
		set_focused(0); // clear focus, of combo was clicked second time

	unsigned char clr = enabled ? clr = 255 : clr = 128;
	if (!focused)
		_rqueue->add_triangle(cx, cy, check_size(), check_size(), 1,
							  RGBA(clr, clr, clr, is_item_active(id) ? 255 : 200));
	else
		_rqueue->add_triangle(cx, cy, check_size(), check_size(), 2,
							  RGBA(clr, clr, clr, is_item_active(id) ? 255 : 200));

	_rqueue->add_rounded_rect(x + w / 2, y, w / 2, h, default_round(), button_color(id, enabled));
	_rqueue->add_text(x + check_size(), y, _widget_w, _item_height, _text_align, text,
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
	_rqueue->add_rounded_rect(x, y, w, h, default_round(), button_color(id, enabled));
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
	const int cx = x - check_size() / 2;
	const int cy = y - check_size() / 2;

	_rqueue->add_text(x, y, _widget_w, _item_height, _text_align, text,
					  text_color_hot(id, enabled));

	// file value
	_rqueue->add_rounded_rect(x + w / 2, y, w / 2, h, default_round(), button_color(id, enabled));
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
		_rqueue->add_text(x, y, twidth, _item_height, _text_align, text, text_color_hot(id, true));
	}
	const uint red = clr & 0xff;
	const uint green = (clr >> 8) & 0xff;
	const uint blue = (clr >> 16) & 0xff;

	_rqueue->add_rounded_rect(
		x + twidth, y, vwidth, h, default_round(),
		RGBA(red, green, blue, is_item_hot(id) || is_item_active(id) ? 180 : 255));

	// clear focus
	if (focused)
		set_focused(0);

	return focused;
}
void Ui::rectangle(int height, uint color) {
	int x, y, w, h;
	uint id;
	bool over, was_focused;
	int button_height = _item_height;
	_item_height = height;
	if (!start_control(false, x, y, w, h, id, over, was_focused)){
		_item_height = button_height;
		return;
	}

	_rqueue->add_rect(x, y, w, h, color);
	_item_height = button_height;
}
void Ui::rectangle(int x, int y, int width, int height, uint color) {
	if (y + height < 0 || y > (int)_height || x + width < 0 || x > (int)_width)
		return;
	_rqueue->add_rect(x, y, width, height, color);
}

void Ui::triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint color) {
	// _rqueue->add_triangle(x, y, width, height, color);
}
void Ui::label(const char* text) {
	_widget_id++;
	if (!_alpha)
		return;

	int x = _widget_x;
	int y = _widget_y - _item_height;
	if (!_row)
		_widget_y -= _item_height;
	else
		_widget_x += _widget_w + default_spacing();
	if (_widget_id > 10 && (y < _scroll_bottom || y > _scroll_top))
		return;
	// _rqueue->add_text(x, y, _text_align, text, RGBA(255,255,255,255));
	_rqueue->add_text(x, y, _widget_w, _item_height, _text_align, text, _theme.text_color);
}
bool Ui::edit(char* text, int buffer_len, bool* edit_finished, bool enabled) {
	int x, y, w, h;
	uint id;
	bool over, was_focused;
	if (!start_control(enabled, x, y, w, h, id, over, was_focused))
		return false;

	button_logic(id, over);
	bool res = edit_logic(id, was_focused, enabled, text, buffer_len, edit_finished);
	_rqueue->add_rounded_rect(x, y, w, h, default_round(), edit_color(id, enabled));
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
	_rqueue->add_text(x, y, twidth, _item_height, _text_align, name, text_color_hot(id, enabled));

	// property value
	int vwidth = (int)(w * (1.0f - _property_width));
	_rqueue->add_rounded_rect(x + twidth, y, vwidth, h, default_round(), edit_color(id, enabled));
	_rqueue->add_text(x + twidth, y, vwidth, _item_height, _text_align, text,
					  text_color_hot(id, enabled));
	return res;
}
void Ui::value(const char* text) {
	if (!_alpha)
		return;

	const int x = _widget_x;
	const int y = _widget_y - _item_height;
	const int w = _widget_w;
	if (!_row)
		_widget_y -= _item_height;
	else
		_widget_x += w + default_spacing();
	if (_widget_id > 10 && (y < _scroll_bottom || y > _scroll_top))
		return;
	_rqueue->add_text(x + _item_height / 2, y, _widget_w, _item_height, ALIGN_RIGHT, text,
					  _theme.text_color);
}
bool Ui::slider(const char* text, float* val, float vmin, float vmax, float vinc, bool* last_change,
				bool enabled) {
	int x, y, w, h;
	uint id;
	bool over, was_focused_temp;
	if (!start_control(enabled, x, y, w, h, id, over, was_focused_temp))
		return false;

	_rqueue->add_rounded_rect(x, y, w, h, default_round(), _theme.slider_bg);
	const int range = w - slider_marker_width();

	float u = (*val - vmin) / (vmax - vmin);
	if (u < 0)
		u = 0;
	if (u > 1)
		u = 1;
	int m = (int)(u * range);

	over = in_rect(x + m, y, slider_marker_width(), slider_height());
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
		_rqueue->add_rounded_rect(x + m, y, slider_marker_width(), slider_height(), default_round(),
								  RGBA(255, 255, 255, 255));
	else
		_rqueue->add_rounded_rect(x + m, y, slider_marker_width(), slider_height(), default_round(),
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

	_rqueue->add_rounded_rect(x, y, w, h, default_round(), RGBA(255, 255, 255, 64));

	const int range = w;
	float u = (val - vmin) / (vmax - vmin);
	if (u < 0)
		u = 0;
	if (u > 1)
		u = 1;
	int m = (int)(u * range);

	// if (is_item_active(id))
	//	_rqueue->add_rounded_rect(x+m, y, slider_marker_width(), slider_height(), 4,
	// RGBA(255,255,255,255));
	// else
	if (m > 0)
		_rqueue->add_rounded_rect(x, y, m, _item_height, default_round(), color);

	return true;
}
void Ui::set_options(uint _options) {
	_render_options = _options;
	_rqueue->set_render_options(_render_options);
}
void Ui::indent() {
	_widget_x += intend_size();
	_rollout_left += intend_size();
	_widget_w -= intend_size();
	_rollout_width -= intend_size();
}
void Ui::unindent() {
	_widget_x -= intend_size();
	_rollout_left -= intend_size();
	_widget_w += intend_size();
	_rollout_width += intend_size();
}
void Ui::separator(bool draw_line) {
	if (!_row)
		// _widget_y -= default_spacing()*3;
		_widget_y -= _item_height;
	else
		_widget_x += _item_height;

	if (draw_line) {
		int x = _widget_x;
		int y = _widget_y + _item_height / 2;
		int w = _widget_w;
		int h = _item_height / 4;
		if (y > _scroll_bottom && y < _scroll_top)
			_rqueue->add_rect(x, y, w, h, _theme.button_color);
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

	_padding_left = default_padding();
	_padding_top = default_padding();
	_padding_right = default_padding();
	_padding_bottom = default_padding();
	return mem;
}
uint Ui::get_item_height() const {
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

	int len = (int)strlen(text);
	int w = _item_height * len;
	int h = _item_height;
	if (_widget_id > 10 && (y < _scroll_bottom || y > _scroll_top))
		return false;
	bool over = in_rect(x, y, w, h, false);
	bool res = button_logic(id, over);

	if (is_item_hot(id) || selected) {
		// convert to upper case
		char up[256];
		for (int i = 0; i < len + 1; ++i) up[i] = toupper(text[i]);

		_rqueue->add_text(x, y, _widget_w, _item_height, _text_align, up, _theme.text_color_hot);
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
// check before item
bool Ui::check_rect(int mouse_x, int mouse_y, uint id) const {
	// if (id == CHECK_ITEM_RECT) {
	//	int x = _widget_x;
	//	int y = _widget_y - _item_height;
	//	int w = _widget_w;
	//	int h = _item_height;
	//	return mouse_x >= x && mouse_x <= x + w && mouse_y >= y && mouse_y <= y + h;
	//}
	// else if (id == CHECK_ROLLOUT_RECT) {
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
uint Ui::get_text_align() const {
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
// 		_widget_y -= _item_height + default_spacing();
// 	else
// 		_widget_x += w + default_spacing();

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
const Theme& Ui::get_theme() const{
	return _theme;
}
void Ui::set_theme(const Theme& theme){
	_theme = theme;
}
void Ui::set_depth(int depth) {
	_rqueue->add_depth(depth);
}
void Ui::set_cursor(CURSOR cursor) {
	if (!_platform) {
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
void Ui::get_input(int* mouse_x, int* mouse_y, uint* keys_state, uint* character,
				   bool& left_pressed, bool& left_released) const {
	*keys_state = _keys_state;
	*mouse_x = _mx;
	*mouse_y = _my;
	*character = _character;

	left_pressed = key_pressed(KEY_MOUSE_LEFT);
	left_released = key_released(KEY_MOUSE_LEFT);
}
const gfx_cmd* Ui::get_render_queue(uint& size) {
	size = _rqueue_display->get_size();
	return _rqueue_display->get_queue();
}
Toolbar* Ui::get_root_toolbar() {
	return _toolbar_root;
}
Rollout* Ui::get_root_rollout(){
	return _rollout_root;
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
void Ui::set_color(ColorScheme color_id, color_t clr) {
	if (color_id >= MAX_COLORS) {
		assert(false && "ColorScheme and ColorScheme should be the same");
		return;
	}
	switch(color_id){
		case ROLLOUT_COLOR:
			_theme.rollout_color = clr;
			break;
		case ROLLOUT_CAPTION_COLOR:
			_theme.rollout_caption_color = clr;
			break;
		case BUTTON_COLOR:
			_theme.button_color = clr;
			break;
		case BUTTON_COLOR_ACTIVE:
			_theme.button_color_active = clr;
			break;
		case BUTTON_COLOR_FOCUSED:
			_theme.button_color_focused = clr;
			break;
		case EDIT_COLOR:
			_theme.edit_color = clr;
			break;
		case EDIT_COLOR_ACTIVE:
			_theme.edit_color_active = clr;
			break;
		case COLLAPSE_COLOR:
			_theme.collapse_color = clr;
			break;
		case COLLAPSE_COLOR_ACTIVE:
			_theme.collapse_color_active = clr;
			break;
		case TEXT_COLOR:
			_theme.text_color = clr;
			break;
		case TEXT_COLOR_HOT:
			_theme.text_color_hot = clr;
			break;
		case TEXT_COLOR_CHECKED:
			_theme.text_color_checked = clr;
			break;
		case TEXT_COLOR_DISABLED:
			_theme.text_color_disabled = clr;
			break;
		case DRAG_COLOR:
			_theme.drag_color = clr;
			break;
		case SLIDER_BG:
			_theme.slider_bg = clr;
			break;
		default:
			assert(false && "color is undefined");
	};
}
color_t Ui::get_color(ColorScheme id) const {
	if (id >= MAX_COLORS) {
		assert(false && "ColorScheme and ColorScheme should be the same");
		return 0;
	}
	switch(id){
		case ROLLOUT_COLOR:
			return _theme.rollout_color;
		case ROLLOUT_CAPTION_COLOR:
			return _theme.rollout_caption_color;
		case BUTTON_COLOR:
			return _theme.button_color;
		case BUTTON_COLOR_ACTIVE:
			return _theme.button_color_active;
		case BUTTON_COLOR_FOCUSED:
			return _theme.button_color_focused;
		case EDIT_COLOR:
			return _theme.edit_color;
		case EDIT_COLOR_ACTIVE:
			return _theme.edit_color_active;
		case COLLAPSE_COLOR:
			return _theme.collapse_color;
		case COLLAPSE_COLOR_ACTIVE:
			return _theme.collapse_color_active;
		case TEXT_COLOR:
			return _theme.text_color;
		case TEXT_COLOR_HOT:
			return _theme.text_color_hot;
		case TEXT_COLOR_CHECKED:
			return _theme.text_color_checked;
		case TEXT_COLOR_DISABLED:
			return _theme.text_color_disabled;
		case DRAG_COLOR:
			return _theme.drag_color;
		case SLIDER_BG:
			return _theme.slider_bg;
		default:
			assert(false && "color is undefined");
	};
	return 0;

}
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
