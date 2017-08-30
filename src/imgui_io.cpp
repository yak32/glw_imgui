// glw_imgui
// Copyright (C) 2016 Iakov Sumygin - BSD license

#include "imgui_io.h"
#include "imgui_toolbars.h"
#include "glw_json/glw_json.h"
#include "imgui_ui.h"
#include "imgui_rollout.h"

using namespace json;

namespace imgui {

template <typename T> bool serialize(T& t, Rollout& v) {
	bool r = true;
	r &= SERIALIZE(id);
	r &= SERIALIZE(name);
	r &= SERIALIZE(options);
	r &= SERIALIZE(tabs);
	return r;
}
template <typename T> bool serialize(T& t, Toolbar& v) {
	bool r = true;
	r &= SERIALIZE(div);
	r &= SERIALIZE(h);
	r &= SERIALIZE(horz);
	r &= SERIALIZE(left);
	r &= SERIALIZE(right);
	r &= SERIALIZE(rollout);
	r &= SERIALIZE(w);
	return r;
}
bool save_layout(Ui& ui, const char* filename) {
	int res = save_object_to_file(filename, *ui.get_root_toolbar());
	if (res != JSON_OK) {
		LOG_ERROR("Saving layout file failed, error: %d", res);
		return false;
	}
	return true;
}
bool load_layout(Ui& ui, const char* filename) {
	Toolbar* root = new Toolbar(NULL);
	int res = load_object_from_file(filename, *root);
	if (res != JSON_OK) {
		LOG_ERROR("Opening layout file failed, error: %d", res);
		return false;
	}
	ui.set_root_toolbar(root);
	return true;
}
}
