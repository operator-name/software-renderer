#include <sdw/window.h>

#include "glmt.hpp"
#include "practical.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/component_wise.hpp>
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
  typedef std::array<glmt::vec3l, 3> triangle;

  std::vector<triangle> triangles;
  std::vector<glmt::rgbf01> colours;

  glm::mat4 matrix; // model matrix, TODO: proper types
  glm::vec3 centre;
  glm::vec3 scale = glm::vec3(1, 1, 1);
  glm::vec3 position;

  enum class RenderMode {
    WIREFRAME,
    FILL,
  };

  RenderMode mode = RenderMode::WIREFRAME;
};

struct Camera {
  glmt::vec3w target = glm::vec4(0, 0, 0, 1);

  float dist = 10.2f;
  float pitch = 0;
  float yaw = 0.001;
  float rot_velocity = 0.05f;
  float velocity = 0.05f;

  // regular lookat, defaults positive y as up
  glm::mat4 lookat(glm::vec3 from, glm::vec3 to,
                   glm::vec3 up = glm::vec3(0, 1, 0)) {
    glm::vec3 forward = glm::normalize(from - to);
    glm::vec3 right = glm::cross(glm::normalize(up), forward);

    glm::mat4 mat(1);

    mat[0][0] = right.x;
    mat[0][1] = right.y;
    mat[0][2] = right.z;
    mat[1][0] = up.x;
    mat[1][1] = up.y;
    mat[1][2] = up.z;
    mat[2][0] = forward.x;
    mat[2][1] = forward.y;
    mat[2][2] = forward.z;

    mat[3][0] = from.x;
    mat[3][1] = from.y;
    mat[3][2] = from.z;

    return mat;
  }

  // "look at" but with orbit style configuration, nicer for demos
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

Model align(Model model) {
  glm::vec3 max(std::numeric_limits<float>::lowest());
  glm::vec3 min(std::numeric_limits<float>::max());
  for (const auto &triangle : model.triangles) {
    for (const glmt::vec3l point : triangle) {
      max = glm::max(max, glm::vec3(point));
      min = glm::min(min, glm::vec3(point));
    }
  }

  model.centre = glm::vec3(min + max) / 2.f;
  float scale = 1 / glm::compMax(glm::abs(max - min));
  model.scale = glm::vec3(scale, scale, scale);

  return model;
}

void setup() {
  // seed random state to be the same each time (for debugging)
  // TODO: add proper random state
  std::srand(0);

  // state.obj = parse_obj("logo.obj");

  // for (size_t i = 0; i < state.obj.triangles.size(); ++i) {
  //   state.model.triangles.push_back(std::make_tuple(
  //       state.obj.triangles[i],
  //       glmt::rgbf01(glm::linearRand(0.f, 1.f), glm::linearRand(0.f, 1.f),
  //                    glm::linearRand(0.f, 1.f))));
  // }

  // state.model = align(state.model);
  // state.model.scale *= 9;

  state.obj = parse_obj("cornell-box.obj");
  state.model.triangles = state.obj.triangles;
  state.model.colours = state.obj.colours;

  state.model = align(state.model);
  state.model.scale *= 5;

  window = sdw::window(WIDTH, HEIGHT, false);

  std::cout << "Saving " << FRAMES << "frames at " << FPS << "fps" << std::endl;
}

void draw() {
  window.clearPixels();
  window.clearDepthBuffer();
  for (size_t i = 0; i < state.model.triangles.size(); i++) {
    std::array<glmt::vec3s, 3> transformed;
    std::array<glmt::vec2s, 3> transformed2;

    for (size_t t = 0; t < transformed.size(); t++) {
      // manually apply stuff, need to unpack if clipping to be implemented
      // although clipping can be done in w space if using glm::project
      // glmt::vec3l ls = std::get<0>(t)[i];
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

      glm::vec4 ss =
          glm::vec4(glm::project(glm::vec3(state.model.triangles[i][t]),
                                 state.view * state.model.matrix, state.proj,
                                 glm::vec4(0, 0, WIDTH, HEIGHT)),
                    1);

      transformed[t] = ss;
      transformed2[t] = glm::vec2(ss);
    }

    switch (state.model.mode) {
    case Model::RenderMode::WIREFRAME:
      linetriangle(window,
                   std::make_tuple(transformed2, state.model.colours[i]));
      break;
    case Model::RenderMode::FILL:
      filledtriangle(window,
                     std::make_tuple(transformed, state.model.colours[i]));
      break;
    }
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

  float s2 = glm::sin(delta / 2);
  float s3 = glm::sin(delta / 3);
  float s5 = glm::sin(delta / 5);
  float s7 = glm::sin(delta / 7);
  float s11 = glm::sin(delta / 11);

  state.camera.yaw = (s2 * s3 * s7);
  state.camera.pitch = (s3 * s5 * s11);
  state.camera.dist = 7 - 3 * (s2 * s3 * s5 * s7 * s11);

  state.view = state.camera.view();

  glm::mat4 fun = glm::rotate(
      delta,
      glm::euclidean(glm::vec2(glm::sin(delta / 3), glm::cos(delta / 5))));
  state.model.matrix = fun * glm::translate(state.model.position) *
                       glm::scale(state.model.scale) *
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
