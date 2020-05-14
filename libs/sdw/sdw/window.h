#pragma once
#include "glmt.hpp"

#include "SDL.h"
#include <string>

namespace sdw {

  class window {

  private:
    SDL_Window *_window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    uint32_t *pixelBuffer;
    float *depthBuffer;

  public:
    unsigned int height;
    unsigned int width;

    // Constructor method
    window();
    window(int w, int h, bool fullscreen,
           std::string title = "hybrid software rasteriser");
    void close();
    void destroy();
    void renderFrame();
    bool pollForInputEvents(SDL_Event *event);
    void setPixelColour(glmt::vec2p pos, const uint32_t colour);
    void setPixelColour(glmt::vec2p pos, float invz, const uint32_t colour);
    glmt::rgba8888 getPixelColour(glmt::vec2p pos);
    float getDepthBuffer(glmt::vec2p pos);
    void clearPixels();
    void clearDepthBuffer();

    void printMessageAndQuit(const char *message, const char *error);
  };

} // namespace sdw
