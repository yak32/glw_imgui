

#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>
#include <stdio.h>
#include <string.h>

#include "platform_sdl.h"
#include "imgui_ui.h"
#include "imgui_io.h"

//#include "texture_atlas/texture_atlas.h"

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

bool init();
bool initGL();
void handleKeys(unsigned char key, int x, int y);
void update_ui();
void render();
void close();

bool quit = false;
unsigned int last_char = 0;
SDL_Window* gWindow = NULL;
SDL_GLContext gContext;
SDL_Renderer* gRenderer;
bool gRenderQuad = true;
int mouse_wheel = 0;

using namespace imgui;

PlatformSDL gPlatform;
Ui ui;
Rollout *root_rollout, *rollout, *vert_rollout;
RenderSDL renderer;

void getDisplayScaleFactor(float& x, float& y) {
	int w, h, low_dpi_w, low_dpi_h;
	SDL_GL_GetDrawableSize(gWindow, &w, &h);
	SDL_GetWindowSize(gWindow, &low_dpi_w, &low_dpi_h);
	x = (float)w / low_dpi_w;
	y = (float)h / low_dpi_h;
}
void setup_ui() {
	// scale ui to support high dpi
	float scaleX, scaleY;
	getDisplayScaleFactor(scaleX, scaleY);
	ui.set_item_height(15);

	ui.create(&gPlatform, &renderer);

	root_rollout = ui.create_rollout("root", ROLLOUT_HOLLOW | WND_STYLE);
	ui.insert_rollout(root_rollout, 1, true, NULL);

	rollout = ui.create_rollout("TEST", WND_STYLE);
	ui.insert_rollout(rollout, 0.2f, true, root_rollout);

	vert_rollout = ui.create_rollout("VERT", WND_STYLE);
	ui.insert_rollout(vert_rollout, -200, true, root_rollout);

	ui.font("DroidSans.ttf", 15);
}
bool init() {
	bool success = true;

	// Use OpenGL 3.2 core
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	gWindow = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
							   SCREEN_WIDTH, SCREEN_HEIGHT,
							   SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE |
								   SDL_WINDOW_ALLOW_HIGHDPI);
	if (gWindow == NULL) {
		printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	gContext = SDL_GL_CreateContext(gWindow);
	if (gContext == NULL) {
		printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	// gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);

	// Initialize GLEW
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK) {
		printf("Error initializing GLEW! %s\n", glewGetErrorString(glewError));
		success = false;
	}

	// Use Vsync
	if (SDL_GL_SetSwapInterval(1) < 0) {
		printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
	}

	if (!initGL()) {
		printf("Unable to initialize OpenGL!\n");
		success = false;
	}

	setup_ui();
	// if ( save_layout(ui, "C:\\projects\\glw_imgui\\test.imgui") )
	//    if (!load_layout(ui, "C:\\projects\\glw_imgui\\test.imgui"))
	//        printf("failed to load");


	// TextureAtlas a;
	// a.create(20,100);
	// unsigned int x, y;
	// a.add_box(10,10, &x, &y);
	// a.add_box(20, 20, &x, &y);
	// a.add_box(10, 10, &x, &y);
	// a.add_box(18, 18, &x, &y);
	// a.add_box(20, 90, &x, &y);

	return success;
}

bool initGL() {
	glClearColor(0.f, 0.f, 0.f, 1.f);
	return renderer.create();
}
void handleKeys(unsigned char key) {
	if (key == 'q') {
		quit = true;
	}
	last_char = key;
}

uint handle_input(int& mouse_x, int& mouse_y) {
	uint keysPressed = 0;
	uint mouse_keys = SDL_GetMouseState(&mouse_x, &mouse_y);
	if (mouse_keys & SDL_BUTTON(SDL_BUTTON_LEFT))
		keysPressed |= KEY_MOUSE_LEFT;

	if (mouse_keys & SDL_BUTTON(SDL_BUTTON_RIGHT))
		keysPressed |= KEY_MOUSE_RIGHT;

	const Uint8* keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_RETURN])
		keysPressed |= KEY_ENTER;

	if (keys[SDL_SCANCODE_DOWN])
		keysPressed |= KEY_DOWN;

	if (keys[SDL_SCANCODE_UP])
		keysPressed |= KEY_UP;

	if (keys[SDL_SCANCODE_LEFT])
		keysPressed |= KEY_LEFT;

	if (keys[SDL_SCANCODE_RIGHT])
		keysPressed |= KEY_RIGHT;
	
	return keysPressed;
}
void update_ui() {
	int x, y;
	
	uint keysPressed = handle_input(x, y);
	int w, h;
	SDL_GetWindowSize(gWindow, &w, &h);
	ui.set_text_align(ALIGN_LEFT);

	ui.begin_frame(w, h, x, y, -mouse_wheel, last_char, keysPressed);
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
}
void render() {
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	ui.render_draw(true);
}

void close() {
	SDL_GL_DeleteContext(gContext);
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;
}

int main(int argc, char* args[]) {
	if (!init()) {
		printf("Failed to initialize!\n");
	}
	else {
		SDL_Event e;
		SDL_StartTextInput();

		while (!quit) {
			while (SDL_PollEvent(&e) != 0) {
				// User requests quit
				if (e.type == SDL_QUIT) {
					quit = true;
				}
				else if (e.type == SDL_KEYDOWN) {
					if (e.key.keysym.sym == SDLK_ESCAPE)
						quit = true;
					handleKeys(e.key.keysym.sym);
				}
				else if (e.type == SDL_TEXTINPUT) {
					handleKeys(e.text.text[0]);
				}
				else if (e.type == SDL_MOUSEWHEEL) {
					mouse_wheel = e.wheel.y;
				}
			}
			update_ui();
			render();
			SDL_GL_SwapWindow(gWindow);
		}
		SDL_StopTextInput();
	}
	close();
	return 0;
}
