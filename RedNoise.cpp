#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <ModelTriangle.h>
#include <Utils.h>

#include "practical.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/io.hpp>

#include <cstdlib>
#include <fstream>

using namespace std;
using namespace glm;

#define WIDTH 320
#define HEIGHT 240

void setup();
void draw();
void update();
void handleEvent(SDL_Event event);

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);

struct State {
  CanvasTriangle triangle{CanvasPoint(0, 0), CanvasPoint(0, 0), CanvasPoint(0, 0)};
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

void setup() { std::srand(0); }

void draw() {
  // window.clearPixels();

  triangle(window, state.triangle);
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
    case SDLK_u:
      state.triangle.colour = Colour(glm::linearRand(0, 255), glm::linearRand(0, 255), glm::linearRand(0, 255));
      for (auto &vertex: state.triangle.vertices) {
        vertex.x = glm::linearRand(0, window.width-1);
        vertex.y = glm::linearRand(0, window.height-1);
      }
      break;
    }
    break;
  case SDL_MOUSEBUTTONDOWN:
    std::cout << "MOUSE CLICKED" << endl;
    break;
  }
}
