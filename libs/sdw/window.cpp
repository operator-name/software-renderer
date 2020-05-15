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
    depthBuffer = new float[width * height];
    clearPixels();
    clearDepthBuffer();

    uint32_t flags = SDL_WINDOW_OPENGL;
    if (fullscreen)
      flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    int ANYWHERE = SDL_WINDOWPOS_UNDEFINED;
    _window = SDL_CreateWindow(title.c_str(), ANYWHERE, ANYWHERE, width, height,
                               flags);
    if (_window == 0)
      printMessageAndQuit("Could not set video mode: ", SDL_GetError());

    flags = SDL_RENDERER_ACCELERATED;
    // vsync mouse lag:
    // https://stackoverflow.com/questions/25173495/c-sdl2-get-mouse-coordinates-without-delay
    // flags |= SDL_RENDERER_PRESENTVSYNC;

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
  void window::close() { SDL_DestroyWindow(_window); }

  // Deconstructor method
  void window::destroy() {
    delete[] pixelBuffer;
    delete[] depthBuffer;
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
      if ((event->type == SDL_QUIT) ||
          ((event->type == SDL_KEYDOWN) &&
           (event->key.keysym.sym == SDLK_ESCAPE))) {
        destroy();
        printMessageAndQuit("Exiting", NULL);
      }
      return true;
    }
    return false;
  }

  void window::setPixelColour(glmt::vec2p pos, uint32_t colour) {
    if ((pos.x < 0) || (pos.x >= width) || (pos.y < 0) || (pos.y >= height)) {
      // std::cout << x << "," << y << " not on visible screen area" <<
      // std::endl;
    } else {
      pixelBuffer[(pos.y * width) + pos.x] = colour;
    }
  }

  void window::setPixelColour(glmt::vec2p pos, float invz,
                              const uint32_t colour) {
    if ((pos.x < 0) || (pos.x >= width) || (pos.y < 0) || (pos.y >= height)) {
      // std::cout << x << "," << y << " not on visible screen area" <<
      // std::endl;
    } else {
      if (0.0000001f >=
          depthBuffer[(pos.y * width) + pos.x] - invz) { // threshold
        depthBuffer[(pos.y * width) + pos.x] = invz;
        setPixelColour(pos, colour);
      }
    }
  }

  glmt::rgba8888 window::getPixelColour(glmt::vec2p pos) {
    if ((pos.x < 0) || (pos.x >= width) || (pos.y < 0) || (pos.y >= height)) {
      // std::cout << x << "," << y << " not on visible screen area" <<
      // std::endl;
      return glmt::rgba8888::fromargb8888packed(
          -1); // TODO: return maybe? add semantics to vec2s?
    } else {
      return glmt::rgba8888::fromargb8888packed(
          pixelBuffer[(pos.y * width) + pos.x]);
    }
  }

  float window::getDepthBuffer(glmt::vec2p pos) {
    if ((pos.x < 0) || (pos.x >= width) || (pos.y < 0) || (pos.y >= height)) {
      return 1;
    } else {
      return depthBuffer[(pos.y * width) + pos.x];
    }
  }

  void window::clearPixels() {
    memset(pixelBuffer, 0, width * height * sizeof(uint32_t));
  }

  void window::clearDepthBuffer() {
    for (size_t i = 0; i < width * height; i++) {
      // captures 1/z, instead of z as suggested so
      // std::numeric_limits<float>::infinity() is not used nothing on screen is
      // the same as every point being infinity far away 1/inf = 0,
      depthBuffer[i] = 0;
    }
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