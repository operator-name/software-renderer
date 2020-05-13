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

  glm::mat4 matrix; // model matrix with below stuff applied, TODO: proper types

  glm::vec3 centre;
  glm::vec3 scale = glm::vec3(1, 1, 1);
  glm::vec3 position;

  enum class RenderMode {
    WIREFRAME_AA, // draw order matters, but lines are AA
    WIREFRAME,    // draw order doesn't matter but lines are not AA
    FILL,
    RAYTRACE,
  };

  RenderMode mode = RenderMode::WIREFRAME;
};

struct Camera {
  glmt::vec3w target = glm::vec4(0, 0, 0, 1);

  float dist = 12.2f;
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

  float fov = glm::radians(90.f);
};

struct State {
  // std::tuple<std::array<glmt::vec2s, 3>, glmt::rgb888> unfilled_triangle;
  // std::tuple<std::array<glmt::vec2s, 3>, glmt::rgb888> filled_triangle;

  std::vector<Model> models;
  std::vector<Model::RenderMode> orig;

  Camera camera;
  glm::mat4 view;
  glm::mat4 proj;

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

// sets centre and scale such that model "centre of mass" is at 0, 0 and is at
// most 1 unit
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
  {
    glmt::OBJ obj = parse_obj("logo.obj");
    Model model;

    model.triangles = obj.triangles;
    // whilst we can't render textures, just render some random colour
    for (size_t i = 0; i < obj.triangles.size(); i++) {
      model.colours.push_back(glmt::rgbf01(glm::linearRand(0.f, 1.f),
                                           glm::linearRand(0.f, 1.f),
                                           glm::linearRand(0.f, 1.f)));
    }
    model = align(model);

    model.scale *= 7;
    model.mode = Model::RenderMode::WIREFRAME;
    model.position = glm::vec3(-3, 4, 0);

    state.models.push_back(model);
  }
  {
    glmt::OBJ obj = parse_obj("cornell-box.obj");
    Model model;

    model.triangles = obj.triangles;
    model.colours = obj.colours;
    model = align(model);

    model.scale *= 5;
    model.mode = Model::RenderMode::FILL;
    model.position = glm::vec3(3, 4, 0);

    state.models.push_back(model);
  }
  {
    glmt::OBJ obj = parse_obj("cornell-box.obj");
    Model model;

    model.triangles = obj.triangles;
    model.colours = obj.colours;
    // for (size_t i = 0; i < obj.triangles.size(); i++) {
    //   model.colours.push_back(glmt::rgbf01(glm::linearRand(0.f, 1.f),
    //                                        glm::linearRand(0.f, 1.f),
    //                                        glm::linearRand(0.f, 1.f)));
    // }
    model = align(model);

    model.scale *= 5;
    model.mode = Model::RenderMode::RAYTRACE;
    model.position = glm::vec3(0, -3, 0);
    // model.position = glm::vec3(0, 0, 8);

    state.models.push_back(model);
    // model.mode = Model::RenderMode::WIREFRAME;
    // state.models.push_back(model);
  }

  window = sdw::window(WIDTH, HEIGHT, false);

  if (WRITE_FILE) {
    std::cout << "Saving " << FRAMES << "frames at " << FPS << "fps"
              << std::endl;
  } else {
    state.orig.resize(state.models.size());
    for (size_t i = 0; i < state.models.size(); i++) {
      state.orig[i] = state.models[i].mode;
      state.models[i].mode = state.models[i].mode == Model::RenderMode::RAYTRACE
                                 ? Model::RenderMode::FILL
                                 : state.models[i].mode;
    }
  }
}

glmt::rgbf01
light(const Model &model,
      const std::vector<std::array<glm::vec4, 3>> &triangles, // camera space
      const Intersection &intersection) {
  // We'll use the colour space rgbf0inf implicitly
  // TODO: add rgbf0inf to glmt
  // TODO: move things involving state.logic to update function
  // TODO: add values into state and model
  float delta = (state.logic / FPS); // s since start

  float s2 = glm::sin(delta / 2);
  float s3 = glm::sin(delta / 3);
  float s5 = glm::sin(delta / 5);
  float s7 = glm::sin(delta / 7);
  float s11 = glm::sin(delta / 11);

  const glm::vec4 light_position =
      state.view * model.matrix *
          glm::vec4(-0.234011, 5.218497 - 0.218497, -3.042968, 1) +
      glm::vec4(s3 * 2, 0, s5 * s7, 0);
  const glm::vec3 direct_colour = glm::vec3(1, 1, 1) * (600.f + 100.f * s2);
  const glm::vec3 indirect_colour = glm::vec3(1, 1, 1) * 0.0745f;
  const glm::vec3 point_colour = model.colours[intersection.triangleIndex];
  const auto triangle_normal =
      [](std::array<glm::vec4, 3> triangle) -> glm::vec4 {
    glm::vec3 e1 = glm::vec3(triangle[1] - triangle[0]);
    glm::vec3 e2 = glm::vec3(triangle[2] - triangle[0]);
    glm::vec3 normal = glm::normalize(glm::cross(e2, e1));

    return glm::vec4(
        normal,
        1.0); // normals should be transformed when the triangle is transformed
  };

  glm::vec4 r = glm::normalize(-intersection.position + light_position);
  glm::vec4 n = triangle_normal(triangles[intersection.triangleIndex]);
  float radius = glm::length(-intersection.position + light_position);

  Intersection to_light;
  if (ClosestIntersection(light_position, -r, triangles, to_light)) {
    if (to_light.triangleIndex != intersection.triangleIndex) {
      return tm_basic(point_colour * (indirect_colour));
    }
  }

  return tm_basic(point_colour *
                  ((direct_colour * glm::max(glm::dot(r, n), 0.f)) /
                       (4 * glm::pi<float>() * (radius * radius)) +
                   indirect_colour));
}

