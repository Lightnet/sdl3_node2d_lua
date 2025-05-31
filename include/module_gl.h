#ifndef MODULE_GL_H
#define MODULE_GL_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

// Initialize OpenGL context
SDL_GLContext init_opengl_context(SDL_Window *window);

// Render text at position (x, y) in world space
void render_text(const char *text, float x, float y, TTF_Font *font, SDL_Window *window, float cam_x, float cam_y, float cam_scale);

// Render square centered at (x, y)
void render_square(float x, float y, float size, float r, float g, float b, SDL_Window *window, float cam_x, float cam_y, float cam_scale);

// Render circle centered at (x, y)
void render_circle(float x, float y, float radius, float r, float g, float b, SDL_Window *window, float cam_x, float cam_y, float cam_scale);

#endif // MODULE_GL_H