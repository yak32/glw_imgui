# glw_imgui - C++ IMGUI implementation
Immediate Mode UI C++ implementation.

IMGUI is a code-driven, simple and bloat-free GUI system, widely used in modern game engines and games.
Best explanation of IMGUI concept is [here](https://www.youtube.com/watch?v=Z1qyvQsjK5Y).

#### IMGUI: creation, usage and rendering of a button in one line:
```c++

// simple button
if (gui.button("Cancel"))
	do_cancel_action();

```
All logic is hidden in the Gui::button() call, rendering commands are issued and added to the render queue. String pooling is used to pack strings in render queue and avoid memory allocation during rendering. Rendering is graphics API independent, platform specific rendering should be implemented (the reference implementation by SDL is provided in the demo project.)

The library has basic suppot for layouting (attachable toolbars, layout serialization), themes (right now colors only), lock-free multithreading (triple buffering).

### Gallery
Basic GUI manipulations, various controls, resizable and movable toolbars. Entire UI is built using IMGUI.
[![ScreenShot](/../feature-screenshots/screenshots/IMGUI_toolbars.png)](https://www.youtube.com/watch?v=TlJiuguyLVo)

The Editor of [Glow game engine](www.glow3d.com)

### Sample UI update loop with various controls
```c++
	int mouse_x, mouse_y;

	uint keysPressed = handle_input(mouse_x, mouse_y);
	int width, height;
	SDL_GetWindowSize(gWindow, &width, &height);
	ui.set_text_align(ALIGN_LEFT);

	ui.begin_frame(width, height, mouse_x, mouse_y, -mouse_wheel, last_char, keysPressed);
	last_char = 0;
	mouse_wheel = 0;

	ui.begin_rollout(rollout);
	static bool collapsed = false;
	if (ui.collapse("collapse", collapsed))
		collapsed = !collapsed;

	if (!collapsed) {
		ui.indent();
		ui.button("button1");

		static char combo_value[100] = {0};
		if (ui.combo("combo", combo_value)) {
			if (ui.combo_item("combo item1"))
				strcpy(combo_value, "combo item1");
			if (ui.combo_item("combo item2"))
				strcpy(combo_value, "combo item2");
			if (ui.combo_item("combo item3"))
				strcpy(combo_value, "combo item3");
			if (ui.combo_item("combo item4"))
				strcpy(combo_value, "combo item4");
		}

		static bool checked = false;
		if (ui.check("checkbox", checked))
			checked = !checked;

		static bool button_checked = false;
		if (ui.button_check("button checked", button_checked))
			button_checked = !button_checked;

		ui.label("label");
		ui.value("value");
		static float val = 1.0f;
		ui.slider("slider", &val, 0.0f, 10.0f, 1.0f);

		static float progress = 1.0f;
		ui.progress(progress, 0.0f, 10.0f, 1.0f);

		static char str[100];
		ui.edit(str, 100, NULL);

		ui.row(3);
		ui.button("row1");
		ui.button("row2");
		ui.button("row3");
		ui.end_row();

		static char str_property[100] = "property val";
		ui.property("property", str_property, 100, NULL);

		if (ui.button_collapse("button collapse", true)) {
			ui.item("item1");
			ui.item("item2");
			ui.item("item3");
		}

		ui.draw_text(5, 5, 0, "draw item", 0xffffffff);

		ui.unindent();
	}
	ui.end_rollout();

	ui.begin_rollout(vert_rollout);

	char str[100];
	static int selected = -1;
	for (int i = 0; i < 100; ++i) {
		sprintf(str, "item %d", i);
		if (ui.item(str, i == selected))
			selected = i;
	}
	ui.end_rollout();

	ui.end_frame();
```
![Alt text](/../feature-screenshots/screenshots/ref_ui.png)

## FAQ

The license?
[BSD](https://opensource.org/licenses/BSD-2-Clause)

### Dependencies
 STL, c-runtime.

### Performance
Rendering - almost allocation-free (and I will try to make it allocation-free completely in the future)

### Compartibility
  Working on Windows (tested with Visual Studio 2013, 2015), MacOS and Linux.


