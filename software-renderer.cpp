#include <sdw/window.h>

#include "glmt.hpp"
#include "practical.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/polar_coordinates.hpp>
#include <glm/gtx/transform.hpp>

#include <glm/gtx/io.hpp>

#include <chrono>
#include <cstdlib>

#define FPS 30.0
#define TIME 30.0
#define FRAMES (FPS * TIME)
#define WRITE_FILE false
#define EXIT_AFTER_WRITE (WRITE_FILE && true)
#define RENDER true

#define N 1
#define WIDTH (320 * N)
#define HEIGHT (240 * N)

void setup();
void draw();
void update();
void handleEvent(SDL_Event event);

// TODO: move into State struct
sdw::window window;

struct State {
  // std::tuple<std::array<glmt::vec2s, 3>, glmt::rgb888> unfilled_triangle;
  // std::tuple<std::array<glmt::vec2s, 3>, glmt::rgb888> filled_triangle;

  std::vector<Model> models;
  std::vector<Model::RenderMode> orig;

  Camera camera;
  glm::mat4 view;
  glm::mat4 proj;

  PointLight light;
  bool raymarch = false;

  struct SDL_detail {
    bool mouse_down = false;
  } sdl;

  unsigned int frame = -1; // update called first which incremets frame
  unsigned int logic = -1;
  bool update = true; // logic is paused but frames keep advancing
} state;

void setup() {
  // seed random state to be the same each time (for debugging)
  // TODO: add proper random state
  std::srand(0);
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
    model.mode = Model::RenderMode::PATHTRACE;
    model.position = glm::vec3(0, 0, 0);
    // model.position = glm::vec3(0, 0, 8);

    state.models.push_back(model);
    // model.mode = Model::RenderMode::WIREFRAME;
    // state.models.push_back(model);
  }
  {
    glmt::OBJ obj = parse_obj("cornell-box.obj");
    Model model;

    model.triangles = obj.triangles;
    model.colours = obj.colours;
    model = align(model);

    model.scale *= 1;
    model.mode = Model::RenderMode::FILL;
    model.position = glm::vec3(-0.8, 1.2, -0.8);

    state.models.push_back(model);
  }
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

    model.scale *= 1.5;
    model.mode = Model::RenderMode::WIREFRAME;
    model.position = glm::vec3(0.9, -0.5, 0.9);

    state.models.push_back(model);
  }

  window = sdw::window(WIDTH, HEIGHT, false);

  if (WRITE_FILE) {
    std::cout << "Saving " << FRAMES << "frames at " << FPS << "fps"
              << std::endl;
  }
}

int main(int argc, char *argv[]) {
  auto t1 = std::chrono::high_resolution_clock::now();
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
      if (state.frame > 0 && state.frame % static_cast<int>(FPS) == 0) {
        auto t2 = std::chrono::high_resolution_clock::now();
        std::cout << "frames: " << static_cast<float>(state.frame) << std::endl;
        std::cout
            << "duration (s): "
            << std::chrono::duration_cast<std::chrono::seconds>(t2 - t1).count()
            << std::endl;
        std::cout << "ms/frame: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(t2 -
                                                                           t1)
                             .count() /
                         static_cast<float>(state.frame)
                  << std::endl;
        std::cout << "frames/s: "
                  << static_cast<float>(state.frame) /
                         std::chrono::duration_cast<std::chrono::seconds>(t2 -
                                                                          t1)
                             .count()
                  << std::endl;
      }

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
      auto t2 = std::chrono::high_resolution_clock::now();
      std::cout << "frames: " << static_cast<float>(state.frame) << std::endl;
      std::cout
          << "duration (s): "
          << std::chrono::duration_cast<std::chrono::seconds>(t2 - t1).count()
          << std::endl;
      std::cout << "ms/frame: "
                << std::chrono::duration_cast<std::chrono::milliseconds>(t2 -
                                                                         t1)
                           .count() /
                       static_cast<float>(state.frame)
                << std::endl;
      std::cout << "frames/s: "
                << static_cast<float>(state.frame) /
                       std::chrono::duration_cast<std::chrono::seconds>(t2 - t1)
                           .count()
                << std::endl;
      exit(0);
    }
  }
}

// https://iquilezles.org/www/articles/distfunctions/distfunctions.htm
float sphere(const glmt::vec3w pos, const float radius) {
  return glm::length(glm::vec3(pos)) - radius;
}

