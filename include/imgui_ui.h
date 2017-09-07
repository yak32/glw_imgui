// glw_imgui
// Copyright (C) 2016 Iakov Sumygin - BSD license

#ifndef _IMGUI_H_
#define _IMGUI_H_

#include <unordered_map>
#include <string>
#include <limits.h>
#include "imgui_toolbars.h"
#include "imgui_render_queue.h"
#include <atomic>
#include <mutex>

class TextureAtlas;

namespace imgui {

class IRenderer;
class IPlatform;

typedef unsigned int uint;
typedef uint color;

const uint UNDEFINED_TEXTURE = (uint)-1;

#define IMGUI_LOG_INFO(...)
#define IMGUI_LOG_ERROR(...)

enum Key {
	KEY_MOUSE_LEFT = 0x1,
	KEY_MOUSE_RIGHT = 0x2,
	KEY_ENTER = 0x4,
	KEY_DOWN = 0x8,
	KEY_UP = 0x10,
	KEY_LEFT = 0x20,
	KEY_RIGHT = 0x40,
	KEY_RETURN = 0x80
};
enum Options {
	DRAG_ITEM = 1,
	DRAG_AREA = 1 << 1,
	RESIZE_AREA = 1 << 2,
	SYSTEM_BUTTONS = 1 << 3,
	INPUT_DISABLED = 1 << 4,
	ROLLOUT_HOLLOW = 1 << 5,
	TOOLBAR = 1 << 6,
	DISABLE_SCROLL = 1 << 7,
	DISPLAY_SCROLL_BARS = 1 << 8,
	ROLLOUT_ATTACHED = 1 << 31
};

#define WND_STYLE (DRAG_AREA | RESIZE_AREA | SYSTEM_BUTTONS | DISPLAY_SCROLL_BARS)

enum BlendMode { BLEND_NONE = 0, BLEND_RECT, BLEND_TEXT };

inline uint RGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
	return (r) | (g << 8) | (b << 16) | (a << 24);
}
inline uint RGBA(uint rgb, unsigned char a) {
	rgb &= 0x00ffffff;
	return rgb | (a << 24);
}
static const int SCROLL_DISABLED = INT_MAX;

enum UiMode { MODE_BATCH_DRAW_CALLS = 0x1 };

// change Theme struct, if ColorScheme changed
enum ColorScheme {
	ROLLOUT_COLOR = 0,
	ROLLOUT_CAPTION_COLOR,
	BUTTON_COLOR,
	BUTTON_COLOR_ACTIVE,
	BUTTON_COLOR_FOCUSED,
	EDIT_COLOR,
	EDIT_COLOR_ACTIVE,
	COLLAPSE_COLOR,
	COLLAPSE_COLOR_ACTIVE,
	TEXT_COLOR,
	TEXT_COLOR_HOT,
	TEXT_COLOR_CHECKED,
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
enum ALIGN { ALIGN_LEFT = 0, ALIGN_RIGHT, ALIGN_TOP, ALIGN_BOTTOM, ALIGN_CENTER, ALIGN_VCENTER };
enum SOUNDS { SOUND_MOUSE_HOVER = 0, SOUND_CLICK };
enum SCROLL_MODE { SCROLL_START = 0, SCROLL_END, SCROLL_CURRENT };

static const int MAX_UI_LAYER_COUNT = 10;

struct texture_t {
	uint id;
	float x, y, width, height;
};

struct font_t;
struct Toolbar;
struct Rollout;

typedef unsigned int color_t;

struct Theme {
	color_t rollout_color;
	color_t rollout_caption_color;
	color_t button_color;
	color_t button_color_active;
	color_t button_color_focused;
	color_t edit_color;
	color_t edit_color_active;
	color_t collapse_color;
	color_t collapse_color_active;
	color_t text_color;
	color_t text_color_hot;
	color_t text_color_checked;
	color_t text_color_disabled;
	color_t drag_color;
	color_t slider_bg;
};

enum ROLLOUT_CONTROLS{
	ROLLOUT_TITLEBAR_ID = 0,
	ROLLOUT_MOVE_ROLLOUT_ID,
	ROLLOUT_RESIZE_LEFT_ID,
	ROLLOUT_RESIZE_RIGHT_ID,
	ROLLOUT_RESIZE_TOP_ID,
	ROLLOUT_RESIZE_BOTTOM_ID,
	ROLLOUT_RESIZE_CORNER_ID,
	ROLLOUT_CLOSE_WINDOW_ID,
	ROLLOUT_SCROLL_ID,
	ROLLOUT_CONTROLS_COUNT
};

class Ui {
public:
	typedef std::vector<Rollout*> Rollouts;

