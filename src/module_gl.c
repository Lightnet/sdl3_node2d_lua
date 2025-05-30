#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <glad/gl.h>
#include "module_gl.h"

// Vertex shader source
const char *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
uniform mat4 projection;
void main() {
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

// Fragment shader source
const char *fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D textTexture;
void main() {
    vec4 texColor = texture(textTexture, TexCoord);
    if (texColor.a < 0.1) discard; // Discard low-alpha pixels
    FragColor = texColor;
}
)";

SDL_GLContext init_opengl(SDL_Window *window) {
    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    // Create OpenGL context
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        SDL_Log("SDL_GL_CreateContext failed: %s", SDL_GetError());
        return NULL;
    }

    // Initialize GLAD
    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        SDL_Log("Failed to initialize GLAD");
        SDL_GL_DestroyContext(gl_context);
        return NULL;
    }

    // Set up viewport
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    glViewport(0, 0, w, h);

    // Set clear color
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    return gl_context;
}

void render_text(TTF_Font *font, const char *text, SDL_Window *window) {
    // Render text to surface
    SDL_Color color = {255, 255, 255, 255}; // White text
    SDL_Surface *textSurface = TTF_RenderText_Blended(font, text, strlen(text), color);
    if (!textSurface) {
        SDL_Log("TTF_RenderText_Blended failed: %s", SDL_GetError());
        return;
    }

    // Log surface format for debugging
    SDL_Log("Surface format: %s", SDL_GetPixelFormatName(textSurface->format));
    SDL_Log("Surface dimensions: %dx%d", textSurface->w, textSurface->h);

    // Convert surface to RGBA32
    SDL_Surface *convertedSurface = SDL_ConvertSurface(textSurface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(textSurface); // Free original surface
    if (!convertedSurface) {
        SDL_Log("SDL_ConvertSurface failed: %s", SDL_GetError());
        return;
    }

    // Create OpenGL texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, convertedSurface->w, convertedSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, convertedSurface->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Get window size
    int win_width, win_height;
    SDL_GetWindowSize(window, &win_width, &win_height);

    // Create orthographic projection matrix
    float ortho[16] = {
        2.0f / win_width, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / win_height, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };

    // Compile shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        SDL_Log("Vertex shader compilation failed: %s", infoLog);
        SDL_DestroySurface(convertedSurface);
        glDeleteTextures(1, &texture);
        return;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        SDL_Log("Fragment shader compilation failed: %s", infoLog);
        glDeleteShader(vertexShader);
        SDL_DestroySurface(convertedSurface);
        glDeleteTextures(1, &texture);
        return;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        SDL_Log("Shader program linking failed: %s", infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        SDL_DestroySurface(convertedSurface);
        glDeleteTextures(1, &texture);
        return;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Set up vertex data
    float x = (win_width - convertedSurface->w) / 2.0f; // Center horizontally
    float y = (win_height - convertedSurface->h) / 2.0f; // Center vertically
    float vertices[] = {
        x, y, 0.0f, 0.0f, // Top-left
        x + convertedSurface->w, y, 1.0f, 0.0f, // Top-right
        x + convertedSurface->w, y + convertedSurface->h, 1.0f, 1.0f, // Bottom-right
        x, y + convertedSurface->h, 0.0f, 1.0f // Bottom-left
    };
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, ortho);
    glUniform1i(glGetUniformLocation(shaderProgram, "textTexture"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Clean up
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glDeleteTextures(1, &texture);
    SDL_DestroySurface(convertedSurface);
}