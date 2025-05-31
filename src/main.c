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

    // Dragging and panning state
    bool is_dragging = false;
    bool is_panning = false;
    float drag_offset_x = 0.0f;
    float drag_offset_y = 0.0f;
    float pan_start_x = 0.0f;
    float pan_start_y = 0.0f;

    // Main loop
    SDL_Event event;
    bool running = true;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
                // Get camera properties for click detection
                lua_getglobal(L, "config");
                lua_getfield(L, -1, "camera");
                float cam_x = 0.0f, cam_y = 0.0f, cam_scale = 1.0f;
                if (lua_istable(L, -1)) {
                    lua_getfield(L, -1, "x");
                    cam_x = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 0.0f;
                    lua_pop(L, 1);
                    lua_getfield(L, -1, "y");
                    cam_y = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 0.0f;
                    lua_pop(L, 1);
                    lua_getfield(L, -1, "scale");
                    cam_scale = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 1.0f;
                    lua_pop(L, 1);
                }
                lua_pop(L, 1); // Pop camera table

                // Transform mouse coordinates to world space
                int win_width, win_height;
                SDL_GetWindowSize(window, &win_width, &win_height);
                float mouse_x = event.button.x;
                float mouse_y = event.button.y;
                float world_x = mouse_x / cam_scale + cam_x;
                float world_y = mouse_y / cam_scale + cam_y;

                // Get square properties to check if click is within bounds
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

                    // Check if click is within square bounds
                    float half_size = square_size / 2.0f;
                    if (world_x >= square_x - half_size && world_x <= square_x + half_size &&
                        world_y >= square_y - half_size && world_y <= square_y + half_size) {
                        is_dragging = true;
                        drag_offset_x = world_x - square_x;
                        drag_offset_y = world_y - square_y;
                        SDL_Log("Dragging started: mouse=(%.1f, %.1f), world=(%.1f, %.1f), square=(%.1f, %.1f), bounds=[%.1f, %.1f]x[%.1f, %.1f], cam=(%.1f, %.1f, %.2f)", 
                                mouse_x, mouse_y, world_x, world_y, square_x, square_y,
                                square_x - half_size, square_x + half_size, square_y - half_size, square_y + half_size,
                                cam_x, cam_y, cam_scale);
                    } else {
                        SDL_Log("Click outside square: mouse=(%.1f, %.1f), world=(%.1f, %.1f), square=(%.1f, %.1f), bounds=[%.1f, %.1f]x[%.1f, %.1f], cam=(%.1f, %.1f, %.2f)", 
                                mouse_x, mouse_y, world_x, world_y, square_x, square_y,
                                square_x - half_size, square_x + half_size, square_y - half_size, square_y + half_size,
                                cam_x, cam_y, cam_scale);
                    }
                }
                lua_pop(L, 1); // Pop square table
                lua_pop(L, 1); // Pop config table
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
                is_dragging = false;
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_MIDDLE) {
                is_panning = true;
                pan_start_x = event.button.x;
                pan_start_y = event.button.y;
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_MIDDLE) {
                is_panning = false;
            }
            else if (event.type == SDL_EVENT_MOUSE_MOTION && is_dragging) {
                // Get camera properties
                lua_getglobal(L, "config");
                lua_getfield(L, -1, "camera");
                float cam_x = 0.0f, cam_y = 0.0f, cam_scale = 1.0f;
                if (lua_istable(L, -1)) {
                    lua_getfield(L, -1, "x");
                    cam_x = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 0.0f;
                    lua_pop(L, 1);
                    lua_getfield(L, -1, "y");
                    cam_y = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 0.0f;
                    lua_pop(L, 1);
                    lua_getfield(L, -1, "scale");
                    cam_scale = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 1.0f;
                    lua_pop(L, 1);
                }
                lua_pop(L, 1); // Pop camera table

                // Transform mouse coordinates to world space
                int win_width, win_height;
                SDL_GetWindowSize(window, &win_width, &win_height);
                float mouse_x = event.motion.x;
                float mouse_y = event.motion.y;
                float world_x = mouse_x / cam_scale + cam_x;
                float world_y = mouse_y / cam_scale + cam_y;

                // Update square position in Lua table
                lua_getfield(L, -1, "square");
                if (lua_istable(L, -1)) {
                    lua_pushnumber(L, world_x - drag_offset_x);
                    lua_setfield(L, -2, "x");
                    lua_pushnumber(L, world_y - drag_offset_y);
                    lua_setfield(L, -2, "y");
                }
                lua_pop(L, 1); // Pop square table
                lua_pop(L, 1); // Pop config table
            }
            else if (event.type == SDL_EVENT_MOUSE_MOTION && is_panning) {
                // Get window size and camera properties
                int win_width, win_height;
                SDL_GetWindowSize(window, &win_width, &win_height);
                lua_getglobal(L, "config");
                lua_getfield(L, -1, "camera");
                float cam_x = 0.0f, cam_y = 0.0f, cam_scale = 1.0f;
                if (lua_istable(L, -1)) {
                    lua_getfield(L, -1, "x");
                    cam_x = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 0.0f;
                    lua_pop(L, 1);
                    lua_getfield(L, -1, "y");
                    cam_y = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 0.0f;
                    lua_pop(L, 1);
                    lua_getfield(L, -1, "scale");
                    cam_scale = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 1.0f;
                    lua_pop(L, 1);
                }

                // Calculate pan delta in world space
                float delta_x = (event.motion.x - pan_start_x) / cam_scale;
                float delta_y = (event.motion.y - pan_start_y) / cam_scale;

                // Update camera position
                lua_pushnumber(L, cam_x - delta_x);
                lua_setfield(L, -2, "x");
                lua_pushnumber(L, cam_y - delta_y);
                lua_setfield(L, -2, "y");

                // Update pan start position
                pan_start_x = event.motion.x;
                pan_start_y = event.motion.y;

                lua_pop(L, 1); // Pop camera table
                lua_pop(L, 1); // Pop config table
            }
            else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                // Get mouse position and current camera properties
                int win_width, win_height;
                SDL_GetWindowSize(window, &win_width, &win_height);
                float mouse_x, mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);

                lua_getglobal(L, "config");
                lua_getfield(L, -1, "camera");
                float cam_x = 0.0f, cam_y = 0.0f, cam_scale = 1.0f;
                if (lua_istable(L, -1)) {
                    lua_getfield(L, -1, "x");
                    cam_x = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 0.0f;
                    lua_pop(L, 1);
                    lua_getfield(L, -1, "y");
                    cam_y = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 0.0f;
                    lua_pop(L, 1);
                    lua_getfield(L, -1, "scale");
                    cam_scale = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 1.0f;
                    lua_pop(L, 1);
                }

                // Zoom
                float zoom_factor = event.wheel.y > 0 ? 1.1f : 0.9f;
                float new_scale = cam_scale * zoom_factor;

                // Adjust camera position to zoom towards mouse
                float world_x_before = mouse_x / cam_scale + cam_x;
                float world_y_before = mouse_y / cam_scale + cam_y;
                float world_x_after = mouse_x / new_scale + cam_x;
                float world_y_after = mouse_y / new_scale + cam_y;
                float new_cam_x = cam_x + (world_x_before - world_x_after);
                float new_cam_y = cam_y + (world_y_before - world_y_after);

                // Update camera in Lua table
                lua_pushnumber(L, new_cam_x);
                lua_setfield(L, -2, "x");
                lua_pushnumber(L, new_cam_y);
                lua_setfield(L, -2, "y");
                lua_pushnumber(L, new_scale);
                lua_setfield(L, -2, "scale");

                lua_pop(L, 1); // Pop camera table
                lua_pop(L, 1); // Pop config table
            }
        }

        // Clear screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Get camera properties
        lua_getglobal(L, "config");
        lua_getfield(L, -1, "camera");
        float cam_x = 0.0f, cam_y = 0.0f, cam_scale = 1.0f;
        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, "x");
            cam_x = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 0.0f;
            lua_pop(L, 1);
            lua_getfield(L, -1, "y");
            cam_y = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 0.0f;
            lua_pop(L, 1);
            lua_getfield(L, -1, "scale");
            cam_scale = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 1.0f;
            lua_pop(L, 1);
        }
        lua_pop(L, 1); // Pop camera table

        // Get square properties from Lua
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
            render_square(square_x, square_y, square_size, square_r, square_g, square_b, window, cam_x, cam_y, cam_scale);
        }
        lua_pop(L, 1); // Pop square table
        lua_pop(L, 1); // Pop config table

        // Render text
        lua_getglobal(L, "config");
        lua_getfield(L, -1, "text");
        const char *text = lua_tostring(L, -1);
        lua_pop(L, 1);
        lua_pop(L, 1); // Pop config table

        render_text(font, text ? text : "Hello, World!", window, cam_x, cam_y, cam_scale);

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