float torus(const glmt::vec3w p, const glm::vec2 t) {
  glm::vec2 xz = glm::vec2(p.x, p.z);
  glm::vec2 q = glm::vec2(glm::length(xz) - t.x, p.y);
  return length(q) - t.y;
}

// https://iquilezles.org/www/articles/smin/smin.htm
// polynomial smooth min (k = 0.1);
float smin(const float a, const float b, const float k = 1.5f) {
  const float h = glm::clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
  return glm::mix(b, a, h) - k * h * (1.0 - h);
}

float d_sphere(glmt::vec3w p) {
  const float X = 10;
  const float Y = 0.1;
  float d1 = sphere(p - glm::vec4(0, 0, 0, 0), 0.4f);
  float d2 = Y * sin(X * p.x) * sin(X * p.y) * sin(X * p.z);
  return d1 + d2;
}

float twist_torus(glmt::vec3w p, float s2) {
  const float k = 10.0; // or some other amount
  float c = glm::cos(k * p.y);
  float s = glm::sin(k * p.y);
  glm::mat2 m = glm::mat2(c, -s, s, c);
  glm::vec3 q = glm::vec3(m * glm::vec2(p.x, p.z), p.y);
  return torus(glm::vec4(q, 1.f) - glm::vec4(0, 0, s2 * 2, 0),
               glm::vec2(0.2f, 0.05f));
  ;
}

float scene(const glmt::vec3w pos) {
  float logic = state.logic;
  float delta = (logic / FPS) * 5; // s since start

  float s2 = glm::sin(delta / 2);
  float s3 = glm::sin(delta / 3);
  float s5 = glm::sin(delta / 5);
  float s7 = glm::sin(delta / 7);
  float s11 = glm::sin(delta / 11);

  float result = std::numeric_limits<float>::max();

  glm::mat4 fun = glm::rotate(
      delta * 0.5f,
      glm::euclidean(glm::vec2(glm::sin(delta / 7), glm::cos(delta / 9))));
  result = glm::min(result, d_sphere(pos));
  result = smin(result, twist_torus(pos, s2));
  result =
      smin(result, sphere(pos - glm::vec4(0, s3 - 1, s3 * 1.5 + 2, 1), 0.3));
  result = smin(result, torus(fun * pos - glm::vec4(s5 * 2, s7 * 3, 0, 0),
                              glm::vec2(1.0, 0.5)));

  return result;
}

// TODO: merge back into scene
glm::vec3 colour(const glmt::vec3w pos) {
  float logic = state.logic;
  float delta = (logic / FPS) * 5; // s since start

  float s2 = glm::sin(delta / 2);
  float s3 = glm::sin(delta / 3);
  float s5 = glm::sin(delta / 5);
  float s7 = glm::sin(delta / 7);
  float s11 = glm::sin(delta / 11);

  float result = std::numeric_limits<float>::max();

  glm::mat4 fun = glm::rotate(
      delta * 0.5f,
      glm::euclidean(glm::vec2(glm::sin(delta / 7), glm::cos(delta / 9))));
  float dsph = d_sphere(pos) + 0.1;
  float ttor = twist_torus(pos, s2) + 0.1;
  float sphe = sphere(pos - glm::vec4(0, s3 - 1, s3 * 1.5 + 2, 1), 0.3) + 0.1;
  float toru =
      torus(fun * pos - glm::vec4(s5 * 2, s7 * 3, 0, 0), glm::vec2(1.0, 0.5)) +
      0.1;
  glm::vec3 red(1, 0, 0);
  glm::vec3 gre(0, 0, 1);
  glm::vec3 blu(0, 0, 1);
  glm::vec3 bla(0.00000000001);

  const float p = 2;

  return bla / glm::pow(toru, p) + red / glm::pow(sphe, p) +
         gre / glm::pow(ttor, p) + blu / glm::pow(dsph, p);
}

float march(const glmt::vec3w eye, const glm::vec4 dir, const float start,
            const float end, const int steps, const float epsilon) {
  float depth = start;
  for (int i = 0; i < steps; i++) {
    float dist = scene(eye + depth * dir);
    if (dist < epsilon) {
      return depth;
    }
    depth += dist;
    if (depth >= end) {
      return end;
    }
  }
  return end;
}

