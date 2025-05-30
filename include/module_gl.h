#ifndef MODULE_GL_H
#define MODULE_GL_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

SDL_GLContext init_opengl(SDL_Window *window);
void render_text(TTF_Font *font, const char *text, SDL_Window *window);

#endif