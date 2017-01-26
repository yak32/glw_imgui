// glw_imgui
// Copyright (C) 2016 Iakov Sumygin - BSD license

#include <memory>

namespace imgui {
struct Rollout {
	// same as Options

	static const int MAX_TABS = 20;
	Rollout(int _x = 0, int _y = 0, int _w = 0, int _h = 0, int _z = 0);
	void set(int _x, int _y, int _w, int _h, int _z, bool _visible, bool _minimized,
			 int _alpha_inc = 0);
	void set(const char* _name = "", int _options = 0, bool _visible = true,
			 bool _minimized = false, int _alpha_inc = 0);
	int top() const;
	bool is_visible() const;

	int x, y, w, h, z;
	int scroll;
	int alpha_inc;
	int alpha;

	bool minimized;
	std::string name;
	int options;
	int id;

	typedef std::vector<int> tabs_array_t;
	tabs_array_t tabs;
};
}