// https://iquilezles.org/www/articles/normalsSDF/normalsSDF.htm
glm::vec3 normal(const glmt::vec3w p, const float epsilon) {
  const glm::vec4 xyy(epsilon, 0, 0, 0);
  const glm::vec4 yxy(0, epsilon, 0, 0);
  const glm::vec4 yyx(0, 0, epsilon, 0);
  return glm::normalize(glm::vec3(scene(p + xyy) - scene(p - xyy),
                                  scene(p + yxy) - scene(p - yxy),
                                  scene(p + yyx) - scene(p - yyx)));
}

void draw() {
  window.clearPixels();
  window.clearDepthBuffer();

  // TODO: AoS to SoA
  for (const auto &model : state.models) {
    if (model.mode == Model::RenderMode::PATHTRACE) {
      // float focalLength =
      //     (HEIGHT / 2) *
      //     glm::tan(glm::radians(90.f / 2.0f));
      glm::vec4 cameraPos = glm::vec4(0, 0, 0, 1); // camera in camera space

      glmt::bound2s bounds;
      {
        glm::vec2 max(std::numeric_limits<float>::lowest());
        glm::vec2 min(std::numeric_limits<float>::max());
        // TODO: figure out how many points in the BB of the model are required
        // for the affine transformation proj when converting to vec2s, but for
        // now a few matrix multiplcations are not that expensive
        for (const auto &triangle : model.triangles) {
          for (const glmt::vec3l point : triangle) {
            glm::vec3 ss = glm::project(
                glm::vec3(point), state.view * model.matrix, state.proj,
                glm::vec4(0, 0, window.width, window.height));
            max = glm::max(max, glm::vec2(ss));
            min = glm::min(min, glm::vec2(ss));
          }
        }

        bounds.min = glm::max(glm::floor(min), 0.f);
        bounds.max =
            glm::min(glm::ceil(max), glm::vec2(window.width, window.height));
      }

      for (int y = bounds.min.y; y <= bounds.max.y; y++) {
        for (int x = bounds.min.x; x <= bounds.max.x; x++) {
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
            glm::vec3 p = glm::project(
                glm::vec3(intersection.position), glm::mat4(1), state.proj,
                glm::vec4(0, 0, window.width, window.height));
            window.setPixelColour(glmt::vec2p(x, y), 1.f / p.z,
                                  pathtrace_light(model, triangles, state.light,
                                                  ray, intersection)
                                      .argb8888());
          }
        }
      }

    } else {
      for (size_t i = 0; i < model.triangles.size(); i++) {
        std::array<glm::vec4, 3> transformedc;
        std::array<glmt::vec3s, 3> transformed;
        std::array<glmt::vec2s, 3> transformed2;

        for (size_t t = 0; t < transformed.size(); t++) {
          // manually apply stuff, need to unpack if near/far clipping to be
          // implemented although clipping can be done in w space if using
          // glm::project
          glmt::vec3l ls = model.triangles[i][t];
          glmt::vec3w ws = model.matrix * ls;
          glmt::vec3c cs = state.view * ws;
          // clipping done here
          // glm::vec4 ss = state.proj * cs; // viewport transformation below
          // ss /= ss.w;
          // ss *= glm::vec4(0.5, 0.5, 1, 1);
          // ss += glm::vec4(0.5, 0.5, 0, 0);
          // ss += glm::vec4(0, 0, 0,
          //                 0); // glm::vec4(viewport[0], viewport[1], 0, 0);
          // ss *= glm::vec4(window.width, window.height, 1,
          //                 1); // glm::vec4(viewport[2], viewport[3], 1, 1);

          glm::vec4 ss = glm::vec4(
              glm::project(glm::vec3(cs), glm::mat4(1), state.proj,
                           glm::vec4(0, 0, window.width, window.height)),
              1);

          transformedc[t] = cs;
          transformed[t] = ss;
          transformed2[t] = glm::vec2(ss);
        }

        if (model.mode == Model::RenderMode::WIREFRAME) {
          linetriangle(window, std::make_tuple(transformed, model.colours[i]));
        } else if (model.mode == Model::RenderMode::WIREFRAME_AA) {
          linetriangle(window, std::make_tuple(transformed2, model.colours[i]));
        } else if (model.mode == Model::RenderMode::FILL) {
          filledtriangle(window,
                         std::make_tuple(transformed2, model.colours[i]));
        } else if (model.mode == Model::RenderMode::RASTERISE_GOURAD) {
          std::array<glm::vec3, 3> normals;
          std::array<glm::vec3, 3> cs;

          for (size_t j = 0; j < normals.size(); j++) {
            // use vertex normals if they exist
            normals[j] = glm::vec3(triangle_normal(transformedc));
            cs[j] = glm::vec3(transformedc[j]);
          }

          filledtriangle(window, state.light, transformed, cs, normals,
                         model.colours[i]);

        } else if (model.mode == Model::RenderMode::RASTERISE_VERTEX) {
          std::array<glmt::rgbf01, 3> colours;

          for (size_t j = 0; j < colours.size(); j++) {
            float d = glm::length(glm::vec3(state.light.pos) -
                                  glm::vec3(transformedc[j]));
            glm::vec3 r = glm::normalize(glm::vec3(state.light.pos) -
                                         glm::vec3(transformedc[j]));
            glm::vec3 n = glm::vec3(triangle_normal(
                transformedc)); // use vertex normals if they exist
            glm::vec3 c = glm::normalize(
                glm::vec3(transformedc[j])); // in camera space camera, so
                                             // camera is at (0, 0)

            colours[j] = model.colours[i] * phong(state.light, d, r, n, c);
          }

          filledtriangle(window, transformed, colours);
        }
      }
    }
  }

  if (state.raymarch) { // raymarch
    const size_t MAX_MARCHING_STEPS = 256;
    const float MIN_DIST = 0.0;  // raymarching doesn't have the same problems
    const float MAX_DIST = 10.0; // match far plane from perspective
    const float EPSILON = 0.001;

    // const glmt::vec3w start = glm::inverse(state.view) * glm::vec4(0, 0, 0,
    // 1);
    const glmt::vec3w start =
        glm::vec4(glm::unProject(glm::vec3(0, 0, 0), state.view, state.proj,
                                 glm::vec4(0, 0, window.width, window.height)),
                  1);

    for (unsigned int y = 0; y < window.height; y++) {
      for (unsigned int x = 0; x < window.width; x++) {
        const glm::vec4 ray =
            glm::vec4(glm::normalize(glm::unProject(
                          glm::vec3(x, y, 1), state.view, state.proj,
                          glm::vec4(0, 0, window.width, window.height))),
                      0);

        float dist =
            march(start, ray, MIN_DIST, MAX_DIST, MAX_MARCHING_STEPS, EPSILON);

        if (dist > MAX_DIST - EPSILON) {
          // window.setPixelColour(glmt::vec2p(x, y),
          //                       glmt::rgbf01(0.08f).argb8888());
        } else {
          const glmt::vec3c pos = state.view * (start + dist * ray);
          const float d =
              glm::length(glm::vec3(state.light.pos) - glm::vec3(pos));
          const glm::vec3 r =
              glm::normalize(glm::vec3(state.light.pos) - glm::vec3(pos));
          const glm::vec3 n = normal(start + dist * ray, EPSILON);
          const glm::vec3 c = glm::normalize(glm::vec3(pos));

          glm::vec3 col =
              colour(start + dist * ray) * phong(state.light, d, r, n, c);
          glmt::rgbf01 tm = tm_aces(col);

          glm::vec3 z =
              glm::project(glm::vec3(pos), glm::mat4(1), state.proj,
                           glm::vec4(0, 0, window.width, window.height));
          // window.setPixelColour(glmt::vec2p(x, y), tm.argb8888());
          window.setPixelColour(glmt::vec2p(x, y), 1.f / z.z, tm.argb8888());
        }
      }
    }
  } // end raymarch

  { // light
    glm::vec3 light =
        glm::project(glm::vec3(state.light.pos), glm::mat4(1), state.proj,
                     glm::vec4(0, 0, window.width, window.height));

    // window.setPixelColour(glmt::vec2p(light), glmt::rgbf01(1.f).argb8888());
    window.setPixelColour(glmt::vec2p(light), 1.f / light.z,
                          glmt::rgbf01(1.f).argb8888());
  } // end light
}