	Ui(uint mode = 0);
	~Ui();

public:
	bool create(IPlatform* p, IRenderer* r);
	void destroy();
	bool begin_frame(uint width, uint height, int mx, int my, int scroll, uint character, uint key);
	void end_frame();
	void cleanup();
	void set_options(uint options);
	void indent();
	void unindent();
	void separator(bool draw_line = false);
	bool clear_focus();

	// rollouts
	Rollout* create_rollout(const char* name, int options);
	bool remove_rollout(Rollout* r);
	Rollout* find_rollout(const char* name);

	bool begin_rollout(Rollout* r, bool focused = false);
	void end_rollout();

	bool insert_rollout(Rollout* r, float div, bool horz, Rollout* parent_rollout);
	bool detach_rollout(Rollout* r);

	bool show_rollout(Rollout* r, bool animate = true);
	bool hide_rollout(Rollout* r, bool animate = true);

	void scroll_rollout(Rollout* r, int scroll_val, bool animate = false,
						float animation_speed = 0.0f);

	void set_focus_rollout(Rollout* r);
	Rollout* get_focus_rollout();

	Rollouts& get_rollouts();

	bool get_rollout_rect(Rollout* r, int& x, int& y, int& w, int& h);
	bool set_rollout_rect(Rollout* r, int x, int y, int w, int h);
	const char* get_rollout_name(Rollout* r) const;
	bool is_rollout_visible(Rollout* r) const;

	unsigned int mode() const { return _mode; }

	bool scroll_rollout(Rollout* r, int scroll, SCROLL_MODE mode);
	bool hit_test(int x, int y) const;

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
	void rectangle(int height, uint color);
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
	int get_widget_width() const;
	bool property(const char* name, char* text, int buffer_len, bool* edit_finished,
				  bool enabled = true);
	bool combo(const char* text, const char* value, bool enabled = true);
	bool button_collapse(const char* text, bool enabled);
	bool file(const char* text, const char* value, bool enabled = true);
	bool color_edit(const char* text, color color, bool enabled = true, bool is_property = true);
	bool item_dropped(char* text, uint buffer_len, int& mouse_x, int& mouse_y);
	void draw_text(int x, int y, int align, const char* text, uint color);
	bool active_text(int x, int y, int align, const char* text, uint color, bool selected = false);

	bool key_pressed(Key key) const;
	bool key_released(Key key) const;

	uint set_item_height(uint button_height);
	uint get_item_height() const;
	void set_padding(int left, int top, int right, int bottom);
	void set_item_padding(int left, int top, int right, int bottom);
	void set_property_width(float w);
	RolloutMoveSide rollout_move(Rollout* dr, Rollout* r, int x, int y);
	bool rollout_move_rect(int x, int y, int w, int h);
	bool update_input(int mx, int my, int scroll, uint character, uint keys_state);
	void get_input(int* mouse_x, int* mouse_y, uint* keys_state, uint* character,
				   bool& left_pressed, bool& left_released) const;


	void set_color(ColorScheme color_id, color_t clr);
	color_t get_color(ColorScheme color) const;
	const Theme& get_theme() const;
	void set_theme(const Theme& theme);

	void set_depth(int depth);

	bool texture(const char* path, const frect& rc, bool blend = false);
	bool texture(const char* path);
	void end_texture();
	void set_cursor(CURSOR cursor);
	bool font(const char* path, float height);
	// bool custom(CUSTOM_RENDER_CALLBACK callback, int param, bool enabled);
	void set_text_align(uint align);
	uint get_text_align() const;
	bool check_rect(int x, int y, uint id) const;

	const gfx_cmd* get_render_queue(uint& size);

	Toolbar* get_root_toolbar();
	void set_root_toolbar(Toolbar* t);

	Rollout* get_root_rollout();

	void play_sound(SOUNDS s);
	void render_draw(bool transparency);

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
					bool* edit_finished);
	bool combo_button_logic(uint id, bool over);
	bool start_control(bool enabled, int& x, int& y, int& w, int& h, uint& id, bool& over,
					   bool& wasFocus);
	color text_color(uint id, bool enabled);
	color text_color_hot(uint id, bool enabled, bool focused = false);
	color button_color(uint id, bool enabled = true);
	color edit_color(uint id, bool enabled = true);

