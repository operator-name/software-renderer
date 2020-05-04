#include <sdw/window.h>

#include "glmt.hpp"
#include "practical.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/polar_coordinates.hpp>
#include <glm/gtx/transform.hpp>

#include <glm/gtx/io.hpp>

#include <cstdlib>

#define FPS 30.0
#define TIME 30.0
#define FRAMES (FPS * TIME)
#define WRITE_FILE false
#define EXIT_AFTER_WRITE (WRITE_FILE && true)
#define RENDER true

#define N 2
#define WIDTH (320 * N)
#define HEIGHT (240 * N)

void setup();
void draw();
void update();
void handleEvent(SDL_Event event);

// TODO: move into State struct
sdw::window window;

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

  float dist = 10.2f;
  float pitch = 0;
  float yaw = 0.001;
  float rot_velocity = 0.02f;
  float velocity = 0.03f;

  // "look at" but with orbit style configuration
  glm::mat4 view() {
    glm::vec4 pos = glm::vec4(0, 0, -dist, 0) + target;
    glm::mat4 rot = glm::rotate(pitch, glm::vec3(1, 0, 0)) *
                    glm::rotate(yaw, glm::vec3(0, 1, 0));
    return glm::translate(glm::vec3(pos)) * rot;
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

  unsigned int frame = -1; // update called first which incremets frame
  unsigned int logic = -1;
  bool update = true; // logic is paused but frames keep advancing
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

    if (RENDER) {
      window.renderFrame();
    }

    if (WRITE_FILE && state.frame < FRAMES) { // save frame as PPM
      glmt::PPM ppm;
      ppm.header.width = window.width;
      ppm.header.height = window.height;
      ppm.header.maxval = 255;
      ppm.reserve();

      // COPY
      for (unsigned int y = 0; y < window.height; y++) {
        for (unsigned int x = 0; x < window.width; x++) {
          // to glm::vec3, dropping the alpha channel then divide by 255.f
          glmt::rgbf01 curr =
              glm::vec3(window.getPixelColour(glmt::vec2p(x, y))) / 255.f;

          ppm[glmt::vec2t(x, y)] = curr;
        }
      }

      // WRITE
      std::stringstream filename;
      filename << "PPM/frame" << std::setfill('0') << std::setw(5)
               << state.frame << ".ppm";
      // std::cout << filename.str() << std::endl;
      std::ofstream file(filename.str());
      file << ppm;
    } else if (EXIT_AFTER_WRITE) {
      exit(0);
    }
  }
}

void setup() {
  state.obj = parse_obj("cornell-box.obj");

  state.model.triangles = state.obj.triangles;

  // calculate "centre" of the model to use as origin for tansformations
  // TODO: move to bound3
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

  std::cout << "Saving " << FRAMES << "frames at " << FPS << "fps" << std::endl;
}

void draw() {
  window.clearPixels();
  for (auto const &t : state.model.triangles) {
    std::array<glmt::vec2s, 3> ft;
    auto c = std::get<1>(t);

    for (size_t i = 0; i < ft.size(); i++) {

      glmt::vec3l ls = std::get<0>(t)[i];
      // manually apply stuff
      // glmt::vec3w ws = state.model.matrix * ls;
      // glmt::vec3c cs = state.proj * state.view * ws;
      // clipping done here
      // glm::vec4 ss = cs; // viewport transformation below
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

    // filledtriangle(window, std::make_tuple(ft, c));
    linetriangle(window, std::make_tuple(ft, c));
  }

  linetriangle(window, state.unfilled_triangle);
  filledtriangle(window, state.filled_triangle);
}

void update() {
  state.frame++;

  if (!state.update) {
    return;
  }

  state.logic++;

  float delta = (state.logic / FPS); // s since start

  // state.camera.yaw = glm::sin(delta / 2) * 2;
  // state.camera.pitch = (-glm::abs(glm::sin(delta / 3)) + 0.5) * 1.5;
  float spoon = glm::atan(glm::atan(delta - TIME / 3) * glm::log(delta + 0.5));
  state.camera.dist = 10 - 2 * spoon;

  state.view = state.camera.view();
  state.proj = glm::perspectiveFov(glm::radians(100.f), (float)WIDTH,
                                   (float)HEIGHT, 0.1f, 100.0f);

  glm::mat4 fun = glm::rotate(
      delta,
      glm::euclidean(glm::vec2(glm::sin(delta / 3), glm::cos(delta / 5))));
  state.model.matrix = fun *
                       glm::scale(glm::vec3(1, -1, 1)) * // y is up to y is down
                       glm::translate(-state.model.centre);
}

void handleEvent(SDL_Event event) {
  switch (event.type) {
  case SDL_KEYDOWN:
    switch (event.key.keysym.sym) {
    case SDLK_LEFT:
      state.camera.yaw += state.camera.rot_velocity;
      // std::cout << "LEFT" << std::endl;
      break;
    case SDLK_RIGHT:
      state.camera.yaw -= state.camera.rot_velocity;
      // std::cout << "RIGHT" << std::endl;
      break;
    case SDLK_UP:
      state.camera.dist -= state.camera.velocity;
      // std::cout << "UP" << std::endl;
      break;
    case SDLK_DOWN:
      state.camera.dist += state.camera.velocity;
      // std::cout << "DOWN" << std::endl;
      break;
    case SDLK_PAGEUP:
      state.camera.pitch += state.camera.rot_velocity;
      break;
    case SDLK_PAGEDOWN:
      state.camera.pitch -= state.camera.rot_velocity;
      break;
    case SDLK_c:
      window.clearPixels();
      break;
    case SDLK_d:
      std::cout << "[DEBUG] state.frame: " << state.frame << std::endl;
      break;
    case SDLK_p:
      state.update = !state.update;
      std::cout << "[DEBUG] state.update: " << (state.update ? "true" : "false")
                << std::endl;
      break;
    case SDLK_s: {
      std::cout << "[DEBUG] saving frame to file debug.ppm" << std::endl;

      glmt::PPM debug_ppm;
      debug_ppm.header.width = window.width;
      debug_ppm.header.height = window.height;
      debug_ppm.header.maxval = 255;
      debug_ppm.reserve();

      // COPY
      for (unsigned int y = 0; y < window.height; y++) {
        for (unsigned int x = 0; x < window.width; x++) {
          glmt::rgbf01 curr =
              glm::vec3(window.getPixelColour(glmt::vec2p(x, y))) / 255.f;

          debug_ppm[glmt::vec2t(x, y)] = curr;
        }
      }

      {
        std::ofstream file("debug.ppm");
        file << debug_ppm;
      }

      std::cout << "[DEBUG] frame saved to debug.ppm, opening..." << std::endl;
      sdw::window debug = texture_window("debug.ppm");
      std::cout << "[DEBUG] press enter to close debug and continue..."
                << std::endl;
      std::cin.get();
      debug.close();
      break;
    }

    case SDLK_u:
      state.unfilled_triangle = randomtriangleinside(window);
      break;
    case SDLK_f:
      state.filled_triangle = randomtriangleinside(window);
      break;
      // case SDLK_x:
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
