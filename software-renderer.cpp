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

#define N 2
#define WIDTH (320 * N)
#define HEIGHT (240 * N)

void setup();
void draw();
void update();
void handleEvent(SDL_Event event);

sdw::window window;

struct State {
  CanvasTriangle unfilled_triangle;
  CanvasTriangle filled_triangle;

  struct ModelTriangle {
    CanvasTriangle triangle;
    CanvasTriangle texture;
    glmt::PPM ppm;
  } modeltriangle;
  glmt::PPM ppm;
} state;

// assumes trangle.vertices[1].y == trangle.vertices[2].y
void texturedtriangleflat(sdw::window window, ModelTriangle model) {}

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
  texture >> state.ppm;
  if (texture.fail()) {
    std::cerr << "Parsing PPM \"" << path << "\" failed" << std::endl;
  }

  state.modeltriangle.ppm = state.ppm;
  state.modeltriangle.triangle = CanvasTriangle(
      glm::vec2(160, 10), glm::vec2(300, 230), glm::vec2(10, 150));
  state.modeltriangle.texture = CanvasTriangle(
      glm::vec2(195, 5), glm::vec2(395, 380), glm::vec2(65, 330));

  // seed random state to be the same each time (for debugging)
  // TODO: add proper random state
  std::srand(0);
  window = sdw::window(WIDTH, HEIGHT, true);
}

void draw() {
  // window.clearPixels();
  // linetriangle(window, state.modeltriangle.triangle);
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
      break;
    case SDLK_t:
      for (int h = 0; h < window.height; h++) {
        for (int w = 0; w < window.width; w++) {
          // PPM::operator[] is GL_REPEAT by default
          glm::ivec3 c = state.ppm[glm::ivec2(w, h)] * 255.0f;
          float red = c.r;
          float green = c.g;
          float blue = c.b;

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
