#include <sdw/window.h>

#include "glmt.hpp"
#include "practical.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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

float scale = 1;
float x = 0;
float y = 0;
float z = 0;

// strictly for what is supported for rendering, whereas glmt::OBJ may include
// more data that the renderer does not support
struct Model {
  typedef std::array<glmt::vec3l, 3> triangle_points;
  typedef std::tuple<triangle_points, glmt::rgbf01> triangle;
  std::vector<triangle> triangles;

  glm::mat4 matrix; // model matrix, TODO: proper types
  glm::vec3 centre;
};

struct Camera {
  glmt::vec3w target = glm::vec4(0, 0, 0, 1);

  float dist = -10.2f;
  float pitch = 0;
  float yaw = 0.001;
  float rot_velocity = 0.0002f;
  float velocity = 0.003f;

  // "look at" but with orbit style configuration
  glm::mat4 view() {
    glm::vec4 pos = glm::vec4(0, 0, dist, 0) + target;
    glm::mat4 rot = glm::rotate(yaw, glm::vec3(0, 1, 0)) *
                    glm::rotate(pitch, glm::vec3(1, 0, 0));
    return glm::translate(-glm::vec3(pos)) * rot;
  }
};

struct State {
  std::tuple<std::array<glmt::vec2s, 3>, glmt::rgb888> unfilled_triangle;
  std::tuple<std::array<glmt::vec2s, 3>, glmt::rgb888> filled_triangle;

  glmt::OBJ obj;
  Model model;

  Camera camera;
  glm::mat4 view = glm::lookAt(glm::vec3(0.1, 0.01, -10), glm::vec3(0, 0, 0),
                               glm::vec3(0, 1, 0));
  glm::mat4 proj = glm::perspectiveFov(glm::radians(90.f), (float)WIDTH,
                                       (float)HEIGHT, 0.1f, 100.0f);

  struct SDL_detail {
    bool mouse_down = false;
  } sdl;

  unsigned int frame = 0;
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
  glm::vec3 max(std::numeric_limits<float>::lowest());
  glm::vec3 min(std::numeric_limits<float>::max());
  for (auto const &t : state.model.triangles) {
    for (glmt::vec3l point : std::get<0>(t)) {
      max = glm::max(max, glm::vec3(point));
      min = glm::min(min, glm::vec3(point));
    }
  }
  state.model.centre = (min + max) / 2.f;

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
      glmt::vec3l ls = std::get<0>(t)[i];
      // manually apply stuff
      // glmt::vec3w ws = state.model.matrix * ls;
      // glmt::vec3c cs = state.proj * state.view * ws;
      // clip
      // glm::vec4 ss = cs;
      // ss /= ss.w;
      // ss *= glm::vec4(0.5, 0.5, 1, 1);
      // ss += glm::vec4(0.5, 0.5, 0, 0);
      // ss += glm::vec4(0, 0, 0, 0); // glm::vec4(viewport[0], viewport[1], 0,
      // 0); ss *= glm::vec4(WIDTH, HEIGHT, 1,
      //                 1); // glm::vec4(viewport[2], viewport[3], 1, 1);

      glm::vec3 ss =
          glm::project(glm::vec3(ls), state.view * state.model.matrix,
                       state.proj, glm::vec4(0, 0, WIDTH, HEIGHT));

      ft[i] = glm::vec2(ss);
    }

    filledtriangle(window, std::make_tuple(ft, c));
    // linetriangle(window, std::make_tuple(ft, c));
  }
}

void update() {
  state.frame++;

  state.view = state.camera.view();
  state.model.matrix =
      glm::scale(
          glm::vec3(scale, scale, scale)) * // scale can be edited with keys
      // glm::scale(glm::vec3(1, -1, 1)) *
      glm::translate(-state.model.centre);
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
      std::cout << "scale\t"
                << "x\t"
                << "y\t"
                << "z\t" << std::endl;
      std::cin >> scale >> x >> y >> z;
      std::cout << "scale\t"
                << "x\t"
                << "y\t"
                << "z\t" << std::endl;
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
