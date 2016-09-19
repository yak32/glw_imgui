// glw_imgui
// Copyright (C) 2016 Iakov Sumygin - BSD license

#ifndef _IMGUI_H_
#define _IMGUI_H_

#include <unordered_map>
#include <string>
#include "toolbars.h"
#include "render_queue.h"

namespace imgui {

class IRenderer;

typedef unsigned int uint;
typedef uint		 color;

#define LOGI(...)
#define LOG_ERROR(...)

enum Key { RETURN_KEY = 8, ENTER_KEY = 0x0D, TAB_KEY = 0x09 };
enum Options {
	DRAG_ITEM = 1,
	DRAG_AREA = 1 << 1,
	RESIZE_AREA = 1 << 2,
	SYSTEM_BUTTONS = 1 << 3,
	INPUT_DISABLED = 1 << 4,
	ROLLOUT_HOLLOW = 1 << 5,
	TOOLBAR = 1 << 6,
	NO_SCROLL = 1 << 7,
	ROLLOUT_ATTACHED = 1 << 31
};

enum BlendMode{
	BLEND_NONE = 0,
	BLEND_RECT,
	BLEND_TEXT	
};

struct Rollout;

inline uint RGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
	return (r) | (g << 8) | (b << 16) | (a << 24);
}
inline uint RGBA(uint rgb, unsigned char a) {
	rgb &= 0x00ffffff;
	return rgb | (a << 24);
}
static const int SCROLL_DISABLED = INT_MAX;
struct toolbar_t;

// direct correspondence with ColorScheme, check iui.h
enum ColorScheme {
	ROLLOUT_COLOR = 0,
	ROLLOUT_CAPTION_COLOR,
	BUTTON_COLOR,
	BUTTON_COLOR_ACTIVE,
	EDIT_COLOR,
	EDIT_COLOR_ACTIVE,
	COLLAPSE_COLOR,
	COLLAPSE_COLOR_ACTIVE,
	TEXT_COLOR,
	TEXT_COLOR_HOT,
	TEXT_COLOR_DISABLED,
	DRAG_COLOR,
	SLIDER_BG,
	MAX_COLORS
};
enum RolloutMoveSide {
	ROLLOUT_UNDEFINED = 0,
	ROLLOUT_LEFT,
	ROLLOUT_RIGHT,
	ROLLOUT_TOP,
	ROLLOUT_BOTTOM,
	ROLLOUT_LEFT_FIXED,
	ROLLOUT_RIGHT_FIXED,
	ROLLOUT_TOP_FIXED,
	ROLLOUT_BOTTOM_FIXED,
	ROLLOUT_CENTER
};
enum CURSOR {
	CURSOR_DEFAULT = 0,
	CURSOR_RESIZE_HORZ,
	CURSOR_RESIZE_VERT,
	CURSOR_RESIZE_CORNER,
	CURSOR_COUNT
};
enum ALIGN { ALIGN_LEFT = 0, ALIGN_RIGHT, ALIGN_BOTTOM, ALIGN_TOP, ALIGN_CENTER };
enum SOUNDS { SOUND_MOUSE_HOVER = 0, SOUND_CLICK };
enum INPUT_KEYS { MBUT_LEFT = 1, MBUT_RIGHT = 2, MBUT_LEFT_DBL = 4};
enum SCROLL_MODE { SCROLL_START = 0, SCROLL_END, SCROLL_CURRENT };

#define WND_STYLE DRAG_AREA | RESIZE_AREA | SYSTEM_BUTTONS;
#define DEFAULT_PADDING 6
static const int MAX_UI_LAYER_COUNT = 10;

class Ui {
public:
	typedef std::vector<Rollout*> rollouts_t;

	Ui();
	~Ui();

public:
	void begin_frame(uint width, uint height, int mx, int my, unsigned char mbut, int scroll,
					 char key);
	void end_frame();
	void cleanup();
	void set_options(size_t options);
	void indent();
	void unindent();
	void separator(bool draw_line);
	bool clear_focus();

	// rollouts
	Rollout* create_rollout(const char* name, int options);
	bool remove_rollout(Rollout* r);

	bool begin_rollout(Rollout* r);
	void end_rollout();

	bool insert_rollout(Rollout* r, float div, bool horz, Rollout* parent_rollout);
	bool detach_rollout(Rollout* r);

	bool show_rollout(Rollout* r);
	bool hide_rollout(Rollout* r);

	rollouts_t& get_rollouts();

	bool get_rollout_rect(Rollout* r, int& x, int& y, int& w, int& h);
	bool set_rollout_rect(Rollout* r, int x, int y, int w, int h);
	const char* get_rollout_name(Rollout* r);
	bool is_rollout_visible(Rollout* r);

	bool scroll_rollout(Rollout* r, int scroll, SCROLL_MODE mode);
	bool hit_test(int x, int y);