	void detach_tabbed_rollout(Toolbar* n, Rollout* r, Rollout* detach_rollout);

	bool system_drag(uint id, int x, int y, int w, int h, int& xdiff, int& ydiff, bool& over);
	bool system_button(uint id, const char* text, int x, int y, int w, int h, bool enabled);
	bool system_tab(uint id, const char* text, int x, int y, int w, int h, bool checked, int& xdiff, int& ydiff, bool& over);

	// render
	bool render_init(IRenderer* r);
	bool render_destroy();
	void on_render_finished();

	bool load_font(const char* path, float font_height, font_t& font);
	bool bind_texture(const char* path);

	void render_text(const font_t& font, float x, float y, float w, float h, const char* text,
					 int align, unsigned int col, float depth);

	inline const int slider_height() const;
	inline const int slider_marker_width() const;
	inline const int check_size() const;
	inline const int default_spacing() const;
	// inline const int ITEM_SPACING(ITEM side) const;
	inline const int padding() const;
	inline const int intend_size() const;
	inline const int area_header() const;
	inline const int toolbar_header() const;
	inline const int default_round() const;
	inline const int default_padding() const;

	uint get_control_id(uint widget_id) const;
	uint get_control_id(uint area_id, uint widget_id) const;
	int render_rollout_tabs(Rollout& r, int x, int y, int h, int caption_y, int caption_height,
							int area_header);
	void process_rollout_resize(Rollout& r, int x, int y, int w, int h, int caption_t,
								int caption_height);
	bool render_caption(Rollout& r, int x, int y, int w, int h, int caption_y, int caption_height,
						int area_header);

private:
	uint _width;
	uint _height;
	bool _left, _double_left;
	int _keys_state, _prev_keys_state;
	bool _left_pressed, _left_released, _double_left_released;
	int _mx, _my;
	int _scroll;
	uint _render_options;
	uint _item_height;

	uint _active;
	uint _hot;
	uint _hot_to_be;
	uint _prev_enabled_id;
	bool _is_hot;
	bool _is_active;
	bool _went_active;
	bool _search_next_focus; // tab pressed, search for next focus
	int _drag_x, _drag_y;
	float _drag_orig;
	int _widget_x, _widget_y, _widget_w;
	int _rollout_width, _rollout_height, _rollout_left, _rollout_top;
	bool _inside_current_scroll;
	uint _area_id;
	uint _character;
	uint _widget_id;
	uint _focus;

	char _edit_buffer[256]; // used for holding buffer for current edited control
	int _row;
	uint _drag_item_width; // width of dragging item
	uint _options;
	int _alpha;
	uint _text_align;
	int _padding_left, _padding_right, _padding_top, _padding_bottom;
	int _item_padding_left, _item_padding_right, _item_padding_top, _item_padding_bottom;
	float _property_width;
	Rollout* _dragged_rollout_id;
	Rollout* _focused_rollout_id;
	int _cursor;
	bool _cursor_over_drag;
	int _scroll_right;
	int _scroll_area_top;
	int* _scroll_val;
	int _focus_top;
	int _focus_bottom;
	bool _inside_scroll_area;
	int _scroll_top;
	int _scroll_bottom;
	IPlatform* _platform;

	char _drag_item[256]; // used for holding buffer for current draging item
	uint _edit_buffer_id;

	RolloutMoveSide _target_side;
	Rollout* _target_rollout;
	div_drag _rollout_drag_div;
	Toolbar *_toolbar_root, *_rollout_last;

	Rollouts _rollouts;

	RenderQueue _rqueues[2];
	RenderQueue *_rqueue, *_rqueue_display;
	std::mutex _mutex;
	IRenderer* _renderer;
	std::unordered_map<std::string, font_t*> _fonts;
	std::unordered_map<std::string, texture_t> _textures;

	texture_t _current_texture;
	texture_t _white_texture;

	std::unordered_map<std::string, font_t*>::iterator _current_font;
	bool _blend_texture;
	float _depth;
	uint _mode;

	uint _atlas;
	TextureAtlas* _texture_atlas;
	Theme _theme;

	Rollout* _focus_rollout;
	Rollout* _rollout_root;
};
}
#endif // _IMGUI_H_
