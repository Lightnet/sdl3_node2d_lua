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
    SDL_GLContext gl_context = init_opengl_context(window);
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
    int dragged_node_index = 0; // 1-based index of dragged node
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

                // Check nodes for click (last node has priority)
                int node_count = lua_utils_get_nodes_count(L);
                dragged_node_index = 0;
                for (int i = 1; i <= node_count; i++) {
                    float node_x = lua_utils_get_node_number(L, i, "x", 400.0f);
                    float node_y = lua_utils_get_node_number(L, i, "y", 300.0f);
                    float node_size = lua_utils_get_node_number(L, i, "size", 100.0f);
                    const char* node_text = lua_utils_get_node_text(L, i, "");

                    float half_size = node_size / 2.0f;
                    if (world_x >= node_x - half_size && world_x <= node_x + half_size &&
                        world_y >= node_y - half_size && world_y <= node_y + half_size) {
                        dragged_node_index = i;
                        drag_offset_x = world_x - node_x;
                        drag_offset_y = world_y - node_y;
                        is_dragging = true;
                        SDL_Log("Dragging started: node=%d, text='%s', mouse=(%.1f, %.1f), world=(%.1f, %.1f), node=(%.1f, %.1f), bounds=[%.1f, %.1f]x[%.1f, %.1f], cam=(%.1f, %.1f, %.2f)",
                                i, node_text, mouse_x, mouse_y, world_x, world_y, node_x, node_y,
                                node_x - half_size, node_x + half_size, node_y - half_size, node_y + half_size,
                                cam_x, cam_y, cam_scale);
                    }
                }
                if (!is_dragging && node_count > 0) {
                    float node_x = lua_utils_get_node_number(L, 1, "x", 400.0f);
                    float node_y = lua_utils_get_node_number(L, 1, "y", 300.0f);
                    float node_size = lua_utils_get_node_number(L, 1, "size", 100.0f);
                    const char* node_text = lua_utils_get_node_text(L, 1, "");
                    float half_size = node_size / 2.0f;
                    SDL_Log("Click outside nodes: mouse=(%.1f, %.1f), world=(%.1f, %.1f), node1=(%.1f, %.1f), text='%s', bounds=[%.1f, %.1f]x[%.1f, %.1f], cam=(%.1f, %.1f, %.2f)",
                            mouse_x, mouse_y, world_x, world_y, node_x, node_y, node_text,
                            node_x - half_size, node_x + half_size, node_y - half_size, node_y + half_size,
                            cam_x, cam_y, cam_scale);
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
                is_dragging = false;
                dragged_node_index = 0;
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

                // Update dragged node position
                if (dragged_node_index > 0) {
                    lua_utils_set_node_number(L, dragged_node_index, "x", world_x - drag_offset_x);
                    lua_utils_set_node_number(L, dragged_node_index, "y", world_y - drag_offset_y);
                }
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

        // Render nodes
        int node_count = lua_utils_get_nodes_count(L);
        for (int i = 1; i <= node_count; i++) {
            float node_x = lua_utils_get_node_number(L, i, "x", 400.0f);
            float node_y = lua_utils_get_node_number(L, i, "y", 300.0f);
            float node_size = lua_utils_get_node_number(L, i, "size", 100.0f);
            float node_r = lua_utils_get_node_number(L, i, "r", 1.0f);
            float node_g = lua_utils_get_node_number(L, i, "g", 0.0f);
            float node_b = lua_utils_get_node_number(L, i, "b", 0.0f);
            const char* node_text = lua_utils_get_node_text(L, i, "");

            // Render square
            render_square(node_x, node_y, node_size, node_r, node_g, node_b, window, cam_x, cam_y, cam_scale);

            // Render node text (centered above square)
            if (node_text[0] != '\0') {
                int text_width, text_height;
                if (TTF_GetStringSize(font, node_text, strlen(node_text), &text_width, &text_height)) {// return bool true
                    float text_x = node_x - text_width / 2.0f; // Center horizontally
                    float text_y = node_y - node_size / 2.0f - text_height - 10.0f; // Above square
                    render_text(node_text, text_x, text_y, font, window, cam_x, cam_y, cam_scale);
                } else {
                    // global text and no node2d
                    // TTF_GetStringSize failed for global text 'Two Node2D Test':
                    // SDL_Log("TTF_GetStringSize failed for text '%s': %s", node_text, SDL_GetError());
                    // Fallback: approximate centering
                    float text_x = node_x - node_size / 4.0f;
                    float text_y = node_y - node_size / 2.0f - 20.0f;
                    render_text(node_text, text_x, text_y, font, window, cam_x, cam_y, cam_scale);
                }
            }
        }

        // Render global text at top-left (optionally centered)
        const char *text = lua_utils_get_string(L, "config", "text", "Hello, World!");
        if (text[0] != '\0') {
            int text_width, text_height;
            if (TTF_GetStringSize(font, text, strlen(text), &text_width, &text_height) == 0) {
                float text_x = 10.0f; // Keep at left edge
                float text_y = 10.0f + text_height; // Adjust for baseline
                render_text(text, text_x, text_y, font, window, cam_x, cam_y, cam_scale);
            } else {
                SDL_Log("TTF_GetStringSize failed for global text '%s': %s", text, SDL_GetError());
                render_text(text, 10.0f, 10.0f, font, window, cam_x, cam_y, cam_scale);
            }
        }

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