void update() {
  state.frame++;

  if (!state.update) {
    return;
  }

  state.logic++;

  switch (state.logic) {
  case 0:
    state.models[0].mode = Model::RenderMode::FILL;
    break;
  case int(FRAMES * 1 / 7):
    state.models[0].mode = Model::RenderMode::RASTERISE_VERTEX;
    break;
  case int(FRAMES * 2 / 7):
    state.models[0].mode = Model::RenderMode::RASTERISE_GOURAD;
    state.models[1].mode = Model::RenderMode::RASTERISE_VERTEX;
    break;
  case int(FRAMES * 3 / 7):
    state.models[0].mode = Model::RenderMode::PATHTRACE;
    state.models[1].mode = Model::RenderMode::RASTERISE_GOURAD;
    break;
  case int(FRAMES * 4 / 7):
    state.models[1].mode = Model::RenderMode::WIREFRAME;
    state.models[2].mode = Model::RenderMode::WIREFRAME_AA;
    break;
  case int(FRAMES * 5 / 7):
    state.raymarch = true;
    state.models[1].mode = Model::RenderMode::WIREFRAME_AA;
    state.models[2].mode = Model::RenderMode::PATHTRACE;
    break;
  case int(FRAMES * 6 / 7):
    state.models[1].mode = Model::RenderMode::PATHTRACE;
    break;
  case int(FRAMES) - 1:
    std::cout << "1 frame away" << std::endl;
    break;
  }

  float delta = (state.logic / FPS); // s since start

  float s2 = glm::sin(delta / 2);
  float s3 = glm::sin(delta / 3);
  float s5 = glm::sin(delta / 5);
  float s7 = glm::sin(delta / 7);
  float s11 = glm::sin(delta / 11);

  // if (state.raymarch) {
  //   state.light.pos = state.view *
  //                     glm::rotate(glm::sin(delta) * 2.f, glm::vec3(1, 0, 0))
  //                     * glm::rotate(glm::cos(delta) * 2.f, glm::vec3(0, 1,
  //                     0)) * glm::vec4(0, 0, -2, 1);
  // } else {
  state.light.pos = state.view * (state.models[0].matrix *
                                      glm::vec4(-0.234011, 5.218497 - 0.318497,
                                                -3.042968, 1) +
                                  glm::vec4(s2, s3 + 2, s5 + 2, 0));
  // }
  state.light.diff_b = 200.f + (s2 * s5) * 50 + 70;
  state.light.spec_b = 3.f + (s2 * s5) * 0.2 + 0.5;
  state.light.ambi_b = 0.14f;

  state.camera.yaw = 0.243 + (s2 * s3 * s7) * 0.3;
  state.camera.pitch = -0.257 + (s3 * s5 * s11) * 0.4;
  state.camera.dist = 5.7 - (1.5 * glm::clamp(state.logic / FRAMES, 0.0, 1.0));

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
        glm::translate(glm::vec3(tp)) * (i != 0 ? fun : glm::mat4(1)) *
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

    case SDLK_m:
      state.raymarch = !state.raymarch;
      std::cout << "raymarch: " << state.raymarch << std::endl;
      break;
    case SDLK_r:
      if (state.orig.empty()) {
        std::cout << "pathtrace " << state.orig.size() << std::endl;
        state.orig.resize(state.models.size());
        for (size_t i = 0; i < state.models.size(); i++) {
          state.orig[i] = state.models[i].mode;
          state.models[i].mode = Model::RenderMode::PATHTRACE;
        }
      }
      break;
    case SDLK_g:
      if (state.orig.empty()) {
        std::cout << "gourad " << state.orig.size() << std::endl;
        state.orig.resize(state.models.size());
        for (size_t i = 0; i < state.models.size(); i++) {
          state.orig[i] = state.models[i].mode;
          state.models[i].mode = Model::RenderMode::RASTERISE_GOURAD;
        }
      }
      break;
    case SDLK_o:
      if (!state.orig.empty()) {
        std::cout << "orig " << state.orig.size() << std::endl;
        for (size_t i = 0; i < state.models.size(); i++) {
          state.models[i].mode = state.orig[i];
        }
        state.orig.clear();
      }
    }
    break;
  case SDL_MOUSEBUTTONDOWN: {
    state.sdl.mouse_down = true;
    glm::vec2 pos{event.button.x, event.button.y};
    std::cout << "MOUSE DOWN { " << pos << " }" << std::endl;

    glm::vec3 ws = glm::unProject(glm::vec3(pos, window.getDepthBuffer(pos)),
                                  state.view, state.proj,
                                  glm::vec4(0, 0, window.width, window.height));
    std::cout << "ws: " << ws << std::endl;

    break;
  }
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
