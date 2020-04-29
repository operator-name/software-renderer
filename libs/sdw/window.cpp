#include "sdw/window.h"

#include <iostream>

namespace sdw {

// Simple constructor method
window::window() {}

// Complex constructor method
window::window(int w, int h, bool fullscreen, std::string title) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    printMessageAndQuit("Could not initialise SDL: ", SDL_GetError());
  }

  width = w;
  height = h;
  pixelBuffer = new uint32_t[width * height];
  clearPixels();

  uint32_t flags = SDL_WINDOW_OPENGL;
  if (fullscreen)
    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
  int ANYWHERE = SDL_WINDOWPOS_UNDEFINED;
  _window =
      SDL_CreateWindow(title.c_str(), ANYWHERE, ANYWHERE, width, height, flags);
  if (_window == 0)
    printMessageAndQuit("Could not set video mode: ", SDL_GetError());

  flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
  renderer = SDL_CreateRenderer(_window, -1, flags);
  if (renderer == 0)
    printMessageAndQuit("Could not create renderer: ", SDL_GetError());

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
  SDL_RenderSetLogicalSize(renderer, width, height);

  int PIXELFORMAT = SDL_PIXELFORMAT_ARGB8888;
  texture = SDL_CreateTexture(renderer, PIXELFORMAT, SDL_TEXTUREACCESS_STATIC,
                              width, height);
  if (texture == 0)
    printMessageAndQuit("Could not allocate texture: ", SDL_GetError());
}

// Just close this window without quitting SDL
void window::close() {
  delete[] pixelBuffer;
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(_window);
}

// Deconstructor method
void window::destroy() {
  delete[] pixelBuffer;
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(_window);
  SDL_Quit();
}

void window::renderFrame() {
  SDL_UpdateTexture(texture, NULL, pixelBuffer, width * sizeof(uint32_t));
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

bool window::pollForInputEvents(SDL_Event *event) {
  if (SDL_PollEvent(event)) {
    if ((event->type == SDL_QUIT) || ((event->type == SDL_KEYDOWN) &&
                                      (event->key.keysym.sym == SDLK_ESCAPE))) {
      destroy();
      printMessageAndQuit("Exiting", NULL);
    }
    return true;
  }
  return false;
}

void window::setPixelColour(int x, int y, uint32_t colour) {
  if ((x < 0) || (x >= width) || (y < 0) || (y >= height)) {
    // std::cout << x << "," << y << " not on visible screen area" << std::endl;
  } else
    pixelBuffer[(y * width) + x] = colour;
}

uint32_t window::getPixelColour(int x, int y) {
  if ((x < 0) || (x >= width) || (y < 0) || (y >= height)) {
    // std::cout << x << "," << y << " not on visible screen area" << std::endl;
    return -1;
  } else
    return pixelBuffer[(y * width) + x];
}

void window::clearPixels() {
  memset(pixelBuffer, 0, width * height * sizeof(uint32_t));
}

void window::printMessageAndQuit(const char *message, const char *error) {
  if (error == NULL) {
    std::cout << message << std::endl;
    exit(0);
  } else {
    std::cout << message << " " << error << std::endl;
    exit(1);
  }
}

} // namespace sdw