void draw() {
  window.clearPixels();
  window.clearDepthBuffer();

  // linetriangle(window, state.unfilled_triangle);
  // filledtriangle(window, state.filled_triangle);

  // TODO: AoS to SoA
  for (const auto &model : state.models) {
    if (model.mode == Model::RenderMode::RAYTRACE) {
      // float focalLength =
      //     (HEIGHT / 2) *
      //     glm::tan(glm::radians(90.f / 2.0f));
      glm::vec4 cameraPos = glm::vec4(0, 0, 0, 1);

      for (unsigned int x = 0; x < window.width; x++) {
        for (unsigned int y = 0; y < window.height; y++) {
          // glm::vec4 ray(((float)x - window.width / 2.0),
          //               ((float)y - window.height / 2.0), -focalLength, 0.0);
          glm::vec4 ray = glm::vec4(
              glm::unProject(glm::vec3(x, y, 1), glm::mat4(1.0), state.proj,
                             glm::vec4(0, 0, window.width, window.height)),
              0);

          Intersection intersection;
          std::vector<std::array<glm::vec4, 3>> triangles;

          std::transform(model.triangles.begin(), model.triangles.end(),
                         std::back_inserter(triangles),
                         [=](std::array<glmt::vec3l, 3> triangle)
                             -> std::array<glm::vec4, 3> {
                           std::array<glm::vec4, 3> ts;

                           for (size_t i = 0; i < ts.size(); ++i) {
                             ts[i] = state.view * model.matrix * triangle[i];
                           }

                           return ts;
                         });

          if (ClosestIntersection(cameraPos, glm::normalize(ray), triangles,
                                  intersection)) {
            glmt::rgbf01 c = model.colours[intersection.triangleIndex];
            glm::vec3 p = glm::project(
                glm::vec3(intersection.position), glm::mat4(1), state.proj,
                glm::vec4(0, 0, window.width, window.height));
            window.setPixelColour(
                glmt::vec2p(x, y), 1.f / p.z,
                light(model, triangles, intersection).argb8888());
          }
        }
      }

    } else {
      for (size_t i = 0; i < model.triangles.size(); i++) {
        std::array<glmt::vec3s, 3> transformed;
        std::array<glmt::vec2s, 3> transformed2;

        for (size_t t = 0; t < transformed.size(); t++) {
          // manually apply stuff, need to unpack if near/far clipping to be
          // implemented although clipping can be done in w space if using
          // glm::project glmt::vec3l ls = std::get<0>(t)[i]; glmt::vec3w ws =
          // state.model.matrix * ls; glmt::vec3c cs = state.proj * state.view *
          // ws; clipping done here glm::vec4 ss = cs; // viewport
          // transformation below ss /= ss.w; ss *= glm::vec4(0.5, 0.5, 1, 1);
          // ss += glm::vec4(0.5, 0.5, 0, 0);
          // ss += glm::vec4(0, 0, 0, 0); // glm::vec4(viewport[0], viewport[1],
          // 0, 0); ss *= glm::vec4(window.width, window.height, 1,
          //                 1); // glm::vec4(viewport[2], viewport[3], 1, 1);

          glm::vec4 ss = glm::vec4(
              glm::project(glm::vec3(model.triangles[i][t]),
                           state.view * model.matrix, state.proj,
                           glm::vec4(0, 0, window.width, window.height)),
              1);

          transformed[t] = ss;
          transformed2[t] = glm::vec2(ss);
        }

        if (model.mode == Model::RenderMode::WIREFRAME) {
          linetriangle(window, std::make_tuple(transformed, model.colours[i]));
        } else if (model.mode == Model::RenderMode::WIREFRAME_AA) {
          linetriangle(window, std::make_tuple(transformed2, model.colours[i]));
        } else if (model.mode == Model::RenderMode::FILL) {
          filledtriangle(window,
                         std::make_tuple(transformed, model.colours[i]));
        }
      }
    }
  }
}

