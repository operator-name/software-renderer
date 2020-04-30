#include <sdw/window.h>

#include "glmt.hpp"
#include "practical.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <glm/gtx/io.hpp>

#include <cstdlib>

#define N 2
#define WIDTH (320 * N)
#define HEIGHT (240 * N)

void setup();
void draw();
void update();
void handleEvent(SDL_Event event);

sdw::window window;

float scale = 50;
float x = 300;
float y = 100;

// strictly for what is supported for rendering, whereas glmt::OBJ may include
// more data that the renderer does not support
struct Model {
  typedef std::array<glmt::vec3l, 3> triangle_points;
  typedef std::tuple<triangle_points, glmt::rgbf01> triangle;
  std::vector<triangle> triangles;
  glm::mat4 matrix; // model matrix, TODO: proper types
};

struct State {
  std::tuple<std::array<glmt::vec2s, 3>, glmt::rgb888> unfilled_triangle;
  std::tuple<std::array<glmt::vec2s, 3>, glmt::rgb888> filled_triangle;

  glmt::OBJ obj;
  Model model;

  glm::mat4 view;
  glm::mat4 projection;

  struct SDL_detail {
    bool mouse_down = false;
  } sdl;
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
  state.obj = parse_obj("cornell-box.obj");
  state.model.triangles = state.obj.triangles;

  // seed random state to be the same each time (for debugging)
  // TODO: add proper random state
  std::srand(0);
  window = sdw::window(WIDTH, HEIGHT, false);
}

void draw() {
  // window.clearPixels();
  for (auto const &t : state.model.triangles) {
    std::array<glmt::vec2s, 3> ft;
    auto c = std::get<1>(t);

    for (size_t i = 0; i < ft.size(); i++) {
      ft[i] = glm::vec2(state.model.matrix * std::get<0>(t)[i]);
    }

    // filledtriangle(window, std::make_tuple(ft, c));
    linetriangle(window, std::make_tuple(ft, c));
  }
}

void update() {
  // Function for performing animation (shifting artifacts or moving the camera)
  state.model.matrix = glm::translate(glm::vec3(x, y, 0)) *
                       glm::scale(glm::vec3(scale, scale, scale));
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
    case SDLK_b: {

    } break;
    case SDLK_u:
      state.unfilled_triangle = randomtriangleinside(window);
      linetriangle(window, state.unfilled_triangle);
      break;
    case SDLK_f:
      state.filled_triangle = randomtriangleinside(window);
      filledtriangle(window, state.filled_triangle);
      break;
    case SDLK_x:
      std::cout << "scale = " << scale << ", x = " << x << ", y = " << y
                << std::endl;
      std::cin >> scale >> x >> y;
      std::cout << "scale = " << scale << ", x = " << x << ", y = " << y
                << std::endl;
    }
    break;
  case SDL_MOUSEBUTTONDOWN:
    state.sdl.mouse_down = true;
    std::cout << "MOUSE DOWN { x = " << event.button.x
              << ", y = " << event.button.y << "}" << std::endl;
    break;
  case SDL_MOUSEBUTTONUP:
    state.sdl.mouse_down = false;
    std::cout << "MOUSE UP { x = " << event.button.x
              << ", y = " << event.button.y << "}" << std::endl;
    break;
  case SDL_MOUSEMOTION:
    if (state.sdl.mouse_down) {
      glmt::rgb888 colour(glm::linearRand(0, 255), glm::linearRand(0, 255),
                          glm::linearRand(0, 255));

      glmt::vec2s start{event.motion.x, event.motion.y};
      glmt::vec2s rel{event.motion.xrel, event.motion.yrel};
      glmt::vec2s end = start + rel;

      line(window, start, end, colour);
    }

    break;
  }
}
