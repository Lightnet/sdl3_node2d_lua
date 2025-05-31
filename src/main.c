#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <glad/gl.h>
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
    lua_State *L = lua_utils_init("script.lua");
    if (!L) {
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Get window configuration
    const char *window_title = lua_utils_get_string(L, "config", "window_title", "SDL3 Lua App");
    int window_width = lua_utils_get_integer(L, "config", "window_width", 800);
    int window_height = lua_utils_get_integer(L, "config", "window_height", 600);

    // Create SDL window with OpenGL
    SDL_Window *window = SDL_CreateWindow(
        window_title,
        window_width,
        window_height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        lua_utils_cleanup(L);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Initialize OpenGL
    SDL_GLContext gl_context = init_opengl(window);
    if (!gl_context) {
        SDL_DestroyWindow(window);
        lua_utils_cleanup(L);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Load font
    const char *font_path = lua_utils_get_string(L, "config", "font_path", "Kenney Mini.ttf");
    int font_size = lua_utils_get_integer(L, "config", "font_size", 24);
    TTF_Font *font = TTF_OpenFont(font_path, font_size);
    if (!font) {
        SDL_Log("TTF_OpenFont failed: %s", SDL_GetError());
        SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
        lua_utils_cleanup(L);
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
                // Get camera properties
                float cam_x = lua_utils_get_number(L, "config", "camera.x", 0.0f);
                float cam_y = lua_utils_get_number(L, "config", "camera.y", 0.0f);
                float cam_scale = lua_utils_get_number(L, "config", "camera.scale", 1.0f);

                // Transform mouse coordinates to world space
                int win_width, win_height;
                SDL_GetWindowSize(window, &win_width, &win_height);
                float mouse_x = event.button.x;
                float mouse_y = event.button.y;
                float world_x = mouse_x / cam_scale + cam_x;
                float world_y = mouse_y / cam_scale + cam_y;

                // Get square properties
                float square_x = lua_utils_get_number(L, "config", "square.x", 400.0f);
                float square_y = lua_utils_get_number(L, "config", "square.y", 300.0f);
                float square_size = lua_utils_get_number(L, "config", "square.size", 100.0f);

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
                float cam_x = lua_utils_get_number(L, "config", "camera.x", 0.0f);
                float cam_y = lua_utils_get_number(L, "config", "camera.y", 0.0f);
                float cam_scale = lua_utils_get_number(L, "config", "camera.scale", 1.0f);

                // Transform mouse coordinates to world space
                int win_width, win_height;
                SDL_GetWindowSize(window, &win_width, &win_height);
                float mouse_x = event.motion.x;
                float mouse_y = event.motion.y;
                float world_x = mouse_x / cam_scale + cam_x;
                float world_y = mouse_y / cam_scale + cam_y;

                // Update square position
                lua_utils_set_number(L, "config", "square.x", world_x - drag_offset_x);
                lua_utils_set_number(L, "config", "square.y", world_y - drag_offset_y);
            }
            else if (event.type == SDL_EVENT_MOUSE_MOTION && is_panning) {
                // Get window size and camera properties
                int win_width, win_height;
                SDL_GetWindowSize(window, &win_width, &win_height);
                float cam_x = lua_utils_get_number(L, "config", "camera.x", 0.0f);
                float cam_y = lua_utils_get_number(L, "config", "camera.y", 0.0f);
                float cam_scale = lua_utils_get_number(L, "config", "camera.scale", 1.0f);

                // Calculate pan delta in world space
                float delta_x = (event.motion.x - pan_start_x) / cam_scale;
                float delta_y = (event.motion.y - pan_start_y) / cam_scale;

                // Update camera position
                lua_utils_set_number(L, "config", "camera.x", cam_x - delta_x);
                lua_utils_set_number(L, "config", "camera.y", cam_y - delta_y);

                // Update pan start position
                pan_start_x = event.motion.x;
                pan_start_y = event.motion.y;
            }
            else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                // Get mouse position and current camera properties
                int win_width, win_height;
                SDL_GetWindowSize(window, &win_width, &win_height);
                float mouse_x, mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);
                float cam_x = lua_utils_get_number(L, "config", "camera.x", 0.0f);
                float cam_y = lua_utils_get_number(L, "config", "camera.y", 0.0f);
                float cam_scale = lua_utils_get_number(L, "config", "camera.scale", 1.0f);

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

                // Update camera
                lua_utils_set_number(L, "config", "camera.x", new_cam_x);
                lua_utils_set_number(L, "config", "camera.y", new_cam_y);
                lua_utils_set_number(L, "config", "camera.scale", new_scale);
            }
        }

        // Clear screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Get camera properties
        float cam_x = lua_utils_get_number(L, "config", "camera.x", 0.0f);
        float cam_y = lua_utils_get_number(L, "config", "camera.y", 0.0f);
        float cam_scale = lua_utils_get_number(L, "config", "camera.scale", 1.0f);

        // Get square properties
        float square_x = lua_utils_get_number(L, "config", "square.x", 400.0f);
        float square_y = lua_utils_get_number(L, "config", "square.y", 300.0f);
        float square_size = lua_utils_get_number(L, "config", "square.size", 100.0f);
        float square_r = lua_utils_get_number(L, "config", "square.r", 1.0f);
        float square_g = lua_utils_get_number(L, "config", "square.g", 0.0f);
        float square_b = lua_utils_get_number(L, "config", "square.b", 0.0f);

        // Render square
        render_square(square_x, square_y, square_size, square_r, square_g, square_b, window, cam_x, cam_y, cam_scale);

        // Render text
        const char *text = lua_utils_get_string(L, "config", "text", "Hello, World!");
        render_text(font, text, window, cam_x, cam_y, cam_scale);

        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    TTF_CloseFont(font);
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    lua_utils_cleanup(L);
    TTF_Quit();
    SDL_Quit();
    return 0;
}