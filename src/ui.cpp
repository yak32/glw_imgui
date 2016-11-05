// glw_imgui
// Copyright (C) 2016 Iakov Sumygin - BSD license

#include <stdio.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include <assert.h>

#include "ui.h"
#include "platform.h"
#include "toolbars.h"
#include "rollout.h"

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
#define DEF_ROUND 3

namespace imgui {

static const int MIN_SCROLL_HEIGHT = 10;

// moved
static const int g_button_height = 16;

Ui::Ui(IPlatform& platform)
	: m_platform(platform), m_width(0), m_height(0), m_left(false), m_double_left(false), m_left_pressed(false),
	  m_left_released(false), m_double_left_released(false), m_mx(-1), m_my(-1), m_scroll(0),
	  m_buttons(0), m_render_options(0), m_button_height(g_button_height), m_active(0), m_hot(0),
	  m_hot_to_be(0), m_is_hot(false), m_is_active(false), m_went_active(false),
	  m_search_next_focus(false), m_drag_x(0), m_drag_y(0), m_drag_orig(0), m_widget_x(0),
	  m_widget_y(0), m_widget_w(100), m_rollout_width(0), m_rollout_height(0), m_rollout_left(0),
	  m_rollout_top(0), m_inside_current_scroll(false), m_area_id(0), m_widget_id(0), m_key(0),
	  m_focus(0), m_edit_buffer_id(0), m_row(0), m_drag_item_width(0), m_options(0), m_alpha(255),
	  m_text_align(ALIGN_LEFT), m_padding_left(DEFAULT_PADDING), m_padding_top(DEFAULT_PADDING),
	  m_padding_right(DEFAULT_PADDING), m_padding_bottom(DEFAULT_PADDING), m_property_width(0.5f),
	  m_dragged_rollout_id(nullptr), m_focused_rollout_id(nullptr), m_cursor(0),
	  m_cursor_over_drag(0), m_scroll_right(0), m_scroll_area_top(0), m_scroll_val(nullptr),
	  m_focus_top(0), m_focus_bottom(0), m_scroll_id(0), m_inside_scroll_area(false),
	  m_scroll_top(0), m_scroll_bottom(0) {
	memset(m_edit_buffer, 0, sizeof(m_edit_buffer));
	memset(m_drag_item, 0, sizeof(m_drag_item));
	// colors
	m_colors[BUTTON_COLOR_ACTIVE] = RGBA(128, 128, 128, 196);
	m_colors[BUTTON_COLOR] = RGBA(128, 128, 128, 96);
	m_colors[EDIT_COLOR] = m_colors[BUTTON_COLOR];
	m_colors[EDIT_COLOR_ACTIVE] = m_colors[BUTTON_COLOR_ACTIVE];
	m_colors[COLLAPSE_COLOR] = RGBA(0, 0, 0, 196);
	m_colors[COLLAPSE_COLOR_ACTIVE] = RGBA(0, 0, 0, 220);
	m_colors[TEXT_COLOR_HOT] = RGBA(255, 196, 0, 255);
	m_colors[TEXT_COLOR] = RGBA(255, 255, 255, 200);
	m_colors[TEXT_COLOR_DISABLED] = RGBA(128, 128, 128, 200);
	m_colors[ROLLOUT_COLOR] = RGBA(0, 0, 0, 192);
	m_colors[ROLLOUT_CAPTION_COLOR] = RGBA(0, 0, 0, 0); // non visible by default
	m_colors[DRAG_COLOR] = RGBA(80, 80, 80, 40);
	m_colors[SLIDER_BG] = RGBA(0, 0, 0, 126);
	m_toolbar_root = new Toolbar(nullptr);
	m_rollout_last = m_toolbar_root;
}
Ui::~Ui() {
	clear_toolbars(m_toolbar_root);
	delete (m_toolbar_root);
}

bool Ui::any_active() const {
	return m_active != 0;
}
bool Ui::is_item_active(uint id) const {
	return m_active == id;
}
bool Ui::is_item_hot(uint id) const {
	return m_hot == id;
}
bool Ui::is_item_focused(uint id) const {
	return m_focus == id;
}
bool Ui::was_focused(uint id) const {
	return m_focus == id;
}
bool Ui::is_edit_buffer(uint id) const {
	return m_edit_buffer_id == id;
}
bool Ui::in_rect(int x, int y, int w, int h, bool checkScroll) const {
	return (!checkScroll || m_toolbar_root) && m_mx >= x && m_mx <= x + w && m_my >= y &&
		   m_my <= y + h;
}
void Ui::clear_input() {
	// trick to have double click flag in input

	// clear all bits except double click
	m_buttons = 0;

	m_left_pressed = false;
	m_left_released = false;
	m_double_left_released = false;
	m_scroll = 0;
	m_key = 0;
}
void Ui::clear_active() {
	m_active = 0;
	m_dragged_rollout_id = nullptr;
	set_cursor(CURSOR_DEFAULT);
	// mark all UI for this frame as processed
	clear_input();
}
void Ui::set_active(uint id) {
	m_active = id;
	m_went_active = true;
}
void Ui::set_hot(uint id) {
	if (m_hot != id)
		play_sound(SOUND_MOUSE_HOVER);

	m_hot_to_be = id;
}
void Ui::play_sound(SOUNDS s) {
	// nothing yet
}
void Ui::set_focused(uint id) {
	m_focus = id;
}
void Ui::set_edit_buffer_id(uint id) {
	m_edit_buffer_id = id;
}
bool Ui::button_logic(uint id, bool over) {
	if (m_options & INPUT_DISABLED)
		return false;

	bool was_focused = false;
	if (m_left_pressed && is_item_focused(id)) {
		set_focused(0);
		was_focused = true;
	}

	bool res = false;
	// process down
	if (!any_active()) {
		if (over)
			set_hot(id);
		if (is_item_hot(id) && m_left_pressed) {
			set_active(id);
			if (!was_focused)
				set_focused(id);
		}
	}
	// if button is active, then react on left up
	if (is_item_active(id)) {
		m_is_active = true;
		if (over)
			set_hot(id);
		if (m_left_released) {
			if (is_item_hot(id)) {
				res = true;
			}
			clear_active();
			play_sound(SOUND_CLICK);
		}
	}
	if (is_item_focused(id) && m_key == TAB_KEY) {
		// TAB pressed -> change focus to next id
		// focus will be automatically moved to next edit control (look edit_logic)
		m_key = 0; // clear tab m_key
		m_search_next_focus = true;
		set_focused(0);
	}
	if (is_item_hot(id))
		m_is_hot = true;
	return res;
}
bool Ui::edit_logic(uint id, bool wasFocus, bool enabled, char* text, int buffer_len,
					bool* edit_finished, char key) {
	if (edit_finished)
		*edit_finished = false;

	if (m_options & INPUT_DISABLED)
		return false;

	if (m_search_next_focus && enabled && !wasFocus) {
		// tab pressed -> set focus to current edit
		m_search_next_focus = false;
		set_focused(id);
	}
	if (!is_item_focused(id) && is_edit_buffer(id)) {
		// control have lost focus -> copy internal buffer into text
		strncpy(text, m_edit_buffer, buffer_len);
		m_edit_buffer[0] = 0;
		set_edit_buffer_id(0);

		// editing is complete
		if (edit_finished)
			*edit_finished = true;
		return true;
	}
	if (is_item_focused(id)) {
		// we can use edit buffer
		if (m_edit_buffer_id == 0) {
			// control received focus -> copy text into internal buffer
			// strncpy(m_edit_buffer, text, sizeof(m_edit_buffer) / sizeof(m_edit_buffer[0]));
			m_edit_buffer_id = id;
			m_edit_buffer[0] = 0;
		}
		else {
			// we still can not edit
			if (!is_edit_buffer(id))
				return false;

			strncpy(text, m_edit_buffer, buffer_len);
		}
	}
	else
		return false;

	if (!buffer_len)
		return false;

	if (!key)
		return false;

	size_t len = strlen(m_edit_buffer);
	if (key == RETURN_KEY) {
		// return key - remove last character
		if (len > 0)
			m_edit_buffer[len - 1] = 0;

		strncpy(text, m_edit_buffer, buffer_len);
		return false;
	}
	if (key == ENTER_KEY) {
		if (edit_finished) {
			*edit_finished = is_item_focused(id);
			LOGI("on enter pressed, id: %d, edit_finished: %d", id, *edit_finished);
		}
		return true;
	}

	if ((int)len < buffer_len - 1) {
		m_edit_buffer[len] = key;
		m_edit_buffer[len + 1] = 0;
		strncpy(text, m_edit_buffer, buffer_len);

		// value is changed
		return true;
	}
	return false;
}
bool Ui::combo_button_logic(uint id, bool over) {
	if (m_options & INPUT_DISABLED)
		return false;

	bool res = false;
	// process down
	if (!any_active()) {
		if (over)
			set_hot(id);
		if (is_item_hot(id) && m_left_pressed) {
			set_active(id);
		}
	}
	// if button is active, then react on left up
	if (is_item_active(id)) {
		m_is_active = true;
		if (over)
			set_hot(id);
		if (m_left_released) {
			if (is_item_hot(id)) {
				res = true;
				set_focused(id);
			}
			clear_active();
			play_sound(SOUND_CLICK);
		}
	}
	if (is_item_focused(id) && m_key == TAB_KEY) {
		// TAB pressed -> change focus to next id
		// focus will be automatically moved to next edit control (look edit_logic)
		m_key = 0; // clear tab m_key
		m_search_next_focus = true;
		set_focused(0);
	}
	if (is_item_hot(id))
		m_is_hot = true;
	return res;
}

void Ui::update_input(int mx, int my, unsigned char mbut, int scroll, char key) {
	bool left = false;
	if (mbut & MBUT_LEFT)
		left = true;

	bool double_left = false;
	if (mbut & MBUT_LEFT_DBL)
		double_left = true;

	m_buttons = mbut;
	m_mx = mx;
	m_my = my;
	m_left_pressed = !m_left && left;
	m_left_released = m_left && !left;
	m_double_left_released = m_double_left && !double_left;
	m_left = left;
	m_double_left = double_left;

	m_key = key;
	m_scroll = scroll;

	if (m_left_pressed)
		m_platform.capture_mouse(true);
	else if (m_left)
		m_platform.capture_mouse(false);

}

void Ui::begin_frame(uint width, uint height, int mx, int my, unsigned char mbut, int scroll,
					 char key) {
	update_input(mx, my, mbut, scroll, key);

	m_width = width;
	m_height = height;

	m_button_height = g_button_height;

	m_hot = m_hot_to_be;
	m_hot_to_be = 0;

	// m_render_options = 0;

	m_went_active = false;
	m_is_active = false;
	m_is_hot = false;
	m_search_next_focus = false;

	m_widget_x = 0;
	m_widget_y = 0;
	m_widget_w = 0;
	m_rollout_width = 0;
	m_rollout_left = 0;
	m_options = 0;

	m_area_id = 1;
	m_widget_id = 0;
	m_row = 0;
	m_alpha = 255;
	m_cursor_over_drag = false;

	// recalculate rollouts based on size of frame
	visit_rollout_node(m_rollouts, m_toolbar_root, 0, 0, width, height);

	// if we have dragged rollout, setup depth testing
	set_depth(0);
}
void Ui::end_frame() {
	if (m_dragged_rollout_id) {
		// user is dragging rollout, display move symbol
		size_t size = m_rollouts.size();
		for (size_t i = 0; i < size; ++i) {
			Rollout* r = m_rollouts[i];
			if (r == m_dragged_rollout_id || !(r->options & ROLLOUT_ATTACHED))
				continue;
			if (in_rect(r->x, r->y, r->w, r->h, false)) {
				m_target_side = rollout_move(m_dragged_rollout_id, r, r->x + r->w / 2, r->y + r->h / 2);
				if (m_target_side) {
					// user selected area to insert rollout
					m_target_rollout = r;
				}
			}
		}
	}
	if (!m_cursor_over_drag && !m_left)
		set_cursor(CURSOR_DEFAULT); // restore to default cursor only of left button is not pressed

	clear_input();
	m_rqueue.on_frame_finished();
}
void Ui::cleanup() {
	m_rollouts.clear();
	clear_toolbars(m_toolbar_root);
	m_rollout_last = m_toolbar_root;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// static const int m_button_height = 20;

static const int SLIDER_HEIGHT = 20;
static const int SLIDER_MARKER_WIDTH = 10;
static const int CHECK_SIZE = 8;
static const int DEFAULT_SPACING = 4;
static const int TEXT_HEIGHT = 8;
static const int SCROLL_AREA_PADDING = 6;
static const int INTEND_SIZE = 16;
static const int AREA_HEADER = 28;
static const int TOOLBAR_HEADER = 4;
static const int DEFAULT_ROLLOUT_DEPTH = 0;
static const int FOCUSED_ROLLOUT_DEPTH = 2;
static const int FLOATING_ROLLOUT_DEPTH = 1;
static const int ATTACHED_ROLLOUT_DEPTH = 0;
static const int MOVE_SIGN_DEPTH = 3; // should be less then MAX_UI_LAYER_COUNT (10)

static const char* cursorMap[] = {"default", "resize_horz", "resize_vert", "resize_corner"};

bool Ui::begin_rollout(Rollout* pr) {
	if (!pr)
		return false;
	Rollout& r = *pr;
	int		 area_header = AREA_HEADER;

	bool toolbar = (r.options & TOOLBAR) != 0;
	bool rollout_attached = (r.options & ROLLOUT_ATTACHED) != 0;

	int alpha_speed = 0; // static_cast<int>(timer::tick() * 255.0f * 3.0f);
	if (alpha_speed > 255)
		alpha_speed = 45;
	r.alpha += r.alpha_inc * alpha_speed;
	if (r.alpha <= 0) {
		if (r.alpha_inc < 0)
			r.alpha_inc = 0; // stop decrementing alpha
		r.alpha = 0;
		detach_rollout(&r);
	}
	if (r.alpha > 255) {
		if (r.alpha_inc > 0)
			r.alpha_inc = 0; // stop decrementing alpha
		r.alpha = 255;
	}
	// if toolbar
	if (toolbar)
		area_header = TOOLBAR_HEADER;

	int h;
	if (r.minimized)
		h = area_header + r.scroll;
	else
		h = r.h;

	m_area_id++;
	m_widget_id = 0;
	m_alpha = r.alpha;
	m_scroll_id = (m_area_id << 16) | m_widget_id;
	m_options = r.options;

	bool draw_scroll = true;
	if (r.scroll == SCROLL_DISABLED) {
		r.scroll = 0;
		draw_scroll = false;
	}
	m_rollout_left = r.x + m_padding_left;
	m_rollout_top = r.y + m_padding_top;
	m_rollout_width = r.w - m_padding_left - m_padding_right;
	if (draw_scroll)
		m_rollout_width = r.w - SCROLL_AREA_PADDING * 2 - m_padding_left - m_padding_right;
	m_rollout_height = h - m_padding_bottom - m_padding_top;
	m_widget_x = m_rollout_left;
	m_widget_y = r.y + h - area_header + r.scroll;
	m_widget_w = m_rollout_width;
	// m_row = m_options & TOOLBAR ? 1 : 0;
	m_row = 0;

	int x = r.x, y = r.y, w = r.w;

	m_scroll_right = m_rollout_left + m_rollout_width;
	m_scroll_top = y - area_header + h;
	m_scroll_bottom = y + m_padding_bottom;
	m_scroll_val = &r.scroll;

	m_scroll_area_top = m_widget_y;

	m_focus_top = y - area_header;
	m_focus_bottom = y - area_header + h;

	m_inside_scroll_area = in_rect(x, y, w, h, false);
	m_inside_current_scroll = m_inside_scroll_area;

	bool ret_val = false;
	int  caption_y = y + h - area_header + m_button_height / 2 - 2;
	int  caption_height = m_button_height + 3;

	// setup rollout depth
	if (m_focused_rollout_id == pr)
		r.z = FOCUSED_ROLLOUT_DEPTH;
	else if (rollout_attached)
		r.z = ATTACHED_ROLLOUT_DEPTH;
	else
		r.z = FLOATING_ROLLOUT_DEPTH;

	if (!(r.options & ROLLOUT_HOLLOW)) {
		m_rqueue.add_depth(r.z);
		m_rqueue.add_rect(x, y, w, h, m_colors[ROLLOUT_COLOR]);
		if (m_left_pressed && in_rect(x, y, w, h)) {
			// click on rollout -> increase depth
			m_focused_rollout_id = pr;
		}

		bool	 cursor_over_drag;
		Rollout* draggedRolloutId = m_dragged_rollout_id;
		// resize area
		if (!(m_options & TOOLBAR) && m_options & RESIZE_AREA && !r.minimized) {
			// dragging area
			int drag_id = -1;
			int ydiff = 0, xdiff = 0;
			// if area is active, then react on left up
			if (draggedRolloutId && m_left_released) {
				Rollout* drollout = m_dragged_rollout_id;
				if (m_target_side == ROLLOUT_LEFT) {
					insert_rollout(drollout, 0.5f, true, m_target_rollout);
				}
				else if (m_target_side == ROLLOUT_TOP) {
					insert_rollout(drollout, -0.5f, false, m_target_rollout);
				}
				else if (m_target_side == ROLLOUT_RIGHT) {
					insert_rollout(drollout, -0.5f, true, m_target_rollout);
				}
				else if (m_target_side == ROLLOUT_BOTTOM) {
					insert_rollout(drollout, 0.5f, false, m_target_rollout);
				}
				else if (m_target_side == ROLLOUT_CENTER) {
					insert_rollout(drollout, 0.0f, false, m_target_rollout);
				}
				if (m_target_side == ROLLOUT_LEFT_FIXED) {
					int neww = drollout->w > m_target_rollout->w / 2 ? m_target_rollout->w / 2
																	 : drollout->w;
					insert_rollout(drollout, (float)neww, true, m_target_rollout);
				}
				else if (m_target_side == ROLLOUT_TOP_FIXED) {
					int newh = drollout->h > m_target_rollout->h / 2 ? m_target_rollout->h / 2
																	 : drollout->h;
					insert_rollout(drollout, -(float)newh, false, m_target_rollout);
				}
				else if (m_target_side == ROLLOUT_RIGHT_FIXED) {
					int neww = drollout->w > m_target_rollout->w / 2 ? m_target_rollout->w / 2
																	 : drollout->w;
					insert_rollout(drollout, -(float)neww, true, m_target_rollout);
				}
				else if (m_target_side == ROLLOUT_BOTTOM_FIXED) {
					int newh = drollout->h > m_target_rollout->h / 2 ? m_target_rollout->h / 2
																	 : drollout->h;
					insert_rollout(drollout, (float)newh, false, m_target_rollout);
				}
				clear_active(); // clear state to prevent
			}
			if (m_options & DRAG_AREA && !r.minimized) {
				if (r.tabs.empty()) {
					if (system_drag(x, caption_y - caption_height / 2 + 5, w, caption_height, xdiff,
									ydiff, cursor_over_drag)) {
						r.x += xdiff;
						r.y += ydiff;
						if (m_left_pressed) {
							m_target_side = ROLLOUT_UNDEFINED;
							m_target_rollout = nullptr;
							m_dragged_rollout_id = pr;
							detach_rollout(&r); // if rollout attached, detach it
						}
					}
				}
			}
			cursor_over_drag = false;
			CURSOR cursor = CURSOR_DEFAULT;
			if (system_drag(x, y, SCROLL_AREA_PADDING * 2, h, xdiff, ydiff, cursor_over_drag)) {
				if (!rollout_attached) {
					r.x += xdiff;
					r.w -= xdiff;
				}
				else if (m_rollout_drag_div.div) {
					m_rollout_drag_div.shift(xdiff);
				}
			}
			if (cursor_over_drag) {
				if (!rollout_attached ||
					find_div(m_mx, m_my, m_toolbar_root, 0, 0, m_width, m_height).div) {
					cursor = CURSOR_RESIZE_HORZ;
					m_cursor_over_drag = true;
				}
			}
			if (system_drag(x + w - SCROLL_AREA_PADDING, y, SCROLL_AREA_PADDING * 2, h, xdiff,
							ydiff, cursor_over_drag)) {
				if (!rollout_attached) {
					r.w += xdiff;
				}
				else if (m_rollout_drag_div.div) {
					m_rollout_drag_div.shift(xdiff);
				}
			}
			if (cursor_over_drag) {
				if (!rollout_attached ||
					find_div(m_mx, m_my, m_toolbar_root, 0, 0, m_width, m_height).div) {
					cursor = CURSOR_RESIZE_HORZ;
					m_cursor_over_drag = true;
				}
			}
			if (system_drag(x, y + h - SCROLL_AREA_PADDING * 2 + 5, w, SCROLL_AREA_PADDING * 2 - 5,
							xdiff, ydiff, cursor_over_drag)) {
				if (!rollout_attached)
					r.h += ydiff;
				else if (m_rollout_drag_div.div)
					m_rollout_drag_div.shift(ydiff);
			}
			if (cursor_over_drag) {
				if (!rollout_attached ||
					find_div(m_mx, m_my, m_toolbar_root, 0, 0, m_width, m_height).div) {
					cursor = CURSOR_RESIZE_VERT;
					m_cursor_over_drag = true;
				}
			}
			if (system_drag(x, y - SCROLL_AREA_PADDING, w, SCROLL_AREA_PADDING * 2, xdiff, ydiff,
							cursor_over_drag)) {
				if (!rollout_attached) {
					r.y += ydiff;
					r.h -= ydiff;
				}
				else if (m_rollout_drag_div.div)
					m_rollout_drag_div.shift(ydiff);
			}
			if (cursor_over_drag) {
				if (!rollout_attached ||
					find_div(m_mx, m_my, m_toolbar_root, 0, 0, m_width, m_height).div) {
					cursor = CURSOR_RESIZE_VERT;
					m_cursor_over_drag = true;
				}
			}
			int padd = SCROLL_AREA_PADDING * 2;
			if (system_drag(x + w - padd, y - padd,
						padd * 2, padd * 2, xdiff,
							ydiff, cursor_over_drag)) {
				if (!rollout_attached) {
					r.w += xdiff;
					r.y += ydiff;
					r.h -= ydiff;
				}
			}
			if (cursor_over_drag) {
				if (!rollout_attached) {
					cursor = CURSOR_RESIZE_CORNER;
					m_cursor_over_drag = true;
				}
			}
			if (!m_left && cursor != CURSOR_DEFAULT)
				set_cursor(cursor);
		}
		// caption and close buttons
		int xdiff, ydiff;
		if (!toolbar) {
			if (!r.tabs.empty()) {
				// several rollouts in tabbed interface
				int width = m_widget_w / r.tabs.size();
				int xx = m_widget_x;
				for (int rollout_id : r.tabs) {
					Rollout* tr = m_rollouts[rollout_id];
					if (system_tab(tr->name.c_str(), xx, caption_y, width - 2, caption_height,
								   tr == &r, xdiff, ydiff)) {
						if (tr != &r) {
							for (int r_id : r.tabs)
								m_rollouts[r_id]->alpha = 0;
							tr->alpha = 255;
						}
					}
					if (xdiff || ydiff) {
						detach_rollout(tr);
						break;
					}
					// close button
					if (tr == &r || in_rect(xx, caption_y, width - 2, caption_height)) {
						int bx = xx + width - int(m_button_height * 1.5f); // button position
						if (system_button("x", bx, y + h - area_header + m_button_height / 2,
										  (int)(m_button_height * 1.2f), m_button_height, true)) {
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
			else {
				m_rqueue.add_rect(x, caption_y, w, caption_height, m_colors[ROLLOUT_CAPTION_COLOR]);
				m_rqueue.add_text(x, y + h - area_header + m_button_height / 2 - 2, m_widget_w,
								  m_button_height, ALIGN_LEFT, r.name.c_str(),
								  RGBA(255, 255, 255, 128));
				// close button
				if (system_button("x", x + w - (int)(m_button_height * 2.0f),
								  y + h - area_header + m_button_height / 2, m_button_height,
								  m_button_height, true)) {
					hide_rollout(pr);
					ret_val = true;
				}
			}
		}
	}
	// no clipping if dragging item is possible
	// if ( !(m_options & DRAG_ITEM) )
	m_rqueue.add_scissor(m_rollout_left, m_scroll_bottom, m_scroll_right - m_rollout_left,
						 m_scroll_top - m_scroll_bottom);

	// dragging control
	m_widget_id = 10;
	// return m_inside_scroll_area;
	return ret_val;
}
void Ui::end_rollout() {
	// Disable scissoring.
	m_rqueue.add_scissor(-1, -1, -1, -1);
	if (!(m_options & NO_SCROLL)) {
		// Draw scroll bar
		int x = m_scroll_right + SCROLL_AREA_PADDING / 2;
		int y = m_scroll_bottom;
		int w = SCROLL_AREA_PADDING * 2;
		int h = m_scroll_top - m_scroll_bottom;
		if (h > 0) {
			int   stop = m_scroll_area_top;
			int   sbot = m_widget_y;
			int   sh = stop - sbot; // The scrollable area height.
			float barHeight = (float)h / (float)sh;
			if (barHeight < 1) {
				float barY = (float)(y - sbot) / (float)sh;
				if (barY < 0)
					barY = 0;
				if (barY > 1)
					barY = 1;

				// Handle scroll bar logic.
				uint hid = m_scroll_id;
				int					   hx = x;
				int					   hy = y + (int)(barY * h);
				int					   hw = w;
				int					   hh = (int)(barHeight * h);

				const int range = h - (hh - 1);
				bool	  over = in_rect(hx, hy, hw, hh);
				button_logic(hid, over);
				if (is_item_active(hid)) {
					float u = (float)(hy - y) / (float)range;
					if (m_went_active) {
						m_drag_y = m_my;
						m_drag_orig = u;
					}
					if (m_drag_y != m_my) {
						u = m_drag_orig + (m_my - m_drag_y) / (float)range;
						if (u < 0)
							u = 0;
						if (u > 1)
							u = 1;
						*m_scroll_val = (int)((1 - u) * (sh - h));
					}
				}
				// BG
				m_rqueue.add_rect(x, y, w, h, RGBA(0, 0, 0, 196));
				// Bar
				hh = std::max(hh, 10);
				if (is_item_active(hid))
					m_rqueue.add_rect(hx, hy, hw, hh, RGBA(255, 196, 0, 196));
				else
					m_rqueue.add_rect(hx, hy, hw, hh, is_item_hot(hid) ? RGBA(255, 196, 0, 96)
																	   : RGBA(255, 255, 255, 64));

				// Handle mouse scrolling.
				if (m_inside_scroll_area) { // && !any_active())
					if (m_scroll) {
						*m_scroll_val += 20 * m_scroll;
						if (*m_scroll_val < 0)
							*m_scroll_val = 0;
						if (*m_scroll_val > (sh - h))
							*m_scroll_val = (sh - h);
					}
				}
			}
			else
				*m_scroll_val = SCROLL_DISABLED; // memorize state - no scroll
		}
	}
	m_inside_current_scroll = false;
	m_widget_id = 0;
	m_row = 0;
}

bool Ui::start_control(bool enabled, int& x, int& y, int& w, int& h, uint& id, bool& over,
					   bool& wasFocus) {
	m_widget_id++;
	id = (m_area_id << 16) | m_widget_id;

	x = m_widget_x;
	y = m_widget_y - m_button_height;
	w = m_widget_w;
	h = m_button_height;
	if (!m_row)
		m_widget_y -= m_button_height + DEFAULT_SPACING;
	else
		m_widget_x += w + DEFAULT_SPACING;
	if (m_widget_id > 10 && (y + h < m_scroll_bottom || y > m_scroll_top))
		return false;

	over = enabled && in_rect(x, y, w, h);
	wasFocus = is_item_focused(id);
	return true;
}
color Ui::text_color_hot(uint id, bool enabled, bool focused) {
	if (enabled)
		return (is_item_hot(id) || focused) ? m_colors[TEXT_COLOR_HOT]
														: m_colors[TEXT_COLOR];
	return m_colors[TEXT_COLOR_DISABLED];
}
color Ui::text_color(uint id, bool enabled) {
	return enabled ? m_colors[TEXT_COLOR] : m_colors[TEXT_COLOR_DISABLED];
}
color Ui::button_color(uint id, bool enabled) {
//	if (!is_item_focused(id))
		return is_item_active(id) ? m_colors[BUTTON_COLOR_ACTIVE] : m_colors[BUTTON_COLOR];

//	return RGBA(m_colors[BUTTON_COLOR_ACTIVE], 150);
}
color Ui::edit_color(uint id, bool enabled) {
	if (!is_item_focused(id))
		return is_item_active(id) ? m_colors[EDIT_COLOR_ACTIVE] : m_colors[EDIT_COLOR];

	return RGBA(m_colors[EDIT_COLOR_ACTIVE], 150);
}
bool Ui::button(const char* text, bool enabled) {
	int  x, y, w, h;
	uint id;
	bool over, wasFocus;
	if (!start_control(enabled, x, y, w, h, id, over, wasFocus))
		return false;

	bool res = button_logic(id, over);
	m_rqueue.add_rounded_rect(x, y, w, h, DEF_ROUND, button_color(id));
	m_rqueue.add_text(x, y, m_widget_w, m_button_height, m_text_align, text,
					  text_color_hot(id, enabled));
	return res;
}
bool Ui::button(const char* text, int x, int y, int w, int h, bool enabled) {
	m_widget_id++;
	uint id = (m_area_id << 16) | m_widget_id;

	x += m_rollout_left;
	y += m_scroll_area_top;

	bool over = enabled && in_rect(x, y, w, h);
	bool res = button_logic(id, over);

	m_rqueue.add_rounded_rect(x, y, w, h, DEF_ROUND, button_color(id));
	m_rqueue.add_text(x, y, m_widget_w, m_button_height, m_text_align, text,
					  text_color(id, enabled));
	return res;
}
bool Ui::system_button(const char* text, int x, int y, int w, int h, bool enabled) {
	m_widget_id++;
	uint id = (m_area_id << 16) | m_widget_id;
	bool over = enabled && in_rect(x, y, w, h);
	bool res = button_logic(id, over);
	m_rqueue.add_text(x, y, w, m_button_height, m_text_align, text, text_color(id, enabled));
	return res;
}
bool Ui::system_tab(const char* text, int x, int y, int w, int h, bool checked, int& xmove,
					int& ymove) {
	xmove = ymove = 0;
	m_widget_id++;
	uint id = (m_area_id << 16) | m_widget_id;

	bool over = in_rect(x, y, w, h);
	bool res = button_logic(id, over);

	if (over && m_left_pressed) {
		m_drag_x = m_mx;
		m_drag_y = m_my;
		m_rollout_drag_div = find_div(m_mx, m_my, m_toolbar_root, 0, 0, m_width, m_height);
	}
	if (is_item_active(id)) {
		xmove = m_mx - m_drag_x;
		ymove = m_my - m_drag_y;
		m_drag_x = m_mx;
		m_drag_y = m_my;
	}
	if (is_item_hot(id) || checked)
		m_rqueue.add_rect(x, y, w, h, m_colors[ROLLOUT_CAPTION_COLOR]);

	m_rqueue.add_text(x, y, w, m_button_height, m_text_align, text, text_color_hot(id, checked));
	return res;
}
bool Ui::system_drag(int x, int y, int w, int h, int& xdiff, int& ydiff, bool& over) {
	// first widget id after area scroll - area drag control (caption)
	m_widget_id++;
	int id = (m_area_id << 16) | m_widget_id;

	over = in_rect(x, y, w, h, false);
	if (over && m_left_pressed) {
		m_drag_x = m_mx;
		m_drag_y = m_my;
		m_rollout_drag_div = find_div(m_mx, m_my, m_toolbar_root, 0, 0, m_width, m_height);
	}
	bool res = button_logic(id, over);
	if (is_item_active(id)) {
		m_rqueue.add_rect(x, y, w, h, m_colors[DRAG_COLOR]);
		xdiff = m_mx - m_drag_x;
		ydiff = m_my - m_drag_y;
		m_drag_x = m_mx;
		m_drag_y = m_my;
	}
	return is_item_active(id);
}
bool Ui::item(const char* text, bool selected, bool enabled) {
	int  x, y, w, h;
	uint id;
	bool over, wasFocus;
	if (!start_control(enabled, x, y, w, h, id, over, wasFocus))
		return false;

	bool res = button_logic(id, over);
	if (is_item_active(id) && !m_drag_item[0] && !over && (m_options & DRAG_ITEM)) {
		// start drag
		strncpy(m_drag_item, text, sizeof(m_drag_item) / sizeof(m_drag_item[0]));
		m_drag_item_width = w;
	}
	if (is_item_hot(id))
		m_rqueue.add_rect(x, y, w, h, RGBA(255, 196, 0, is_item_active(id) ? 196 : 96));
	else if (selected)
		m_rqueue.add_rect(x, y, w, h, RGBA(255, 196, 0, 70));

	m_rqueue.add_text(x, y, m_widget_w, m_button_height, m_text_align, text,
					  text_color(id, enabled));
	return res;
}
bool Ui::combo_item(const char* text, bool enabled) {
	int  x, y, w, h;
	uint id;
	bool over, wasFocus;
	if (!start_control(enabled, x, y, w, h, id, over, wasFocus))
		return false;

	bool res = combo_button_logic(id, over);
	if (res) {
		// combo item clicked - clear active id,
		// to do not affect another controls
		set_focused(0);
	}
	if (is_item_hot(id))
		m_rqueue.add_rect(x + w / 2, y, w / 2, h, RGBA(255, 196, 0, is_item_active(id) ? 196 : 96));

	m_rqueue.add_text(x + w / 2, y, m_widget_w, m_button_height, m_text_align, text,
					  text_color(id, enabled));
	return res;
}
// same as item, but displays only last part of filepath
bool Ui::file_item(const char* text, char slash, bool selected, bool enabled) {
	int  x, y, w, h;
	uint id;
	bool over, wasFocus;
	if (!start_control(enabled, x, y, w, h, id, over, wasFocus))
		return false;

	bool res = button_logic(id, over);

	if (is_item_active(id) && !m_drag_item[0] && !over && (m_options & DRAG_ITEM)) {
		// start drag
		strncpy(m_drag_item, text, sizeof(m_drag_item) / sizeof(m_drag_item[0]));
		m_drag_item_width = w;
	}

	if (is_item_hot(id))
		m_rqueue.add_rect(x, y, w, h, RGBA(255, 196, 0, is_item_active(id) ? 196 : 96));
	else if (selected)
		m_rqueue.add_rect(x, y, w, h, RGBA(255, 196, 0, 70));

	// find slash symbol
	const char* filename = text;
	int			len = strlen(text) - 1;
	for (; len >= 0; --len)
		if (text[len] == slash) {
			filename = &text[len + 1];
			break;
		}
	m_rqueue.add_text(x, y, m_widget_w, m_button_height, m_text_align, filename,
					  text_color(id, enabled));
	return res;
}

bool Ui::item_dropped(char* text, uint buffer_len, int& mouse_x, int& mouse_y) {
	if (!text || !buffer_len) {
		assert(false);
		return false;
	}

	if (!m_drag_item[0]) {
		text[0] = 0;
		return false;
	}

	// report some information about dragging
	strncpy(text, m_drag_item, buffer_len);
	mouse_x = m_mx;
	mouse_y = m_my;

	if (m_drag_item[0] && !m_left) {
		// stop drag process
		m_drag_item[0] = 0;
		return true;
	}

	int x = m_mx;
	int y = m_my;
	int w = m_drag_item_width;
	int h = m_button_height;

	// render item

	// switch off clipping optimization
	// TODO: hack, find another way
	uint id = m_widget_id;
	m_widget_id = 0;

	m_rqueue.add_rect(x, y, w, h, RGBA(255, 196, 0, 96));
	m_rqueue.add_scissor(x, y, w - m_button_height, h);
	m_rqueue.add_text(x + w - m_button_height / 2, y, m_widget_w, m_button_height, ALIGN_RIGHT,
					  m_drag_item, m_colors[TEXT_COLOR]);
	m_rqueue.add_scissor(-1, -1, -1, -1); // disable scissor

	// restore
	m_widget_id = id;
	return false;
}

bool Ui::check(const char* text, bool checked, bool enabled) {
	int  x, y, w, h;
	uint id;
	bool over, wasFocus;
	if (!start_control(enabled, x, y, w, h, id, over, wasFocus))
		return false;

	bool res = button_logic(id, over);

	const int cx = x;
	const int cy = y + CHECK_SIZE / 2;
	m_rqueue.add_rounded_rect(x + CHECK_SIZE - 3, cy - 3, CHECK_SIZE + 6, CHECK_SIZE + 6, DEF_ROUND,
							  button_color(id, enabled));

	if (checked) {
		uint check_clr;
		if (enabled)
			check_clr = RGBA(255, 255, 255, is_item_active(id) ? 255 : 200);
		else
			check_clr = m_colors[TEXT_COLOR_DISABLED];

		m_rqueue.add_rounded_rect(x + CHECK_SIZE, cy, CHECK_SIZE, CHECK_SIZE, DEF_ROUND, check_clr);
	}
	m_rqueue.add_text(x + m_button_height, y, m_widget_w, m_button_height, m_text_align, text,
					  text_color_hot(id, enabled));
	return res;
}
bool Ui::button_check(const char* text, bool checked, bool enabled) {
	int  x, y, w, h;
	uint id;
	bool over, wasFocus;
	if (!start_control(enabled, x, y, w, h, id, over, wasFocus))
		return false;

	bool res = button_logic(id, over);

	// check behavior, but button look
	m_rqueue.add_rounded_rect(x, y, w, h, DEF_ROUND, is_item_active(id) || checked
														 ? m_colors[BUTTON_COLOR_ACTIVE]
														 : m_colors[BUTTON_COLOR]);
	m_rqueue.add_text(x, y, m_widget_w, m_button_height, m_text_align, text,
					  text_color_hot(id, enabled));
	return res;
}
void Ui::row(uint count) {
	// set new widget width
	int remainder = m_row ? (m_rollout_width - m_widget_w) : m_rollout_width;
	m_widget_w = (remainder + DEFAULT_SPACING) / count - DEFAULT_SPACING;
	m_row++;
}
void Ui::set_widget_width(int width) {
	m_widget_w = width;
}
void Ui::end_row() {
	// restore widget width
	if (m_row == 1) {
		m_widget_w = m_rollout_width;
		m_widget_x = m_rollout_left;
		m_widget_y -= m_button_height + DEFAULT_SPACING;
	}
	m_row--;
}
bool Ui::collapse(const char* text, bool checked, bool enabled) {
	int  x, y, w, h;
	uint id;
	bool over, wasFocus;
	if (!start_control(enabled, x, y, w, h, id, over, wasFocus))
		return false;

	const int cx = x + CHECK_SIZE / 2;
	const int cy = y + m_button_height / 4;
	bool	  res = button_logic(id, over);
	//m_rqueue.add_rect(x, y, w, h, is_item_active(id) ? m_colors[COLLAPSE_COLOR_ACTIVE]
	//												 : m_colors[COLLAPSE_COLOR]);
	unsigned char clr = enabled ? clr = 255 : clr = 128;
	;
	if (checked)
		m_rqueue.add_triangle(cx, cy, CHECK_SIZE, CHECK_SIZE, 1,
							  RGBA(clr, clr, clr, is_item_active(id) ? 255 : 200));
	else
		m_rqueue.add_triangle(cx, cy, CHECK_SIZE, CHECK_SIZE, 2,
							  RGBA(clr, clr, clr, is_item_active(id) ? 255 : 200));

	m_rqueue.add_text(x + CHECK_SIZE, y, w, h, m_text_align, text, text_color_hot(id, enabled));
	return res;
}
bool Ui::combo(const char* text, const char* value, bool enabled) {
	int  x, y, w, h;
	uint id;
	bool over, wasFocus;
	if (!start_control(enabled, x, y, w, h, id, over, wasFocus))
		return false;

	const int cx = x + CHECK_SIZE / 2;
	const int cy = y + m_button_height / 4;
	bool	  res = combo_button_logic(id, over);
	bool	  focused = is_item_focused(id);
	if (res && wasFocus)
		set_focused(0); // clear focus, of combo was clicked second time

	unsigned char clr = enabled ? clr = 255 : clr = 128;
	if (!focused)
		m_rqueue.add_triangle(cx, cy, CHECK_SIZE, CHECK_SIZE, 1,
							  RGBA(clr, clr, clr, is_item_active(id) ? 255 : 200));
	else
		m_rqueue.add_triangle(cx, cy, CHECK_SIZE, CHECK_SIZE, 2,
							  RGBA(clr, clr, clr, is_item_active(id) ? 255 : 200));

	m_rqueue.add_rounded_rect(x + w / 2, y, w / 2, h, DEF_ROUND, button_color(id, enabled));
	m_rqueue.add_text(x + CHECK_SIZE, y, m_widget_w, m_button_height, m_text_align, text,
					  text_color(id, true));
	m_rqueue.add_text(x + w / 2, y, m_widget_w, m_button_height, m_text_align, value,
					  text_color_hot(id, enabled, focused));
	return wasFocus;
}
bool Ui::button_collapse(const char* text, bool enabled) {
	int  x, y, w, h;
	uint id;
	bool over, wasFocus;
	if (!start_control(enabled, x, y, w, h, id, over, wasFocus))
		return false;

	bool	  res = button_logic(id, over);
	const int cx = x + m_button_height / 2;
	const int cy = y + m_button_height / 3;

	if (wasFocus && res)
		set_focused(0); // close combo

	bool		  focused = is_item_focused(id);
	unsigned char clr;
	clr = enabled ? 255 : 128;
	m_rqueue.add_rounded_rect(x, y, w, h, DEF_ROUND, button_color(id, enabled));
	m_rqueue.add_text(x + m_button_height, y, m_widget_w, m_button_height, m_text_align, text,
					  text_color_hot(id, enabled));
	if (!focused)
		m_rqueue.add_triangle(cx, cy, m_button_height / 2, m_button_height / 2, 1,
							  RGBA(clr, clr, clr, is_item_active(id) ? 255 : 200));
	else
		m_rqueue.add_triangle(cx, cy, m_button_height / 2, m_button_height / 2, 2,
							  RGBA(clr, clr, clr, is_item_active(id) ? 255 : 200));
	return focused;
}

bool Ui::file(const char* text, const char* value, bool enabled) {
	int  x, y, w, h;
	uint id;
	bool over, wasFocus;
	if (!start_control(enabled, x, y, w, h, id, over, wasFocus))
		return false;

	bool res = button_logic(id, over);

	bool	  focused = is_item_focused(id);
	const int cx = x - CHECK_SIZE / 2;
	const int cy = y - CHECK_SIZE / 2;

	m_rqueue.add_text(x, y, m_widget_w, m_button_height, m_text_align, text,
					  text_color_hot(id, enabled));

	// file value
	m_rqueue.add_rounded_rect(x + w / 2, y, w / 2, h, DEF_ROUND,
							  button_color(id, enabled));
	m_rqueue.add_text(x + w / 2, y, m_widget_w, m_button_height, m_text_align, value,
					  text_color_hot(id, enabled, focused));

	// clear focus
	if (focused)
		set_focused(0);

	return focused;
}

bool Ui::color_edit(const char* text, color clr, bool enabled, bool is_property) {
	int  x, y, w, h;
	uint id;
	bool over, wasFocus;
	if (!start_control(enabled, x, y, w, h, id, over, wasFocus))
		return false;

	bool res = button_logic(id, over);
	bool focused = is_item_focused(id);

	int twidth = 0;
	int vwidth = w;
	if (is_property) {
		twidth = (int)(m_widget_w * m_property_width);
		vwidth = (int)(w * (1.0f - m_property_width));
		m_rqueue.add_text(x, y, twidth, m_button_height, m_text_align, text,
						  text_color_hot(id, true));
	}
	const uint red = clr & 0xff;
	const uint green = (clr >> 8) & 0xff;
	const uint blue = (clr >> 16) & 0xff;

	m_rqueue.add_rounded_rect(
		x + twidth, y, vwidth, h, DEF_ROUND,
		RGBA(red, green, blue, is_item_hot(id) || is_item_active(id) ? 180 : 255));

	// clear focus
	if (focused)
		set_focused(0);

	return focused;
}

void Ui::rectangle(int x, int y, int width, int height, uint color) {
	if (m_widget_id > 10 && (y + height < m_scroll_bottom || y > m_scroll_top))
		return;
	m_rqueue.add_rect(x, y, width, height, color);
}

void Ui::triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint color) {
	// m_rqueue.add_triangle(x, y, width, height, color);
}
void Ui::label(const char* text) {
	m_widget_id++;
	int x = m_widget_x;
	int y = m_widget_y - m_button_height;
	if (!m_row)
		m_widget_y -= m_button_height;
	else
		m_widget_x += m_widget_w + DEFAULT_SPACING;
	if (m_widget_id > 10 && (y < m_scroll_bottom || y > m_scroll_top))
		return;
	// m_rqueue.add_text(x, y, m_text_align, text, RGBA(255,255,255,255));
	m_rqueue.add_text(x, y, m_widget_w, m_button_height, m_text_align, text, m_colors[TEXT_COLOR]);
}
bool Ui::edit(char* text, int buffer_len, bool* edit_finished, bool enabled) {
	int  x, y, w, h;
	uint id;
	bool over, wasFocus;
	if (!start_control(enabled, x, y, w, h, id, over, wasFocus))
		return false;

	button_logic(id, over);
	bool res = edit_logic(id, wasFocus, enabled, text, buffer_len, edit_finished, m_key);
	m_rqueue.add_rounded_rect(x, y, w, h, DEF_ROUND, edit_color(id, enabled));
	m_rqueue.add_text(x, y, m_widget_w, m_button_height, m_text_align, text,
					  text_color_hot(id, enabled));
	return res;
}
bool Ui::property(const char* name, char* text, int buffer_len, bool* edit_finished, bool enabled) {
	int  x, y, w, h;
	uint id;
	bool over, wasFocus;
	if (!start_control(enabled, x, y, w, h, id, over, wasFocus))
		return false;

	button_logic(id, over);
	bool res = edit_logic(id, wasFocus, enabled, text, buffer_len, edit_finished, m_key);

	// property name
	int twidth = (int)(m_widget_w * m_property_width);
	m_rqueue.add_text(x, y, twidth, m_button_height, m_text_align, name,
					  text_color_hot(id, enabled));

	// property value
	int vwidth = (int)(w * (1.0f - m_property_width));
	m_rqueue.add_rounded_rect(x + twidth, y, vwidth, h, DEF_ROUND, edit_color(id, enabled));
	m_rqueue.add_text(x + twidth, y, vwidth, m_button_height, m_text_align, text,
					  text_color_hot(id, enabled));
	return res;
}
void Ui::value(const char* text) {
	const int x = m_widget_x;
	const int y = m_widget_y - m_button_height;
	const int w = m_widget_w;
	if (!m_row)
		m_widget_y -= m_button_height;
	else
		m_widget_x += w + DEFAULT_SPACING;
	if (m_widget_id > 10 && (y < m_scroll_bottom || y > m_scroll_top))
		return;
	m_rqueue.add_text(x + m_button_height / 2, y, m_widget_w, m_button_height, ALIGN_RIGHT,
					  text, m_colors[TEXT_COLOR]);
}
bool Ui::slider(const char* text, float* val, float vmin, float vmax, float vinc, bool* last_change,
				bool enabled) {
	int  x, y, w, h;
	uint id;
	bool over, wasFocus;
	if (!start_control(enabled, x, y, w, h, id, over, wasFocus))
		return false;

	m_rqueue.add_rounded_rect(x, y, w, h, DEF_ROUND, m_colors[SLIDER_BG]);
	const int range = w - SLIDER_MARKER_WIDTH;

	float u = (*val - vmin) / (vmax - vmin);
	if (u < 0)
		u = 0;
	if (u > 1)
		u = 1;
	int m = (int)(u * range);

	over = in_rect(x + m, y, SLIDER_MARKER_WIDTH, SLIDER_HEIGHT);
	bool was_focusedBefore = is_item_focused(id);
	bool wasActiveBefore = is_item_active(id);
	bool res = button_logic(id, over);
	bool was_focused = is_item_focused(id);
	bool isItemActive = is_item_active(id);
	bool valChanged = false;

	if (is_item_active(id)) {
		if (m_went_active) {
			m_drag_x = m_mx;
			m_drag_orig = u;
		}
		if (m_drag_x != m_mx) {
			u = m_drag_orig + (float)(m_mx - m_drag_x) / (float)range;
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
		m_rqueue.add_rounded_rect(x + m, y, SLIDER_MARKER_WIDTH, SLIDER_HEIGHT, DEF_ROUND,
								  RGBA(255, 255, 255, 255));
	else
		m_rqueue.add_rounded_rect(x + m, y, SLIDER_MARKER_WIDTH, SLIDER_HEIGHT, DEF_ROUND,
								  is_item_hot(id) ? RGBA(255, 196, 0, 128)
												  : RGBA(255, 255, 255, 64));

	// TODO: fix this, take a look at 'nicenum'.
	int  digits = (int)(ceilf(log10f(vinc)));
	char fmt[16];
	snprintf(fmt, 16, "%%.%df", digits >= 0 ? 0 : -digits);
	char msg[128];
	snprintf(msg, 128, fmt, *val);

	m_rqueue.add_text(x + m_button_height / 2, y, m_widget_w, m_button_height, m_text_align, text,
					  text_color_hot(id, enabled));
	m_rqueue.add_text(x + m_button_height/2, y, m_widget_w, m_button_height, ALIGN_RIGHT,
					  msg, text_color_hot(id, enabled));

	bool _lastchange = wasActiveBefore && (!isItemActive) && was_focused;
	if (last_change)
		*last_change = _lastchange;

	return _lastchange || res || valChanged ||
		   (was_focused && !was_focusedBefore); // to catch last change in slider value
}
bool Ui::progress(float val, float vmin, float vmax, float vinc, uint color, bool enabled) {
	int  x, y, w, h;
	uint id;
	bool over, wasFocus;
	if (!start_control(enabled, x, y, w, h, id, over, wasFocus))
		return false;

	m_rqueue.add_rounded_rect(x, y, w, h, DEF_ROUND, RGBA(255, 255, 255, 64));

	const int range = w;
	float	 u = (val - vmin) / (vmax - vmin);
	if (u < 0)
		u = 0;
	if (u > 1)
		u = 1;
	int m = (int)(u * range);

	// if (is_item_active(id))
	//	m_rqueue.add_rounded_rect(x+m, y, SLIDER_MARKER_WIDTH, SLIDER_HEIGHT, 4,
	// RGBA(255,255,255,255));
	// else
	if (m > 0)
		m_rqueue.add_rounded_rect(x, y, m, m_button_height, DEF_ROUND, color);

	return true;
}
void Ui::set_options(size_t m_options) {
	m_render_options = m_options;
}
void Ui::indent() {
	m_widget_x += INTEND_SIZE;
	m_rollout_left += INTEND_SIZE;
	m_widget_w -= INTEND_SIZE;
	m_rollout_width -= INTEND_SIZE;
}
void Ui::unindent() {
	m_widget_x -= INTEND_SIZE;
	m_rollout_left -= INTEND_SIZE;
	m_widget_w += INTEND_SIZE;
	m_rollout_width += INTEND_SIZE;
}
void Ui::separator(bool draw_line) {
	if (!m_row)
		// m_widget_y -= DEFAULT_SPACING*3;
		m_widget_y -= m_button_height;
	else
		m_widget_x += m_button_height;

	if (draw_line) {
		int x = m_widget_x;
		int y = m_widget_y + m_button_height / 2;
		int w = m_widget_w;
		int h = m_button_height / 4;
		m_rqueue.add_rect(x, y, w, h, m_colors[BUTTON_COLOR]);
	}
}
void Ui::draw_text(int x, int y, int align, const char* text, uint color) {
	if (m_widget_id > 10 && (y < m_scroll_bottom || y > m_scroll_top))
		return;
	m_rqueue.add_text(x, y, m_widget_w, m_button_height, align, text, color);
}
// return previous button height
uint Ui::set_button_height(uint button_height) {
	uint mem = m_button_height;
	if (button_height)
		m_button_height = button_height;
	return mem;
}
uint Ui::get_button_height() {
	return m_button_height;
}
void Ui::set_padding(int left, int top, int right, int bottom) {
	m_padding_left = left;
	m_padding_top = top;
	m_padding_right = right;
	m_padding_bottom = bottom;
}
void Ui::set_property_width(float w) {
	m_property_width = w;
}
bool Ui::active_text(int x, int y, int align, const char* text, uint color, bool selected) {
	m_widget_id++;
	uint id = m_widget_id;

	size_t len = strlen(text);
	int	w = m_button_height * len;
	int	h = m_button_height;
	if (m_widget_id > 10 && (y < m_scroll_bottom || y > m_scroll_top))
		return false;
	bool over = in_rect(x, y, w, h, false);
	bool res = button_logic(id, over);

	if (is_item_hot(id) || selected) {
		// convert to upper case
		char up[256];
		for (size_t i = 0; i < len + 1; ++i) up[i] = toupper(text[i]);

		m_rqueue.add_text(x, y, m_widget_w, m_button_height, m_text_align, up,
						  m_colors[TEXT_COLOR_HOT]);
	}
	else
		m_rqueue.add_text(x, y, m_widget_w, m_button_height, m_text_align, text, color);

	return m_left && is_item_hot(id);
}
bool Ui::texture(const char* path, const frect& rc, bool blend) {
	m_rqueue.add_texture(path, rc, blend);
	return true;
}
void Ui::end_texture() {
	frect r;
	r.bottom = r.top = r.left = r.right = 0;
	m_rqueue.add_texture(0, r, false);
}
bool Ui::font(const char* path, float height) {
	m_rqueue.add_font(path, height);
	return true;
}
// check before item
//bool Ui::check_rect(int mouse_x, int mouse_y, uint id) {
//	if (id == CHECK_ITEM_RECT) {
//		int x = m_widget_x;
//		int y = m_widget_y - m_button_height;
//		int w = m_widget_w;
//		int h = m_button_height;
//		return mouse_x >= x && mouse_x <= x + w && mouse_y >= y && mouse_y <= y + h;
//	}
//	else if (id == CHECK_ROLLOUT_RECT) {
//		int x = m_rollout_left;
//		int y = m_rollout_top - m_button_height;
//		int w = m_rollout_width;
//		int h = m_rollout_height;
//		return mouse_x >= x && mouse_x <= x + w && mouse_y >= y && mouse_y <= y + h;
//	}
//	return true;
//}
uint Ui::text_align(uint align) {
	uint a = m_text_align;
	m_text_align = align;
	return a;
}
// bool Ui::custom(CUSTOM_RENDER_CALLBACK callback, int param, bool enabled) {
// 	m_widget_id++;
// 	uint id = (m_area_id << 16) | m_widget_id;

// 	int x = m_widget_x;
// 	int y = m_widget_y - m_button_height;
// 	int w = m_widget_w;
// 	int h = m_button_height;

// 	if (!m_row)
// 		m_widget_y -= m_button_height + DEFAULT_SPACING;
// 	else
// 		m_widget_x += w + DEFAULT_SPACING;

// 	bool over = enabled && in_rect(x, y, w, h);
// 	bool res = button_logic(id, over);

// 	// call custom drawing function
// 	if (m_widget_id > 10 && (y < m_scroll_bottom || y > m_scroll_top)) {
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
	m_colors[color_id] = clr;
}
void Ui::set_depth(int depth) {
	m_rqueue.add_depth(depth);
}
uint Ui::get_color(ColorScheme id) {
	if (id >= MAX_COLORS) {
		assert(false && "ColorScheme and ColorScheme should be the same");
		return 0;
	}
	return m_colors[id];
}
void Ui::set_cursor(CURSOR cursor) {
	if (cursor != m_cursor) {
		m_cursor = cursor;
		m_platform.set_cursor(cursor);
	}
}
bool Ui::clear_focus() {
	clear_active();
	set_focused(0);
	set_edit_buffer_id(0);
	m_edit_buffer[0] = 0;
	m_search_next_focus = false;
	return true;
}
void Ui::get_input(unsigned char* buttons, int* mouse_x, int* mouse_y, unsigned char* key,
				   bool& m_left_pressed, bool& m_left_released, bool& double_left_released) {
	*buttons = m_buttons;
	*mouse_x = m_mx;
	*mouse_y = m_my;
	*key = m_key;

	m_left_pressed = m_left_pressed;
	m_left_released = m_left_released;
	double_left_released = m_double_left_released;
}

const gfx_cmd* Ui::get_render_queue(int& size) {
	size = m_rqueue.get_size();
	return m_rqueue.get_queue();
}
Toolbar* Ui::get_root_toolbar() {
	return m_toolbar_root;
}
void Ui::set_root_toolbar(Toolbar* t) {
	clear_toolbars(m_toolbar_root); // remove current root toolbars
	delete (m_toolbar_root);
	m_toolbar_root = t;
	m_rollout_last = m_toolbar_root;
}
Ui::Rollouts& Ui::get_rollouts() {
	return m_rollouts;
}
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
