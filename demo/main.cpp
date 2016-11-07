

#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>
#include <stdio.h>

#include "platform_sdl.h"
#include "ui.h"
#include "io.h"

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 768;

bool init();
bool initGL();
void handleKeys(unsigned char key, int x, int y);
void update();
void render();
void close();

bool            quit = false;
char            last_char = 0;
SDL_Window*     gWindow = NULL;
SDL_GLContext   gContext;
SDL_Renderer*   gRenderer;
bool            gRenderQuad = true;
int				mouse_wheel = 0;

using namespace imgui;

PlatformSDL	gPlatform;
Ui        ui(gPlatform);
Rollout  *root_rollout, *rollout, *vert_rollout;
RenderSDL renderer;

bool init() {
    bool success = true;

    // Use OpenGL 3.2 core
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    gWindow = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                               SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
                               SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI );
    if (gWindow == NULL) {
        printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
        return false;
    }

    gContext = SDL_GL_CreateContext(gWindow);
    if (gContext == NULL) {
        printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
        return false;
    }

	//gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);

    //Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if( glewError != GLEW_OK )
    {
        printf( "Error initializing GLEW! %s\n", glewGetErrorString( glewError ) );
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
	ui.render_init(&renderer);

	root_rollout = ui.create_rollout("root", ROLLOUT_HOLLOW | WND_STYLE);
	ui.insert_rollout(root_rollout, 1, true, nullptr);

	rollout = ui.create_rollout("TEST", WND_STYLE);
	ui.insert_rollout(rollout, 0.2f, true, root_rollout);

	vert_rollout = ui.create_rollout("VERT", WND_STYLE);
	ui.insert_rollout(vert_rollout, -200, true, root_rollout);

	ui.font("e:\\projects\\glw_imgui\\DroidSans.ttf", 15);

    //if ( save_layout(ui, "C:\\projects\\glw_imgui\\test.imgui") )
    //    if (!load_layout(ui, "C:\\projects\\glw_imgui\\test.imgui"))
    //        printf("failed to load");
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
void update() {
	int x, y;
	SDL_PumpEvents();
	int mouse_buttons = 0;
	if (SDL_GetMouseState(&x, &y) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
		mouse_buttons = MBUT_LEFT;
	}

    int w, h;
    SDL_GL_GetDrawableSize(gWindow, &w, &h);

	ui.begin_frame(w, h, x, h - y, mouse_buttons, -mouse_wheel, last_char);
	last_char = 0;
	mouse_wheel = 0;

    ui.begin_rollout(rollout);
	static bool collapsed = false;
	if (ui.collapse("collapse", collapsed))
		collapsed = !collapsed;

    if (!collapsed){

        ui.indent();
        ui.button("button1");

        static char combo_value[100] = {0};
        if ( ui.combo("combo", combo_value) )
        {
            if ( ui.combo_item("combo item1") )
                strcpy(combo_value, "combo item1");
            if ( ui.combo_item("combo item2") )
                strcpy(combo_value, "combo item2");
            if ( ui.combo_item("combo item3") )
                strcpy(combo_value, "combo item3");
            if ( ui.combo_item("combo item4") )
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
        ui.button("row1");ui.button("row2");ui.button("row3");
        ui.end_row();

        static char str_property[100] = "property val";
        ui.property("property", str_property, 100, NULL);

        if (ui.button_collapse("button collapse", true))
        {
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
    for (int i=0;i<100;++i){
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
    ui.render_draw(&renderer, &ui, true);
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
                    int x = 0, y = 0;
                    SDL_GetMouseState(&x, &y);
                    handleKeys(e.text.text[0]);
                }
				else if (e.type == SDL_MOUSEWHEEL) {
					int x = 0, y = 0;
					SDL_GetMouseState(&x, &y);
					mouse_wheel = e.wheel.y;
				}
            }
            update();
            render();
            SDL_GL_SwapWindow(gWindow);
        }
        SDL_StopTextInput();
    }
    close();
    return 0;
}
