

#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>
#include <stdio.h>

#include "renderer_sdl.h"
#include "ui.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

bool init();
bool initGL();
void handleKeys(unsigned char key, int x, int y);
void update();
void render();
void close();

bool          quit = false;
SDL_Window*   gWindow = NULL;
SDL_GLContext gContext;
bool          gRenderQuad = true;

using namespace imgui;

Ui        ui;
Rollout*  rollout, *vert_rollout;
RenderSDL renderer;

bool init() {
    bool success = true;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        return false;
    }
    // Use OpenGL 3.2 core
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    gWindow = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                               SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (gWindow == NULL) {
        printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
        return false;
    }

    gContext = SDL_GL_CreateContext(gWindow);
    if (gContext == NULL) {
        printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
        return false;
    }

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
    rollout = ui.create_rollout("test", 0);
    ui.insert_rollout(rollout, 0.2f, true, nullptr);

	vert_rollout = ui.create_rollout("vert", 0);
	ui.insert_rollout(vert_rollout, -200, true, nullptr);

	ui.font("E:\\projects\\imgui\\DroidSans.ttf", 15);
    return success;
}

bool initGL() {
    glClearColor(0.f, 0.f, 0.f, 1.f);
    return renderer.create();
}
void handleKeys(unsigned char key, int x, int y) {
    if (key == 'q') {
        quit = true;
    }
}
void update() {
	int x, y;
	SDL_PumpEvents();
	int mouse_buttons = 0;
	if (SDL_GetMouseState(&x, &y) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
		mouse_buttons = MBUT_LEFT;
	}

	ui.begin_frame(SCREEN_WIDTH, SCREEN_HEIGHT, x, SCREEN_HEIGHT-y, mouse_buttons, 0, 0);

    ui.begin_rollout(rollout);
	static bool collapsed = false;
	if (ui.collapse("collapse", collapsed))
		collapsed = !collapsed;

	if (!collapsed){
		ui.indent();
		if (ui.button("Test")) {
			// do something
		}
		if (ui.button("Test2")) {
			// do something
		}
		ui.separator(true);
		if (ui.button("Test")) {
			// do something
		}
		if (ui.button("Test2")) {
			// do something
		}
		ui.unindent();
	}
	ui.end_rollout();

	ui.begin_rollout(vert_rollout);
	if (ui.button("Test")) {
		// do something
	}
	if (ui.button("Test2")) {
		// do something
	}
	ui.separator(true);
	if (ui.button("Test")) {
		// do something
	}
	if (ui.button("Test2")) {
		// do something
	}
	char buf[256];
	if (ui.property("Edit",buf, sizeof(buf),NULL)) {
		// do something
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

    SDL_Quit();
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
                }
                else if (e.type == SDL_TEXTINPUT) {
                    int x = 0, y = 0;
                    SDL_GetMouseState(&x, &y);
                    handleKeys(e.text.text[0], x, y);
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