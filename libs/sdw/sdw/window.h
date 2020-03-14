#pragma once
#include "SDL.h"

namespace sdw {

class window {

private:
  SDL_Window *_window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  uint32_t *pixelBuffer;

public:
  int height;
  int width;

  // Constructor method
  window();
  window(int w, int h, bool fullscreen);
  void destroy();
  void renderFrame();
  bool pollForInputEvents(SDL_Event *event);
  void setPixelColour(int x, int y, uint32_t colour);
  uint32_t getPixelColour(int x, int y);
  void clearPixels();

  void printMessageAndQuit(const char *message, const char *error);
};

} // namespace sdw