	// controls
	bool button(const char* text, bool enabled = true);
	bool button(const char* text, int x, int y, int width, int height, bool enabled);
	bool item(const char* text, bool selected = false, bool enabled = true);
	bool combo_item(const char* text, bool enabled = true);
	bool file_item(const char* text, char slash, bool selected = false, bool enabled = true);
	bool check(const char* text, bool checked, bool enabled = true);
	bool button_check(const char* text, bool checked, bool enabled = true);
	bool collapse(const char* text, bool checked, bool enabled = true);
	void label(const char* text);
	void rectangle(int x, int y, int width, int height, uint color);
	void triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint color);
	void value(const char* text);
	bool slider(const char* text, float* val, float vmin, float vmax, float vinc,
				bool* last_change = 0, bool enabled = true);
	bool progress(float val, float vmin, float vmax, float vinc,
				  uint color = RGBA(255, 196, 0, 128), bool enabled = true);
	bool edit(char* text, int buffer_len, bool* edit_finished, bool enabled = true);
	void row(uint count);
	void end_row();
	void set_widget_width(int width);
	bool property(const char* name, char* text, int buffer_len, bool* edit_finished,
				  bool enabled = true);
	bool combo(const char* text, const char* value, bool enabled = true);
	bool button_collapse(const char* text, bool enabled);
	bool file(const char* text, const char* value, bool enabled = true);
	bool color_edit(const char* text, color color, bool enabled = true, bool is_property = true);
	bool item_dropped(char* text, uint buffer_len, int& mouse_x, int& mouse_y);
	void draw_text(int x, int y, int align, const char* text, uint color);
	bool active_text(int x, int y, int align, const char* text, uint color, bool selected = false);

	uint set_button_height(uint button_height);
	uint get_button_height();
	void set_padding(int left, int top, int right, int bottom);
	void set_property_width(float w);
	RolloutMoveSide rollout_move(Rollout* dr, Rollout* r, int x, int y);
	bool rollout_move_rect(int x, int y, int w, int h);
	void update_input(int mx, int my, unsigned char mbut, int scroll, char key);
	void get_input(unsigned char* buttons, int* mousex, int* mousey, unsigned char* key,
				   bool& leftPressed, bool& leftReleased, bool& doubleLeftReleased);

	bool system_drag(int x, int y, int w, int h, int& xdiff, int& ydiff, bool& over);
	bool system_button(const char* text, int x, int y, int w, int h, bool enabled);

	void set_color(ColorScheme color_id, uint clr);
	void set_depth(int depth);
	uint get_color(ColorScheme color);
	bool texture(const char* path, const frect& rc, bool blend);
	void end_texture();
	void set_cursor(CURSOR cursor);
	bool font(const char* path, float height);
	// bool custom(CUSTOM_RENDER_CALLBACK callback, int param, bool enabled);
	uint text_align(uint align);
	//bool check_rect(int x, int y, uint id);

	const gfx_cmd* get_render_queue(int& size);

	toolbar_t* get_root_toolbar();
	void set_root_toolbar(toolbar_t* t);

	bool system_tab(const char* text, int x, int y, int w, int h, bool checked, int& xmove,
					int& ymove);

	void play_sound(SOUNDS s);

	// render
	bool render_init(IRenderer* r);
	void render_destroy(IRenderer* r);
	void render_draw(IRenderer* r, Ui* ui, bool transparency);

private:
	bool any_active() const;
	bool is_item_active(uint id) const;
	bool is_item_hot(uint id) const;
	bool is_item_focused(uint id) const;
	bool was_focused(uint id) const;
	bool is_edit_buffer(uint id) const;
	bool in_rect(int x, int y, int w, int h, bool checkScroll = true) const;
	void clear_input();
	void clear_active();
	void set_active(uint id);
	void set_hot(uint id);
	void set_focused(uint id);
	void set_edit_buffer_id(uint id);
	bool button_logic(uint id, bool over);
	bool edit_logic(uint id, bool wasFocus, bool enabled, char* text, int buffer_len,
					bool* edit_finished, char key);
	bool combo_button_logic(uint id, bool over);
	bool start_control(bool enabled, int& x, int& y, int& w, int& h, uint& id, bool& over,
					   bool& wasFocus);
	color text_color(uint id, bool enabled);
	color text_color_hot(uint id, bool enabled, bool focused = false);
	color button_color(uint id, bool enabled = true);
	color edit_color(uint id, bool enabled = true);

	void detach_tabbed_rollout(Rollout* r);

private:
	uint			m_width;
	uint			m_height;
	uint			m_render_options;
	uint			m_button_height;
	unsigned char   m_buttons;
	bool			m_left, m_double_left;
	bool			m_left_pressed, m_left_released, m_double_left_released;
	int				m_mx, m_my;
	int				m_scroll;
	uint			m_active;
	uint			m_hot;
	uint			m_hot_to_be;
	uint			m_focus;
	bool			m_is_hot;
	bool			m_is_active;
	bool			m_went_active;
	bool			m_search_next_focus; // tab pressed, search for next focus
	int				m_drag_x, m_drag_y;
	float			m_drag_orig;
	int				m_widget_x, m_widget_y, m_widget_w;
	int				m_rollout_width, m_rollout_height, m_rollout_left, m_rollout_top;
	int				m_row;
	bool			m_inside_current_scroll;
	char			m_key;
	uint			m_area_id;
	uint			m_widget_id;
	char			m_edit_buffer[256]; // used for holding buffer for current edited control
	char			m_drag_item[256];   // used for holding buffer for current draging item
	uint			m_drag_item_width;  // width of dragging item
	uint			m_edit_buffer_id;
	uint			m_options;
	int				m_alpha;
	uint			m_colors[MAX_COLORS];
	uint			m_text_align;
	int				m_padding_left, m_padding_right, m_padding_top, m_padding_bottom;
	float			m_property_width;
	RolloutMoveSide m_target_side;
	Rollout*		m_target_rollout;
	div_drag		m_rollout_drag_div;
	Rollout*		m_dragged_rollout_id;
	Rollout*		m_focused_rollout_id;
	toolbar_t 		*m_toolbar_root, *m_rollout_last;
	int		   		m_cursor;
	bool	   		m_cursor_over_drag;

	int			m_scroll_right;
	int			m_scroll_area_top;
	int			m_scroll_top;
	int		 	m_scroll_bottom;
	int*		m_scroll_val;
	int			m_focus_top;
	int			m_focus_bottom;
	uint 		m_scroll_id;
	bool		m_inside_scroll_area;

	rollouts_t m_rollouts;

	RenderQueue m_rqueue;


};
}
#endif // _IMGUI_H_
