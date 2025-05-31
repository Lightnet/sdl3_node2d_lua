#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <glad/gl.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "module_gl.h"
#include "module_lua.h"

int main(int argc, char *argv[]) {
    // Initialize SDL3
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        SDL_Log("TTF_Init failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Initialize Lua
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    // Load Lua script
    if (luaL_dofile(L, "script.lua") != LUA_OK) {
        SDL_Log("Lua script error: %s", lua_tostring(L, -1));
        lua_close(L);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Get window configuration from Lua
    lua_getglobal(L, "config");
    lua_getfield(L, -1, "window_title");
    const char *window_title = lua_tostring(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, -1, "window_width");
    int window_width = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, -1, "window_height");
    int window_height = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);
    lua_pop(L, 1); // Pop config table

    // Create SDL window with OpenGL
    SDL_Window *window = SDL_CreateWindow(
        window_title ? window_title : "SDL3 Lua App",
        window_width ? window_width : 800,
        window_height ? window_height : 600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        lua_close(L);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Initialize OpenGL
    SDL_GLContext gl_context = init_opengl(window);
    if (!gl_context) {
        SDL_DestroyWindow(window);
        lua_close(L);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Load font
    lua_getglobal(L, "config");
    lua_getfield(L, -1, "font_path");
    const char *font_path = lua_tostring(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, -1, "font_size");
    int font_size = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);
    lua_pop(L, 1); // Pop config table

    TTF_Font *font = TTF_OpenFont(font_path ? font_path : "Kenney Mini.ttf", font_size ? font_size : 24);
    if (!font) {
        SDL_Log("TTF_OpenFont failed: %s", SDL_GetError());
        SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
        lua_close(L);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Main loop
    SDL_Event event;
    bool running = true;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Clear screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Get square properties from Lua
        lua_getglobal(L, "config");
        lua_getfield(L, -1, "square");
        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, "x");
            float square_x = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 400.0f;
            lua_pop(L, 1);
            lua_getfield(L, -1, "y");
            float square_y = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 300.0f;
            lua_pop(L, 1);
            lua_getfield(L, -1, "size");
            float square_size = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 100.0f;
            lua_pop(L, 1);
            lua_getfield(L, -1, "r");
            float square_r = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 1.0f;
            lua_pop(L, 1);
            lua_getfield(L, -1, "g");
            float square_g = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 0.0f;
            lua_pop(L, 1);
            lua_getfield(L, -1, "b");
            float square_b = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 0.0f;
            lua_pop(L, 1);

            // Render square
            render_square(square_x, square_y, square_size, square_r, square_g, square_b, window);
        }
        lua_pop(L, 1); // Pop square table
        lua_pop(L, 1); // Pop config table

        // Render text
        lua_getglobal(L, "config");
        lua_getfield(L, -1, "text");
        const char *text = lua_tostring(L, -1);
        lua_pop(L, 1);
        lua_pop(L, 1); // Pop config table

        render_text(font, text ? text : "Hello, World!", window);

        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    TTF_CloseFont(font);
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    lua_close(L);
    TTF_Quit();
    SDL_Quit();
    return 0;
}