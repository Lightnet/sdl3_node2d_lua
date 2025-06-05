#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <glad/gl.h>
#include "module_gl.h"
#include "module_lua.h"
#include <math.h>
#include <stdbool.h>

int main(int argc, char *argv[]) {
    // Initialize SDL3
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    // Initialize SDL_ttf
    if (TTF_Init() == 0) {
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

    // Dragging, panning, and connection state
    bool is_dragging = false;
    bool is_panning = false;
    bool is_connecting = false;
    float drag_offset_x = 0.0f;
    float drag_offset_y = 0.0f;
    int dragged_node_index = 0; // 1-based index of dragged node
    float pan_start_x = 0.0f;
    float pan_start_y = 0.0f;
    int from_node = 0, from_output = 0; // Connection start
    float mouse_x = 0.0f, mouse_y = 0.0f; // For connection line
    int highlighted_node = 0, highlighted_connector = 0; // For hover
    const char *highlighted_type = "";

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
                mouse_x = event.button.x;
                mouse_y = event.button.y;
                float world_x = mouse_x / cam_scale + cam_x;
                float world_y = mouse_y / cam_scale + cam_y;

                // Check for connector click (for connection)
                int node_count = lua_utils_get_nodes_count(L);
                bool connector_clicked = false;
                for (int i = 1; i <= node_count && !connector_clicked; i++) {
                    float node_x = lua_utils_get_node_number(L, i, "x", 400.0f);
                    float node_y = lua_utils_get_node_number(L, i, "y", 300.0f);
                    float node_size = lua_utils_get_node_number(L, i, "size", 100.0f);
                    int inputs = lua_utils_get_node_connectors(L, i, "inputs", 0);
                    int outputs = lua_utils_get_node_connectors(L, i, "outputs", 0);
                    float half_size = node_size / 2.0f;
                    float connector_spacing = 20.0f;
                    const float detect_radius = 15.0f;

                    // Check outputs
                    for (int j = 0; j < outputs && !connector_clicked; j++) {
                        float conn_y = node_y + (j * connector_spacing) - (outputs - 1) * connector_spacing / 2.0f;
                        float conn_x = node_x + half_size;
                        float dx = world_x - conn_x;
                        float dy = world_y - conn_y;
                        if (sqrtf(dx * dx + dy * dy) <= detect_radius) {
                            is_connecting = true;
                            from_node = i;
                            from_output = j + 1;
                            connector_clicked = true;
                            SDL_Log("Connection started: from_node=%d, from_output=%d at (%.1f, %.1f)", i, j+1, conn_x, conn_y);
                        }
                    }

                    // Check inputs (only if not starting a connection)
                    if (!is_connecting) {
                        for (int j = 0; j < inputs && !connector_clicked; j++) {
                            float conn_y = node_y + (j * connector_spacing) - (inputs - 1) * connector_spacing / 2.0f;
                            float conn_x = node_x - half_size;
                            float dx = world_x - conn_x;
                            float dy = world_y - conn_y;
                            if (sqrtf(dx * dx + dy * dy) <= detect_radius) {
                                // Not used for starting connection
                                connector_clicked = true;
                            }
                        }
                    }
                }

                // Check nodes for dragging (if no connector clicked)
                if (!connector_clicked) {
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
                            break;
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
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
                if (is_connecting) {
                    // Check for input connector to complete connection
                    float cam_x = lua_utils_get_number(L, "config", "camera.x", 0.0f);
                    float cam_y = lua_utils_get_number(L, "config", "camera.y", 0.0f);
                    float cam_scale = lua_utils_get_number(L, "config", "camera.scale", 1.0f);
                    int win_width, win_height;
                    SDL_GetWindowSize(window, &win_width, &win_height);
                    mouse_x = event.button.x;
                    mouse_y = event.button.y;
                    float world_x = mouse_x / cam_scale + cam_x;
                    float world_y = mouse_y / cam_scale + cam_y;

                    int node_count = lua_utils_get_nodes_count(L);
                    bool connected = false;
                    for (int i = 1; i <= node_count && !connected; i++) {
                        float node_x = lua_utils_get_node_number(L, i, "x", 400.0f);
                        float node_y = lua_utils_get_node_number(L, i, "y", 300.0f);
                        float node_size = lua_utils_get_node_number(L, i, "size", 100.0f);
                        int inputs = lua_utils_get_node_connectors(L, i, "inputs", 0);
                        float half_size = node_size / 2.0f;
                        float connector_spacing = 20.0f;
                        const float detect_radius = 15.0f;

                        for (int j = 0; j < inputs && !connected; j++) {
                            float conn_y = node_y + (j * connector_spacing) - (inputs - 1) * connector_spacing / 2.0f;
                            float conn_x = node_x - half_size;
                            float dx = world_x - conn_x;
                            float dy = world_y - conn_y;
                            if (sqrtf(dx * dx + dy * dy) <= detect_radius && i != from_node) {
                                lua_utils_add_connection(L, from_node, from_output, i, j + 1);
                                SDL_Log("Connection created: from_node=%d, from_output=%d to node=%d, to_input=%d", from_node, from_output, i, j+1);
                                connected = true;
                            }
                        }
                    }
                    is_connecting = false;
                    from_node = from_output = 0;
                }
                is_dragging = false;
                dragged_node_index = 0;
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_RIGHT) {
                // Remove connections near clicked connector
                float cam_x = lua_utils_get_number(L, "config", "camera.x", 0.0f);
                float cam_y = lua_utils_get_number(L, "config", "camera.y", 0.0f);
                float cam_scale = lua_utils_get_number(L, "config", "camera.scale", 1.0f);
                int win_width, win_height;
                SDL_GetWindowSize(window, &win_width, &win_height);
                mouse_x = event.button.x;
                mouse_y = event.button.y;
                float world_x = mouse_x / cam_scale + cam_x;
                float world_y = mouse_y / cam_scale + cam_y;

                int node_count = lua_utils_get_nodes_count(L);
                for (int i = 1; i <= node_count; i++) {
                    float node_x = lua_utils_get_node_number(L, i, "x", 400.0f);
                    float node_y = lua_utils_get_node_number(L, i, "y", 300.0f);
                    float node_size = lua_utils_get_node_number(L, i, "size", 100.0f);
                    int inputs = lua_utils_get_node_connectors(L, i, "inputs", 0);
                    int outputs = lua_utils_get_node_connectors(L, i, "outputs", 0);
                    float half_size = node_size / 2.0f;
                    float connector_spacing = 20.0f;
                    const float detect_radius = 15.0f;

                    // Check inputs
                    for (int j = 0; j < inputs; j++) {
                        float conn_y = node_y + (j * connector_spacing) - (inputs - 1) * connector_spacing / 2.0f;
                        float conn_x = node_x - half_size;
                        float dx = world_x - conn_x;
                        float dy = world_y - conn_y;
                        if (sqrtf(dx * dx + dy * dy) <= detect_radius) {
                            lua_utils_remove_connections(L, i, "input", j + 1);
                            SDL_Log("Removed connections for node=%d, input=%d at (%.1f, %.1f)", i, j+1, conn_x, conn_y);
                        }
                    }

                    // Check outputs
                    for (int j = 0; j < outputs; j++) {
                        float conn_y = node_y + (j * connector_spacing) - (outputs - 1) * connector_spacing / 2.0f;
                        float conn_x = node_x + half_size;
                        float dx = world_x - conn_x;
                        float dy = world_y - conn_y;
                        if (sqrtf(dx * dx + dy * dy) <= detect_radius) {
                            lua_utils_remove_connections(L, i, "output", j + 1);
                            SDL_Log("Removed connections for node=%d, output=%d at (%.1f, %.1f)", i, j+1, conn_x, conn_y);
                        }
                    }
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_MIDDLE) {
                is_panning = true;
                pan_start_x = event.button.x;
                pan_start_y = event.button.y;
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_MIDDLE) {
                is_panning = false;
            }
            else if (event.type == SDL_EVENT_MOUSE_MOTION) {
                // Update mouse position
                float cam_x = lua_utils_get_number(L, "config", "camera.x", 0.0f);
                float cam_y = lua_utils_get_number(L, "config", "camera.y", 0.0f);
                float cam_scale = lua_utils_get_number(L, "config", "camera.scale", 1.0f);
                int win_width, win_height;
                SDL_GetWindowSize(window, &win_width, &win_height);
                mouse_x = event.motion.x;
                mouse_y = event.motion.y;
                float world_x = mouse_x / cam_scale + cam_x;
                float world_y = mouse_y / cam_scale + cam_y;

                // Check for connector hover
                highlighted_node = 0;
                highlighted_connector = 0;
                highlighted_type = "";
                int node_count = lua_utils_get_nodes_count(L);
                for (int i = 1; i <= node_count && !highlighted_node; i++) {
                    float node_x = lua_utils_get_node_number(L, i, "x", 400.0f);
                    float node_y = lua_utils_get_node_number(L, i, "y", 300.0f);
                    float node_size = lua_utils_get_node_number(L, i, "size", 100.0f);
                    int inputs = lua_utils_get_node_connectors(L, i, "inputs", 0);
                    int outputs = lua_utils_get_node_connectors(L, i, "outputs", 0);
                    float half_size = node_size / 2.0f;
                    float connector_spacing = 20.0f;
                    const float detect_radius = 15.0f;

                    // Check inputs
                    for (int j = 0; j < inputs && !highlighted_node; j++) {
                        float conn_y = node_y + (j * connector_spacing) - (inputs - 1) * connector_spacing / 2.0f;
                        float conn_x = node_x - half_size;
                        float dx = world_x - conn_x;
                        float dy = world_y - conn_y;
                        if (sqrtf(dx * dx + dy * dy) <= detect_radius) {
                            highlighted_node = i;
                            highlighted_connector = j + 1;
                            highlighted_type = "input";
                        }
                    }

                    // Check outputs
                    for (int j = 0; j < outputs && !highlighted_node; j++) {
                        float conn_y = node_y + (j * connector_spacing) - (outputs - 1) * connector_spacing / 2.0f;
                        float conn_x = node_x + half_size;
                        float dx = world_x - conn_x;
                        float dy = world_y - conn_y;
                        if (sqrtf(dx * dx + dy * dy) <= detect_radius) {
                            highlighted_node = i;
                            highlighted_connector = j + 1;
                            highlighted_type = "output";
                        }
                    }
                }

                // Handle dragging
                if (is_dragging) {
                    world_x = mouse_x / cam_scale + cam_x;
                    world_y = mouse_y / cam_scale + cam_y;
                    if (dragged_node_index > 0) {
                        lua_utils_set_node_number(L, dragged_node_index, "x", world_x - drag_offset_x);
                        lua_utils_set_node_number(L, dragged_node_index, "y", world_y - drag_offset_y);
                    }
                }
                // Handle panning
                else if (is_panning) {
                    float delta_x = (event.motion.x - pan_start_x) / cam_scale;
                    float delta_y = (event.motion.y - pan_start_y) / cam_scale;
                    lua_utils_set_number(L, "config", "camera.x", cam_x - delta_x);
                    lua_utils_set_number(L, "config", "camera.y", cam_y - delta_y);
                    pan_start_x = event.motion.x;
                    pan_start_y = event.motion.y;
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                // Get mouse position and current camera properties
                int win_width, win_height;
                SDL_GetWindowSize(window, &win_width, &win_height);
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

        // Render connections (before nodes for layering)
        int conn_count = lua_utils_get_connections_count(L);
        for (int i = 1; i <= conn_count; i++) {
            int from_node, from_output, to_node, to_input;
            lua_utils_get_connection(L, i, &from_node, &from_output, &to_node, &to_input);
            if (from_node > 0 && to_node > 0) {
                float from_x = lua_utils_get_node_number(L, from_node, "x", 400.0f);
                float from_y = lua_utils_get_node_number(L, from_node, "y", 300.0f);
                float from_size = lua_utils_get_node_number(L, from_node, "size", 100.0f);
                int from_outputs = lua_utils_get_node_connectors(L, from_node, "outputs", 0);
                float to_x = lua_utils_get_node_number(L, to_node, "x", 400.0f);
                float to_y = lua_utils_get_node_number(L, to_node, "y", 300.0f);
                float to_size = lua_utils_get_node_number(L, to_node, "size", 100.0f);
                int to_inputs = lua_utils_get_node_connectors(L, to_node, "inputs", 0);
                float from_half = from_size / 2.0f;
                float to_half = to_size / 2.0f;
                float connector_spacing = 20.0f;
                float x1 = from_x + from_half;
                float y1 = from_y + (from_output - 1) * connector_spacing - (from_outputs - 1) * connector_spacing / 2.0f;
                float x2 = to_x - to_half;
                float y2 = to_y + (to_input - 1) * connector_spacing - (to_inputs - 1) * connector_spacing / 2.0f;
                render_line(x1, y1, x2, y2, 1.0f, 0.0f, 1.0f, window, cam_x, cam_y, cam_scale);
            }
        }

        // Render temporary connection line
        if (is_connecting) {
            float from_x = lua_utils_get_node_number(L, from_node, "x", 400.0f);
            float from_y = lua_utils_get_node_number(L, from_node, "y", 300.0f);
            float from_size = lua_utils_get_node_number(L, from_node, "size", 100.0f);
            int from_outputs = lua_utils_get_node_connectors(L, from_node, "outputs", 0);
            float from_half = from_size / 2.0f;
            float connector_spacing = 20.0f;
            float x1 = from_x + from_half;
            float y1 = from_y + (from_output - 1) * connector_spacing - (from_outputs - 1) * connector_spacing / 2.0f;
            float x2 = mouse_x / cam_scale + cam_x;
            float y2 = mouse_y / cam_scale + cam_y;
            render_line(x1, y1, x2, y2, 1.0f, 0.0f, 1.0f, window, cam_x, cam_y, cam_scale);
        }

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
            int inputs = lua_utils_get_node_connectors(L, i, "inputs", 0);
            int outputs = lua_utils_get_node_connectors(L, i, "outputs", 0);

            // Render square
            render_square(node_x, node_y, node_size, node_r, node_g, node_b, window, cam_x, cam_y, cam_scale);

            // Render connectors
            float half_size = node_size / 2.0f;
            float connector_spacing = 20.0f;
            float connector_radius = 10.0f;
            for (int j = 0; j < inputs; j++) {
                float conn_y = node_y + (j * connector_spacing) - (inputs - 1) * connector_spacing / 2.0f;
                float conn_x = node_x - half_size;
                float r = 0.0f, g = 1.0f, b = 0.0f;
                float radius = connector_radius;
                if (highlighted_node == i && highlighted_connector == j+1 && strcmp(highlighted_type, "input") == 0) {
                    radius *= 1.2f; // Highlight by increasing size
                }
                render_circle(conn_x, conn_y, radius, r, g, b, window, cam_x, cam_y, cam_scale);
                SDL_Log("Node %d input %d at (%.1f, %.1f)", i, j+1, conn_x, conn_y);
            }
            for (int j = 0; j < outputs; j++) {
                float conn_y = node_y + (j * connector_spacing) - (outputs - 1) * connector_spacing / 2.0f;
                float conn_x = node_x + half_size;
                float r = 1.0f, g = 1.0f, b = 0.0f;
                float radius = connector_radius;
                if (highlighted_node == i && highlighted_connector == j+1 && strcmp(highlighted_type, "output") == 0) {
                    radius *= 1.2f; // Highlight
                }
                if (is_connecting && from_node == i && from_output == j+1) {
                    radius *= 1.2f; // Highlight during connection
                }
                render_circle(conn_x, conn_y, radius, r, g, b, window, cam_x, cam_y, cam_scale);
                SDL_Log("Node %d output %d at (%.1f, %.1f)", i, j+1, conn_x, conn_y);
            }

            // Render node text
            if (node_text[0] != '\0') {
                int text_width, text_height;
                if (TTF_GetStringSize(font, node_text, strlen(node_text), &text_width, &text_height)) {
                    float text_x = node_x - text_width / 2.0f;
                    float text_y = node_y - node_size / 2.0f - text_height - 10.0f;
                    render_text(node_text, text_x, text_y, font, window, cam_x, cam_y, cam_scale);
                    SDL_Log("Node %d text='%s', width=%d, height=%d, pos=(%.1f, %.1f)", i, node_text, text_width, text_height, text_x, text_y);
                } else {
                    SDL_Log("TTF_GetStringSize failed for text '%s': %s", node_text, SDL_GetError());
                    float text_x = node_x - node_size / 4.0f;
                    float text_y = node_y - node_size / 2.0f - 20.0f;
                    render_text(node_text, text_x, text_y, font, window, cam_x, cam_y, cam_scale);
                    SDL_Log("Node %d fallback text='%s', pos=(%.1f, %.1f)", i, node_text, text_x, text_y);
                }
            }
        }

        // Render global text
        const char *text = lua_utils_get_string(L, "config", "text", "Hello, World!");
        if (text[0] != '\0') {
            int text_width, text_height;
            if (TTF_GetStringSize(font, text, strlen(text), &text_width, &text_height)) {
                float text_x = 10.0f;
                float text_y = 10.0f + text_height;
                render_text(text, text_x, text_y, font, window, cam_x, cam_y, cam_scale);
                SDL_Log("Global text='%s', width=%d, height=%d, pos=(%.1f, %.1f)", text, text_width, text_height, text_x, text_y);
            } else {
                SDL_Log("TTF_GetStringSize failed for global text '%s': %s", text, SDL_GetError());
                render_text(text, 10.0f, 10.0f, font, window, cam_x, cam_y, cam_scale);
                SDL_Log("Global fallback text='%s', pos=(%.1f, %.1f)", text, 10.0f, 10.0f);
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