void update() {
  state.frame++;

  if (!state.update) {
    return;
  }

  state.logic++;

  switch (state.logic) {
  case int(FRAMES / 2 - FPS * 0.1):
    state.models[0].mode = Model::RenderMode::FILL;
  case int(FRAMES / 2):
    state.models[0].mode = Model::RenderMode::WIREFRAME_AA;
  }

  float delta = (state.logic / FPS); // s since start

  float s2 = glm::sin(delta / 2);
  float s3 = glm::sin(delta / 3);
  float s5 = glm::sin(delta / 5);
  float s7 = glm::sin(delta / 7);
  float s11 = glm::sin(delta / 11);

  state.camera.yaw = (s2 * s3 * s7);
  state.camera.pitch = (s3 * s5 * s11);
  state.camera.dist = 10 - 3 * (s2 * s3 * s5 * s7 * s11);

  state.view = state.camera.view();
  state.proj = glm::perspectiveFov(state.camera.fov, (float)window.width,
                                   (float)window.height, 0.1f, 100.0f);

  for (size_t i = 0; i < state.models.size(); ++i) {
    glm::mat4 fun = glm::rotate(
        delta * 0.5f, glm::euclidean(glm::vec2(glm::sin((delta + i) / 3),
                                               glm::cos((delta + i) / 5))));

    // TODO: proper typing in glmt
    // is this what we want? a translation of +y is intuitively upwards
    // but that would mean glmt::d3::vec3s has +y up and glmt::d3::vec2s has +y
    // down
    glm::vec4 tp = glm::scale(glm::vec3(1, -1, 1)) *
                   glm::vec4(state.models[i].position, 1);

    state.models[i].matrix =
        glm::translate(glm::vec3(tp)) * (i != 2 ? fun : glm::mat4(1)) *
        glm::scale(state.models[i].scale) *
        glm::scale(glm::vec3(1, -1, 1)) * // y is up to y is down
        glm::translate(-state.models[i].centre);
  }
}

void handleEvent(SDL_Event event) {
  switch (event.type) {
  case SDL_KEYDOWN:
    switch (event.key.keysym.sym) {
    case SDLK_LEFT:
      state.camera.yaw += state.camera.rot_velocity;
      std::cout << "camera.yaw: " << state.camera.yaw << std::endl;
      break;
    case SDLK_RIGHT:
      state.camera.yaw -= state.camera.rot_velocity;
      std::cout << "camera.yaw: " << state.camera.yaw << std::endl;
      break;
    case SDLK_UP:
      state.camera.dist -= state.camera.velocity;
      std::cout << "camera.dist: " << state.camera.dist << std::endl;
      break;
    case SDLK_DOWN:
      state.camera.dist += state.camera.velocity;
      std::cout << "camera.dist: " << state.camera.dist << std::endl;
      break;
    case SDLK_PAGEUP:
      state.camera.pitch += state.camera.rot_velocity;
      std::cout << "camera.pitch: " << state.camera.pitch << std::endl;
      break;
    case SDLK_PAGEDOWN:
      state.camera.pitch -= state.camera.rot_velocity;
      std::cout << "camera.pitch: " << state.camera.pitch << std::endl;
      break;
    case SDLK_d:
      std::cout << "state.frame: " << state.frame << std::endl;
      std::cout << "state.logic: " << state.logic << std::endl;
      std::cout << "camera.yaw: " << state.camera.yaw << std::endl;
      std::cout << "camera.dist: " << state.camera.dist << std::endl;
      std::cout << "camera.pitch: " << state.camera.pitch << std::endl;
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

    // case SDLK_t:
    //   state.unfilled_triangle = randomtriangleinside(window);
    //   state.filled_triangle = randomtriangleinside(window);
    //   break;
    case SDLK_w:
      if (state.orig.empty()) {
        std::cout << "wireframe " << state.orig.size() << std::endl;
        state.orig.resize(state.models.size());
        for (size_t i = 0; i < state.models.size(); i++) {
          state.orig[i] = state.models[i].mode;
          state.models[i].mode = Model::RenderMode::WIREFRAME;
        }
      }
      break;
    case SDLK_f:
      if (state.orig.empty()) {
        std::cout << "fill " << state.orig.size() << std::endl;
        state.orig.resize(state.models.size());
        for (size_t i = 0; i < state.models.size(); i++) {
          state.orig[i] = state.models[i].mode;
          state.models[i].mode = Model::RenderMode::FILL;
        }
      }
      break;
    case SDLK_o:
      std::cout << "orig" << std::endl;
      for (size_t i = 0; i < state.models.size(); i++) {
        state.models[i].mode = state.orig[i];
      }
      state.orig.clear();
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
