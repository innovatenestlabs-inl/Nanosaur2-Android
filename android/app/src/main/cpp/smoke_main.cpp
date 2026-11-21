// Milestone 2A: demonstrate Android asset I/O + original Nanosaur 2 JPEG
// decoding + a textured OpenGL ES 3.0 draw. This is NOT the full game renderer.
// Nanosaur 2 copyright belongs to its original rights holders.
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <GLES3/gl3.h>
#include <cstddef>

extern bool InitializePommeResources(bool retainPomme);

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "stb_image.h"

namespace
{
constexpr const char* kAsset = "Sprites/maps/level1.jpg";
constexpr const char* kVertex = R"(#version 300 es
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;
out vec2 vTexCoord;
void main() {
    gl_Position = vec4(aPosition, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
)";
constexpr const char* kFragment = R"(#version 300 es
precision mediump float;
in vec2 vTexCoord;
uniform sampler2D uTexture;
out vec4 fragColor;
void main() {
    fragColor = texture(uTexture, vTexCoord);
}
)";

GLuint Compile(GLenum kind, const char* source)
{
    GLuint shader = glCreateShader(kind);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok != GL_TRUE)
    {
        char error[2048] = {};
        glGetShaderInfoLog(shader, sizeof(error), nullptr, error);
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Shader error: %s", error);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint MakeProgram()
{
    GLuint vertex = Compile(GL_VERTEX_SHADER, kVertex);
    GLuint fragment = Compile(GL_FRAGMENT_SHADER, kFragment);
    if (!vertex || !fragment)
    {
        if (vertex) glDeleteShader(vertex);
        if (fragment) glDeleteShader(fragment);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (ok != GL_TRUE)
    {
        char error[2048] = {};
        glGetProgramInfoLog(program, sizeof(error), nullptr, error);
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Shader link error: %s", error);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

GLuint LoadGameTexture(int& imageWidth, int& imageHeight)
{
    size_t bytes = 0;
    void* contents = SDL_LoadFile(kAsset, &bytes);
    if (!contents)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Couldn't open bundled Nanosaur asset '%s': %s",
                     kAsset, SDL_GetError());
        return 0;
    }

    int channels = 0;
    unsigned char* pixels = stbi_load_from_memory(
        static_cast<const stbi_uc*>(contents), static_cast<int>(bytes),
        &imageWidth, &imageHeight, &channels, 4);
    SDL_free(contents);
    if (!pixels)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Original game JPEG decoding failed: %s", stbi_failure_reason());
        return 0;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, imageWidth, imageHeight,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    stbi_image_free(pixels);
    SDL_Log("Loaded original Nanosaur image '%s' (%d x %d)",
            kAsset, imageWidth, imageHeight);
    return texture;
}
}

int main(int, char**)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init: %s", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window* window = SDL_CreateWindow("Nanosaur 2 Asset Preview",
                                         1280, 720,
                                         SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
    if (!window)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Window: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GLES: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Extract the original Data tree when fullGameAssets is enabled, and
    // verify Pomme can resolve the resulting Macintosh FSSpec paths.
    // The original gameplay engine is not invoked yet.
    const bool pommeReady = InitializePommeResources(false);
    SDL_Log("Nanosaur Android Pomme filesystem probe: %s", pommeReady ? "READY" : "PREVIEW ONLY / FAILED");

    GLuint program = MakeProgram();
    int imageWidth = 0, imageHeight = 0;
    GLuint texture = LoadGameTexture(imageWidth, imageHeight);
    if (!program || !texture)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Asset preview initialization failed; see earlier log messages.");
        if (program) glDeleteProgram(program);
        if (texture) glDeleteTextures(1, &texture);
        SDL_GL_DestroyContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "uTexture"), 0);
    SDL_GL_SetSwapInterval(1);
    SDL_Log("Nanosaur Android original-asset preview started");

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_KEY_DOWN &&
                 event.key.key == SDLK_ESCAPE))
                running = false;
        }

        int w = 0, h = 0;
        SDL_GetWindowSizeInPixels(window, &w, &h);
        if (w <= 0 || h <= 0)
        {
            SDL_Delay(16);
            continue;
        }

        const float screenAspect = static_cast<float>(w) / static_cast<float>(h);
        const float imageAspect =
            static_cast<float>(imageWidth) / static_cast<float>(imageHeight);
        const float sx = imageAspect >= screenAspect ? 1.0f : imageAspect / screenAspect;
        const float sy = imageAspect >= screenAspect ? screenAspect / imageAspect : 1.0f;

        // Image data's first row is the top row. OpenGL treats that as v=0.
        const float vertices[] = {
            -sx, -sy, 0, 1,
             sx, -sy, 1, 1,
            -sx,  sy, 0, 0,
             sx,  sy, 1, 0,
        };

        glViewport(0, 0, w, h);
        glClearColor(0.02f, 0.26f, 0.28f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        SDL_GL_SwapWindow(window);
        SDL_Delay(16);
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(1, &texture);
    glDeleteProgram(program);
    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
