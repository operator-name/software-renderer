#include <sdw/CanvasTriangle.h>
#include <sdw/ModelTriangle.h>
#include <sdw/Utils.h>
#include <sdw/window.h>

#include "glmt.hpp"
#include "practical.hpp"

#include <glm/glm.hpp>

#include <glm/gtx/io.hpp>

#include <cstdlib>
#include <fstream>

#define WIDTH 320 * 2
#define HEIGHT 240 * 2

void setup();
void draw();
void update();
void handleEvent(SDL_Event event);

sdw::window window;

struct State {
  CanvasTriangle unfilled_triangle;
  CanvasTriangle filled_triangle;
  glmt::PPM ppm;
} state;

int main(int argc, char *argv[]) {
  setup();

  SDL_Event event;
  while (true) {
    // We MUST poll for events - otherwise the window will freeze !
    if (window.pollForInputEvents(&event))
      handleEvent(event);
    update();
    draw();
    // Need to render the frame at the end, or nothing actually gets shown on
    // the screen !
    window.renderFrame();
  }
}

void setup() {
  std::string path = "texture.ppm";
  std::ifstream texture(path.c_str());

  std::string x;

  texture >> state.ppm;
  if (texture.fail()) {
    std::cerr << "Parsing PPM \"" << path << "\" failed" << std::endl;
  }
  if (!texture.eof()) {
    std::clog << "Parsing PPM \"" << path
              << "\" has undefined characters after specificiation"
              << std::endl;
  }
  std::cout << state.ppm.header << std::endl;

  // exit(0);
  // seed random state to be the same each time (for debugging)
  // TODO: add proper random state
  std::srand(0);
  window = sdw::window(state.ppm.header.width, state.ppm.header.height, false);

  for (int h = 0; h < window.height; h++) {
    for (int w = 0; w < window.width; w++) {
      // u v coordiantes
      glm::vec3 c = state.ppm[glm::uvec2(w, h)];

      float red = c.r * 255;
      float green = c.g * 255;
      float blue = c.b * 255;

      uint32_t packed =
          (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
      window.setPixelColour(w, h, packed);
    }
  }
}

void draw() {
  // window.clearPixels();
}

void update() {
  // Function for performing animation (shifting artifacts or moving the camera)
}

void handleEvent(SDL_Event event) {
  switch (event.type) {
  case SDL_KEYDOWN:
    switch (event.key.keysym.sym) {
    case SDLK_LEFT:
      std::cout << "LEFT" << std::endl;
      break;
    case SDLK_RIGHT:
      std::cout << "RIGHT" << std::endl;
      break;
    case SDLK_UP:
      std::cout << "UP" << std::endl;
      break;
    case SDLK_DOWN:
      std::cout << "DOWN" << std::endl;
      break;
    case SDLK_c:
      window.clearPixels();
      for (int h = 0; h < window.height; h++) {
        for (int w = 0; w < window.width; w++) {
          float red = state.ppm[h][w].r * 255;
          float green = state.ppm[h][w].g * 255;
          float blue = state.ppm[h][w].b * 255;

          uint32_t packed =
              (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
          window.setPixelColour(w, h, packed);
        }
      }
      break;
    case SDLK_u:
      state.unfilled_triangle = randomtriangleinside(window);
      linetriangle(window, state.unfilled_triangle);
      break;
    case SDLK_f:
      state.filled_triangle = randomtriangleinside(window);
      filledtriangle(window, state.filled_triangle);
      break;
    }
    break;
  case SDL_MOUSEBUTTONDOWN:
    std::cout << "MOUSE CLICKED" << std::endl;
    break;
  }
}
