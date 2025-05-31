#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <glad/gl.h>
#include "module_gl.h"
#include <string.h>
#include <math.h>

// Vertex shader for square and circle (no texture coordinates)
const char *squareVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
uniform mat4 projection;
void main() {
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
}
)";

// Fragment shader for square and circle (solid color)
const char *squareFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 color;
void main() {
    FragColor = vec4(color, 1.0);
}
)";

// Vertex shader for text
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

// Fragment shader for text
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

void render_text(TTF_Font *font, const char *text, SDL_Window *window, float cam_x, float cam_y, float cam_scale) {
    // Render text to surface
    SDL_Color color = {255, 255, 255, 255}; // White text
    SDL_Surface *textSurface = TTF_RenderText_Blended(font, text, strlen(text), color);
    if (!textSurface) {
        SDL_Log("TTF_RenderText_Blended failed: %s", SDL_GetError());
        return;
    }

    // Convert surface to RGBA32
    SDL_Surface *convertedSurface = SDL_ConvertSurface(textSurface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(textSurface);
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

    // Create orthographic projection matrix with camera transform
    float ortho[16] = {
        2.0f * cam_scale / win_width, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f * cam_scale / win_height, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -cam_x * 2.0f * cam_scale / win_width - 1.0f, cam_y * 2.0f * cam_scale / win_height + 1.0f, 0.0f, 1.0f
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

    // Set up vertex data (top-left at (0, 10) in world space)
    float x = 0.0f;
    float y = 10.0f;
    float w = (float)convertedSurface->w;
    float h = (float)convertedSurface->h;
    float vertices[] = {
        x, y, 0.0f, 0.0f, // Top-left
        x + w, y, 1.0f, 0.0f, // Top-right
        x + w, y + h, 1.0f, 1.0f, // Bottom-right
        x, y + h, 0.0f, 1.0f // Bottom-left
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

void render_square(float x, float y, float size, float r, float g, float b, SDL_Window *window, float cam_x, float cam_y, float cam_scale) {
    // Get window size
    int win_width, win_height;
    SDL_GetWindowSize(window, &win_width, &win_height);

    // Create orthographic projection matrix with camera transform
    float ortho[16] = {
        2.0f * cam_scale / win_width, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f * cam_scale / win_height, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -cam_x * 2.0f * cam_scale / win_width - 1.0f, cam_y * 2.0f * cam_scale / win_height + 1.0f, 0.0f, 1.0f
    };

    // Compile shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &squareVertexShaderSource, NULL);
    glCompileShader(vertexShader);
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        SDL_Log("Square vertex shader compilation failed: %s", infoLog);
        return;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &squareFragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        SDL_Log("Square fragment shader compilation failed: %s", infoLog);
        glDeleteShader(vertexShader);
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
        SDL_Log("Square shader program linking failed: %s", infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Define square vertices (centered at x, y with given size)
    float half_size = size / 2.0f;
    float vertices[] = {
        x - half_size, y - half_size, // Bottom-left
        x + half_size, y - half_size, // Bottom-right
        x + half_size, y + half_size, // Top-right
        x - half_size, y + half_size  // Top-left
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

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, ortho);
    glUniform3f(glGetUniformLocation(shaderProgram, "color"), r, g, b);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Clean up
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
}

void render_circle(float x, float y, float radius, float r, float g, float b, SDL_Window *window, float cam_x, float cam_y, float cam_scale) {
    // Get window size
    int win_width, win_height;
    SDL_GetWindowSize(window, &win_width, &win_height);

    // Create orthographic projection matrix with camera transform
    float ortho[16] = {
        2.0f * cam_scale / win_width, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f * cam_scale / win_height, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -cam_x * 2.0f * cam_scale / win_width - 1.0f, cam_y * 2.0f * cam_scale / win_height + 1.0f, 0.0f, 1.0f
    };

    // Compile shaders (use same as square)
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &squareVertexShaderSource, NULL);
    glCompileShader(vertexShader);
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        SDL_Log("Circle vertex shader compilation failed: %s", infoLog);
        return;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &squareFragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        SDL_Log("Circle fragment shader compilation failed: %s", infoLog);
        glDeleteShader(vertexShader);
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
        SDL_Log("Circle shader program linking failed: %s", infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Define circle vertices (triangle fan)
    const int segments = 32;
    float vertices[(segments + 2) * 2];
    vertices[0] = x; // Center
    vertices[1] = y;
    for (int i = 0; i <= segments; ++i) {
        float angle = i * 2.0f * M_PI / segments;
        vertices[(i + 1) * 2 + 0] = x + radius * cosf(angle);
        vertices[(i + 1) * 2 + 1] = y + radius * sinf(angle);
    }

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, ortho);
    glUniform3f(glGetUniformLocation(shaderProgram, "color"), r, g, b);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);

    // Clean up
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
}