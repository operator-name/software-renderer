#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <ModelTriangle.h>
#include <Utils.h>

#include "practical.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>

#include <fstream>

using namespace std;
using namespace glm;

#define WIDTH 320
#define HEIGHT 240

void draw();
void update();
void handleEvent(SDL_Event event);

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);

void line(CanvasPoint start, CanvasPoint end/*, Color color*/) {
  glm::vec2 diff{end.x - start.x, end.y - start.y};
  float steps = glm::max(glm::abs(diff.x), glm::abs(diff.y));
  glm::vec2 stepsize = diff / steps;

  for (float i = 0; i < steps; i++) {
    
    float x = start.x + stepsize.x * i;
    float y = start.y + stepsize.y * i;

    int red = 255, green = 255, blue = 255;
    uint32_t colour = (255<<24) + (int(red)<<16) + (int(green)<<8) + int(blue);
    window.setPixelColour(glm::round(x), glm::round(y), colour);
  }
}

int main(int argc, char *argv[]) {
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

void draw() {
  window.clearPixels();
  auto r = glm::vec3(255, 0, 0);
  auto g = glm::vec3(0, 255, 0);
  auto b = glm::vec3(0, 0, 250);
  auto y = r + g;
  auto redtoyellow = interpolate(r, y, window.height);
  auto bluetogreen = interpolate(b, g, window.height);
  for (int y = 0; y < window.height; y++) {
    auto row = interpolate(redtoyellow[y], bluetogreen[y], window.width);
    for (int x = 0; x < window.width; x++) {
      float red = row[x][0];
      float green = row[x][1];
      float blue = row[x][2];
      uint32_t colour =
          (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
      window.setPixelColour(x, y, colour);
    }
  }

  CanvasPoint s(10, 10);
  CanvasPoint e(20, 40);
  line(s, e);
}

void update() {
  // Function for performing animation (shifting artifacts or moving the camera)
}

void handleEvent(SDL_Event event) {
  if (event.type == SDL_KEYDOWN) {
    if (event.key.keysym.sym == SDLK_LEFT)
      cout << "LEFT" << endl;
    else if (event.key.keysym.sym == SDLK_RIGHT)
      cout << "RIGHT" << endl;
    else if (event.key.keysym.sym == SDLK_UP)
      cout << "UP" << endl;
    else if (event.key.keysym.sym == SDLK_DOWN)
      cout << "DOWN" << endl;
  } else if (event.type == SDL_MOUSEBUTTONDOWN)
    cout << "MOUSE CLICKED" << endl;
}
