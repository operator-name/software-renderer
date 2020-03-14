#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <ModelTriangle.h>
#include <Utils.h>

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

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);

struct State {
  CanvasTriangle unfilled_triangle;
  CanvasTriangle filled_triangle;
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
  // seed random state to be the same each time (for debugging)
  std::srand(0);
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
