#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <vector>

// strange lerp
template <typename T>
std::vector<T> interpolate(T start, T end, std::size_t N) {
  std::vector<T> result;
  result.reserve(N);

  for (std::size_t i = 0; i < N; i++) {
    result.push_back(glm::mix(start, end, static_cast<float>(i) / N));
  }

  return result;
}

template <typename T>
std::vector<glmt::vec2<T>> interpolate(glmt::vec2<T> start, glmt::vec2<T> end,
                                       std::size_t N) {
  auto v2i = interpolate(glm::vec2(start), glm::vec2(end), N);
  std::vector<glmt::vec2<T>> result;
  result.reserve(N);

  std::transform(v2i.begin(), v2i.end(), std::back_inserter(result),
                 [](glm::vec2 v) -> glmt::vec2<T> { return v; });

  return result;
}

#include "glmt.hpp"
#include <glm/gtx/component_wise.hpp>
#include <sdw/window.h>

// naive, why not one of these:
// https://en.wikipedia.org/wiki/Line_drawing_algorithm
template <typename T>
std::vector<glmt::vec2p> naiveline(glmt::vec2<T> start, glmt::vec2<T> end) {
  size_t steps = glm::ceil(glm::compMax(glm::abs(end - start))) + 1;
  auto v2T = interpolate(start, end, steps);
  std::vector<glmt::vec2p> points;
  points.reserve(steps);

  std::transform(v2T.begin(), v2T.end(), std::back_inserter(points),
                 [](glm::vec2 v) -> glmt::vec2p { return v; });

  return points;
}

// template <glmt::COLOUR_SPACE CS>
// void line(sdw::window window, glmt::vec2s start, glmt::vec2s end,
//           glmt::colour<CS> colour) {
//   for (auto const &p : naiveline(start, end)) {
//     window.setPixelColour(p, colour.argb8888());
//   }
// }

#include <algorithm>

// xiaolin wu's AA line algorithm
// translated from wikipedia
// https://en.wikipedia.org/wiki/Xiaolin_Wu's_line_algorithm
void line(sdw::window window, glmt::vec2s start, glmt::vec2s end,
          glmt::rgbf01 colour) {
  // To match functions
  auto plot = [&](int x, int y, float c) -> void {
    glmt::rgbf01 curr =
        glm::vec3(window.getPixelColour(glmt::vec2p(x, y))) / 255.f;

    glmt::rgbf01 next = glm::mix(glm::vec3(curr), glm::vec3(colour), c);
    window.setPixelColour(glmt::vec2p(x, y), next.argb8888());
    // above code does "mix", which looks nice if it's only wireframe or a
    // static background
  };

  // definition as per wikipedia begins
  auto ipart = [](float x) -> float { return glm::floor(x); };
  auto fpart = [](float x) -> float { return glm::fract(x); };
  auto rfpart = [](float x) -> float { return 1 - glm::fract(x); };

  // save from having to cast unsigned int to float
  float x0 = start.x;
  float y0 = start.y;
  float x1 = end.x;
  float y1 = end.y;

  const bool steep = glm::abs(y1 - y0) > glm::abs(x1 - x0);

  if (steep) {
    std::swap(x0, y0);
    std::swap(x1, y1);
  }
  if (x0 > x1) {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }

  float dx = x1 - x0;
  float dy = y1 - y0;
  float gradient = (dx == 0) ? 1 : dy / dx;

  // handle first endpoint
  int xpx11;
  float intery;
  {
    const float xend = glm::round(x0);
    const float yend = y0 + gradient * (xend - x0);
    const float xgap = rfpart(x0 + 0.5);
    xpx11 = xend; // used in main loop
    const int ypx11 = ipart(yend);
    if (steep) {
      plot(ypx11, xpx11, rfpart(yend) * xgap);
      plot(ypx11 + 1, xpx11, fpart(yend) * xgap);
    } else {
      plot(xpx11, ypx11, rfpart(yend) * xgap);
      plot(xpx11, ypx11 + 1, fpart(yend) * xgap);
    }
    intery = yend + gradient; // first y intersection for the main loop
  }

  // handle second endpoint
  int xpx12;
  {
    const float xend = glm::round(x1);
    const float yend = y1 + gradient * (xend - x1);
    const float xgap = rfpart(x1 + 0.5);
    xpx12 = xend; // this will be used in the main loop
    const int ypx12 = ipart(yend);
    if (steep) {
      plot(ypx12, xpx12, rfpart(yend) * xgap);
      plot(ypx12 + 1, xpx12, fpart(yend) * xgap);
    } else {
      plot(xpx12, ypx12, rfpart(yend) * xgap);
      plot(xpx12, ypx12 + 1, fpart(yend) * xgap);
    }
  }

  if (steep) {
    for (int x = xpx11 + 1; x < xpx12; x++) {
      plot(ipart(intery), x, rfpart(intery));
      plot(ipart(intery) + 1, x, fpart(intery));
      intery += gradient;
    }
  } else {
    for (int x = xpx11 + 1; x < xpx12; x++) {
      plot(x, ipart(intery), rfpart(intery));
      plot(x, ipart(intery) + 1, fpart(intery));
      intery += gradient;
    }
  }
}

#include <array>
#include <tuple>

template <glmt::COLOUR_SPACE CS>
void linetriangle(
    sdw::window window,
    std::tuple<std::array<glmt::vec2s, 3>, glmt::colour<CS>> triangle) {
  line(window, std::get<0>(triangle)[0], std::get<0>(triangle)[1],
       std::get<1>(triangle));
  line(window, std::get<0>(triangle)[1], std::get<0>(triangle)[2],
       std::get<1>(triangle));
  line(window, std::get<0>(triangle)[2], std::get<0>(triangle)[0],
       std::get<1>(triangle));
}

// naive line in 3d, with z as depth buffer
// technically this is iso projection with no rotations if points aren't
// transformed via proj matrix as an alternative perf increase try Bresenham in
// 3d https://gist.github.com/yamamushi/5823518
void line(sdw::window window, glmt::vec3s start, glmt::vec3s end,
          glmt::rgbf01 colour) {
  const float steps =
      glm::ceil(glm::compMax(glm::abs(glm::vec2(end) - glm::vec2(start)))) +
      1.f;
  // Perspective projection preserves lines, but does not preserve distances.
  start.z = 1.f / start.z;
  end.z = 1.f / end.z;

  for (size_t i = 0; i < steps; i++) {
    glmt::vec3s p = glm::mix(glm::vec4(start), glm::vec4(end), i / steps);
    window.setPixelColour(glmt::vec2p(p), p.z, colour.argb8888());
  }
}

template <glmt::COLOUR_SPACE CS>
void linetriangle(
    sdw::window window,
    std::tuple<std::array<glmt::vec3s, 3>, glmt::colour<CS>> triangle) {
  line(window, std::get<0>(triangle)[0], std::get<0>(triangle)[1],
       std::get<1>(triangle));
  line(window, std::get<0>(triangle)[1], std::get<0>(triangle)[2],
       std::get<1>(triangle));
  line(window, std::get<0>(triangle)[2], std::get<0>(triangle)[0],
       std::get<1>(triangle));
}

#include <glm/gtc/random.hpp>

std::tuple<std::array<glmt::vec2s, 3>, glmt::rgb888>
randomtriangleinside(sdw::window window) {
  glmt::rgb888 colour(glm::linearRand(0, 255), glm::linearRand(0, 255),
                      glm::linearRand(0, 255));
  std::array<glmt::vec2s, 3> points{
      glmt::vec2s(glm::linearRand<float>(0, window.width - 1),
                  glm::linearRand<float>(0, window.height - 1)),
      glmt::vec2s(glm::linearRand<float>(0, window.width - 1),
                  glm::linearRand<float>(0, window.height - 1)),
      glmt::vec2s(glm::linearRand<float>(0, window.width - 1),
                  glm::linearRand<float>(0, window.height - 1))};

  return std::make_tuple(points, colour);
}

#include <algorithm>

template <glmt::COLOUR_SPACE CS>
void filledtriangleflat(sdw::window window, glmt::vec2s top,
                        glmt::vec2s bottom1, glmt::vec2s bottom2,
                        glmt::colour<CS> colour) {
  // assumes bottom1.y == bottom2.y

  std::size_t dy = glm::ceil(glm::abs(bottom1.y - top.y)) + 1;

  auto line1 = interpolate(top, bottom1, dy);
  auto line2 = interpolate(top, bottom2, dy);
  // oh wouldn't it be nice to have python style zip?
  // well boost has it, but I'm not going to import boost just for this little
  // task
  for (size_t i = 0; i < dy; i++) {
    line(window, line1[i], line2[i], colour);
  }
}

// // why not barycentric coordinates
// //
// http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
// template <glmt::COLOUR_SPACE CS>
// void filledtriangle(
//     sdw::window window,
//     std::tuple<std::array<glmt::vec2s, 3>, glmt::colour<CS>> triangle) {
//   std::sort(std::begin(std::get<0>(triangle)),
//   std::end(std::get<0>(triangle)),
//             [](const glm::vec2 a, const glm::vec2 b) { return a.y < b.y; });

//   glmt::vec2s top = std::get<0>(triangle)[0];
//   glmt::vec2s mid = std::get<0>(triangle)[1];
//   glmt::vec2s bot = std::get<0>(triangle)[2];
//   glmt::vec2s midi(
//       top.x + ((mid.y - top.y) / (bot.y - top.y)) * (bot.x - top.x), mid.y);

//   glmt::colour<CS> colour = std::get<1>(triangle);
//   linetriangle(window, triangle);
//   line(window, mid, midi, colour);
//   filledtriangleflat(window, top, mid, midi, colour);
//   filledtriangleflat(window, bot, mid, midi, colour);
// }

glm::mat3 barycentric(std::array<glm::vec2, 3> t) {
  glm::vec3 t0 = glm::vec3(t[0], 1.0);
  glm::vec3 t1 = glm::vec3(t[1], 1.0);
  glm::vec3 t2 = glm::vec3(t[2], 1.0);
  glm::mat3 R(t0, t1, t2);

  return glm::inverse(R);
}
glm::vec3 barycentric(glm::vec2 p, std::array<glm::vec2, 3> t) {
  return barycentric(t) * glm::vec3(p, 1.0);
}
template <typename T>
glm::vec3 barycentric(glmt::vec2<T> p, std::array<glmt::vec2<T>, 3> t) {
  std::array<glm::vec2, 3> triangle{t[0], t[1], t[2]};
  return barycentric(p, triangle);
}

#include <glm/gtx/component_wise.hpp>

template <glmt::COLOUR_SPACE CS>
void filledtriangle(
    sdw::window window,
    std::tuple<std::array<glmt::vec2s, 3>, glmt::colour<CS>> triangle) {
  glmt::bound2s bounds(std::get<0>(triangle).begin(),
                       std::get<0>(triangle).end());
  // TODO: glmt::bound2::operator+ // largest bound which fits both
  // TODO: glmt::bound2::operator- // smallest bound which fits both
  bounds.min.x = glm::max(glm::floor(bounds.min.x), 0.f);
  bounds.min.y = glm::max(glm::floor(bounds.min.y), 0.f);
  bounds.max.x = glm::min(glm::ceil(bounds.max.x), (float)window.width);
  bounds.max.y = glm::min(glm::ceil(bounds.max.y), (float)window.height);

  for (int y = bounds.min.y; y <= bounds.max.y; y++) {
    for (int x = bounds.min.x; x <= bounds.max.x; x++) {
      glm::vec3 bc = barycentric(glmt::vec2s(x, y), std::get<0>(triangle));
      if (bc[0] < 0 || bc[1] < 0 || bc[2] < 0) {
        // outside of triangle
        continue;
      }
      window.setPixelColour(glmt::vec2p(x, y),
                            std::get<1>(triangle).argb8888());
    }
  }
}

void texturedtriangle(sdw::window window, std::array<glmt::vec2s, 3> tri,
                      std::array<glmt::vec2t, 3> tex, glmt::PPM &ppm) {
  glmt::bound2s bounds(tri.begin(), tri.end());
  // TODO: glmt::bound2::operator+ // largest bound which fits both
  // TODO: glmt::bound2::operator- // smallest bound which fits both
  bounds.min.x = glm::max(glm::floor(bounds.min.x), 0.f);
  bounds.min.y = glm::max(glm::floor(bounds.min.y), 0.f);
  bounds.max.x = glm::min(glm::ceil(bounds.max.x), (float)window.width);
  bounds.max.y = glm::min(glm::ceil(bounds.max.y), (float)window.height);

  for (int y = bounds.min.y; y <= bounds.max.y; y++) {
    for (int x = bounds.min.x; x <= bounds.max.x; x++) {
      glm::vec3 bc = barycentric(glmt::vec2s(x, y), tri);
      if (bc[0] < 0 || bc[1] < 0 || bc[2] < 0) {
        // outside of triangle
        continue;
      }

      glmt::vec2t tx = bc[0] * glm::vec2(tex[0]) + bc[1] * glm::vec2(tex[1]) +
                       bc[2] * glm::vec2(tex[2]);
      window.setPixelColour(glmt::vec2p(x, y), ppm[tx].argb8888());
    }
  }
}

#include <fstream>

glmt::PPM parse_ppm(const std::string filename) {
  glmt::PPM ppm;
  std::ifstream file(filename.c_str());

  file >> ppm;

  // TODO: should this throw?
  if (file.fail()) {
    std::cerr << "Parsing PPM \"" << filename << "\" failed" << std::endl;
  }
  if (file.peek(), !file.eof()) {
    std::clog << "PPM \"" << filename << "\" has extra data past specification"
              << std::endl;
  }

  return ppm;
}

glmt::OBJ parse_obj(const std::string filename) {
  glmt::OBJ obj;
  std::ifstream file(filename.c_str());

  file >> obj;

  if (file.fail()) {
    std::cerr << "Parsing OBJ \"" << filename << "\" failed" << std::endl;
  }
  if (file.peek(), !file.eof()) {
    std::clog << "OBJ \"" << filename
              << "\" parsing did not consume entire file" << std::endl;
    // std::string str((std::istreambuf_iterator<char>(file)),
    //                 std::istreambuf_iterator<char>());
    // std::cout << "==== REST ====" << std::endl;
    // std::cout << str << std::endl;
    // std::cout << "==== REST ====" << std::endl;
  }

  return obj;
}

#include <sdw/window.h>
#include <string>

sdw::window texture_window(std::string filename, std::string title = "") {
  glmt::PPM ppm = parse_ppm(filename);
  sdw::window texture_window =
      sdw::window(ppm.header.width, ppm.header.height, false,
                  title.empty() ? filename : title);

  for (unsigned int h = 0; h < texture_window.height; h++) {
    for (unsigned int w = 0; w < texture_window.width; w++) {
      // PPM::operator[] is GL_REPEAT by default
      glm::ivec3 c = ppm[glm::ivec2(w, h)] * 255.0f;
      float red = c.r;
      float green = c.g;
      float blue = c.b;

      uint32_t packed =
          (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
      texture_window.setPixelColour(glmt::vec2p(w, h), packed);
    }
  }

  // should this render or let the caller call render?
  texture_window.renderFrame();

  return texture_window; // caller needs to call .close()
}

template <glmt::COLOUR_SPACE CS>
void filledtriangle(
    sdw::window window,
    std::tuple<std::array<glmt::vec3s, 3>, glmt::colour<CS>> triangle) {

  std::array<glmt::vec2s, 3> s_tri{glm::vec2(std::get<0>(triangle)[0]),
                                   glm::vec2(std::get<0>(triangle)[1]),
                                   glm::vec2(std::get<0>(triangle)[2])};
  glmt::bound2s bounds(s_tri.begin(), s_tri.end());

  bounds.min.x = glm::max(glm::floor(bounds.min.x), 0.f);
  bounds.min.y = glm::max(glm::floor(bounds.min.y), 0.f);
  bounds.max.x = glm::min(glm::ceil(bounds.max.x), (float)window.width);
  bounds.max.y = glm::min(glm::ceil(bounds.max.y), (float)window.height);

  for (int y = bounds.min.y; y <= bounds.max.y; y++) {
    for (int x = bounds.min.x; x <= bounds.max.x; x++) {
      glm::vec3 bc = barycentric(glmt::vec2s(x, y), s_tri);
      if (bc[0] <= 0 || bc[1] <= 0 || bc[2] <= 0) {
        // outside of triangle
        continue;
      }

      float zinv = bc[0] / std::get<0>(triangle)[0].z +
                   bc[1] / std::get<0>(triangle)[1].z +
                   bc[2] / std::get<0>(triangle)[2].z;
      window.setPixelColour(glmt::vec2p(x, y), zinv,
                            std::get<1>(triangle).argb8888());
    }
  }
}

struct Intersection {
  glmt::vec3w position;
  float distance;
  int triangleIndex;
};

bool intersect(glm::vec4 start, glm::vec4 dir,
               const std::array<glm::vec4, 3> &triangle,
               Intersection &intersection) {

  glm::vec4 v0 = triangle[0];
  glm::vec4 v1 = triangle[1];
  glm::vec4 v2 = triangle[2];

  glm::vec3 e1 = glm::vec3(v1 - v0);
  glm::vec3 e2 = glm::vec3(v2 - v0);
  glm::vec3 b = glm::vec3(start - v0);
  glm::mat3 A(-glm::vec3(dir), e1, e2);
  glm::vec3 x = glm::inverse(A) * b;

  if (0 <= x[0] &&
      // in front of camera
      0 <= x[1] &&
      // along e1 /*x[1] <= 1 &&*/
      0 <= x[2] &&
      // along e2 /*x[2] <= 1 &&*/
      (x[1] + x[2] <= 1)
      // inside "e3"
  ) {
    intersection.position = start + x[0] * dir;
    intersection.distance = x[0];
    return true;
  }

  return false;
}

bool ClosestIntersection(glm::vec4 start, glm::vec4 dir,
                         const std::vector<std::array<glm::vec4, 3>> &triangles,
                         Intersection &closestIntersection) {
  closestIntersection.distance = std::numeric_limits<float>::infinity();

  for (size_t i = 0; i < triangles.size(); i++) {
    Intersection temp;
    bool triangleIntersects = intersect(start, dir, triangles[i], temp);

    if (triangleIntersects) {
      if (temp.distance < closestIntersection.distance) {
        closestIntersection.distance = temp.distance;
        closestIntersection.position = temp.position;
        closestIntersection.triangleIndex = i;
      }
    }
  }

  return closestIntersection.distance < std::numeric_limits<float>::infinity();
}

// https://en.wikipedia.org/wiki/Tone_mapping
// https://github.com/tizian/tonemapper
// https://64.github.io/tonemapping/
// rgbf0inf tone mapping stuff, technically the return types are glmt::rgbf01
// TODO: glmt::rgbf0inf types
glm::vec3 tm_basic(glm::vec3 colour) {
  colour.x /= colour.x + 1;
  colour.y /= colour.y + 1;
  colour.z /= colour.z + 1;
  return colour;
}

// john hable's uncharted 2 tone mapping function
// http://filmicworlds.com/blog/filmic-tonemapping-operators/
float hable(float x) {
  float A = 0.15;
  float B = 0.50;
  float C = 0.10;
  float D = 0.20;
  float E = 0.02;
  float F = 0.30;

  return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

glm::vec3 tm_hable_basic(glm::vec3 colour) {
  colour.x = hable(colour.x);
  colour.y = hable(colour.y);
  colour.z = hable(colour.z);
  return colour;
}

glm::vec3 tm_hable(glm::vec3 colour) {
  float sig = glm::max(colour.r, glm::max(colour.g, colour.b));
  float luma = glm::dot(colour, glm::vec3(0.2126, 0.7152, 0.0722));
  float coeff = glm::max(sig - 0.18f, 1e-6f) / glm::max(sig, 1e-6f);
  coeff = glm::pow(coeff, 20.0f);

  // Update the original colour and signal
  colour = glm::mix(colour, glm::vec3(luma), coeff);
  sig = glm::mix(sig, luma, coeff);

  // Perform tone-mapping
  colour *= glm::vec3(hable(sig) / sig);

  return colour;
}

glm::vec3 tm_clamp(glm::vec3 colour) {
  return glm::clamp(colour, glm::vec3(0), glm::vec3(1));
}

// https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
glm::vec3 RRTAndODTFit(glm::vec3 v) {
  glm::vec3 a = v * (v + 0.0245786f) - 0.000090537f;
  glm::vec3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
  return a / b;
}

glm::vec3 tm_aces(glm::vec3 color) {
  // sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
  const glm::mat3 ACESInputMat{{0.59719, 0.35458, 0.04823},
                               {0.07600, 0.90834, 0.01566},
                               {0.02840, 0.13383, 0.83777}};

  // ODT_SAT => XYZ => D60_2_D65 => sRGB
  const glm::mat3 ACESOutputMat{{1.60475, -0.53108, -0.07367},
                                {-0.10208, 1.10813, -0.00605},
                                {-0.00327, -0.07276, 1.07602}};

  // glm::transpose since hlsl code is the wrong order
  color = glm::transpose(ACESInputMat) * color;

  // Apply RRT and ODT
  color = RRTAndODTFit(color);

  color = glm::transpose(ACESOutputMat) * color;

  // Clamp to [0, 1]
  color = glm::clamp(color, glm::vec3(0), glm::vec3(1));

  return color;
}

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
glm::vec3 tm_aces_approx(glm::vec3 colour) {
  colour *= 0.6f;
  float a = 2.51f;
  float b = 0.03f;
  float c = 2.43f;
  float d = 0.59f;
  float e = 0.14f;
  return glm::clamp((colour * (a * colour + b)) /
                        (colour * (c * colour + d) + e),
                    0.0f, 1.0f);
}

// strictly for what is supported for rendering, whereas glmt::OBJ may include
// more data that the renderer does not support
struct Model {
  typedef std::array<glmt::vec3l, 3> triangle;

  std::vector<triangle> triangles;
  std::vector<glmt::rgbf01> colours;

  glm::mat4 matrix; // model matrix with below stuff applied, TODO: proper types

  glm::vec3 centre = glm::vec3(0, 0, 0);
  glm::vec3 scale = glm::vec3(1, 1, 1);
  glm::vec3 position = glm::vec3(0, 0, 0);

  enum class RenderMode {
    NONE,
    WIREFRAME_AA, // draw order matters, but lines are AA
    WIREFRAME,    // draw order doesn't matter but lines are not AA
    FILL,
    PATHTRACE,
    RASTERISE_VERTEX,
    RASTERISE_GOURAD,
    RASTERISE_GOURAD_PATHTRACE, // unimplemented, a large refactor would be
                                // needed but the concept is simple enough
  };

  RenderMode mode = RenderMode::WIREFRAME;
};

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

#include <glm/gtx/transform.hpp>

struct Camera {
  glmt::vec3w target = glm::vec4(0, 0, 0, 1);

  float dist = 5.7f;
  float pitch = -0.257f;
  float yaw = 0.243;
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

struct PointLight {
  glmt::vec3c pos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

  // We'll use the colour space rgbf0inf implicitly
  // a better colour space would make stuff easier (less variables), such as
  // hsv/hsl/lab
  // TODO: add rgbf0inf to glmt
  float ambi_b = 300.f;
  float diff_b = 0.2f; // we can update this using an approximate from
  float spec_b = 50.f;
  glm::vec3 ambi_c = glm::vec3(1, 1, 1);
  glm::vec3 diff_c = glm::vec3(1, 1, 1);
  glm::vec3 spec_c = glm::vec3(1, 1, 1);

  // animating this is quite interesting, but one must consider the model
  // and point of light as making it too large will make diffusion points cross 
  // surface boundaries
  float diffusion = 0.3f; 

  glm::vec3 ambient() const { return ambi_c * ambi_b; }
  glm::vec3 diffuse() const { return diff_c * diff_b; }
  glm::vec3 specular() const { return spec_c * spec_b; }
};

void filledtriangle(sdw::window window, std::array<glmt::vec3s, 3> transformed,
                    std::array<glmt::rgbf01, 3> colours) {

  std::array<glmt::vec2s, 3> s_tri{glm::vec2(transformed[0]),
                                   glm::vec2(transformed[1]),
                                   glm::vec2(transformed[2])};
  glmt::bound2s bounds(s_tri.begin(), s_tri.end());

  bounds.min.x = glm::max(glm::floor(bounds.min.x), 0.f);
  bounds.min.y = glm::max(glm::floor(bounds.min.y), 0.f);
  bounds.max.x = glm::min(glm::ceil(bounds.max.x), (float)window.width);
  bounds.max.y = glm::min(glm::ceil(bounds.max.y), (float)window.height);

  for (int y = bounds.min.y; y <= bounds.max.y; y++) {
    for (int x = bounds.min.x; x <= bounds.max.x; x++) {
      glm::vec3 bc = barycentric(glmt::vec2s(x, y), s_tri);
      if (bc[0] <= 0 || bc[1] <= 0 || bc[2] <= 0) {
        // outside of triangle
        continue;
      }

      float zinv = bc[0] / transformed[0].z + bc[1] / transformed[1].z +
                   bc[2] / transformed[2].z;

      // naive vertex interpolation
      // glm::vec3 col =
      //     bc[0] * colours[0] + bc[1] * colours[1] + bc[2] * colours[2];
      glm::vec3 col(0);
      {
        // perspective corrected col
        for (int i = 0; i < 3; ++i) {
          col += bc[i] * colours[i] / transformed[i].z;
        }
        col /= zinv;
      }
      glmt::rgbf01 c = tm_aces(col);

      window.setPixelColour(glmt::vec2p(x, y), zinv, c.argb8888());
    }
  }
}

// assumes all vec3 are normalized
// d = length of unnormalised r
// r = light position - point position
// n = triangle normal
// c = point position - camera position
glm::vec3 phong(const PointLight &light, float d, glm::vec3 r, glm::vec3 n,
                glm::vec3 c) {
  glm::vec3 ambient = light.ambient();
  glm::vec3 diffuse = light.diffuse() * glm::max(glm::dot(r, n), 0.f) /
                      (4 * glm::pi<float>() * glm::pow(d, 2.f));
  glm::vec3 specular =
      light.specular() *
      glm::pow(glm::max(glm::dot(-c, glm::reflect(-r, n)), 0.f), 128.f);

  return ambient + diffuse + specular;
}

glm::vec4 triangle_normal(std::array<glm::vec4, 3> triangle) {
  glm::vec3 e1 = glm::vec3(triangle[1] - triangle[0]);
  glm::vec3 e2 = glm::vec3(triangle[2] - triangle[0]);
  glm::vec3 normal = glm::normalize(glm::cross(e2, e1));

  // normals should be transformed when the triangle is transformed but using
  // the normal matrix
  return glm::vec4(normal, 1.0);
};

glm::vec3 pathtrace_light(
    const Model &model,
    const std::vector<std::array<glm::vec4, 3>> &triangles, // camera space
    const PointLight &light, const glm::vec4 &ray,
    const Intersection &intersection) {
  const glm::vec3 model_c = model.colours[intersection.triangleIndex];

  // // cheat emmissiveness, add Kd and Ks to Model sometime
  // if (glm::compMin(model_c) == 1) {
  //   return glm::vec3(1e10, 1e10, 1e10);
  // }

  glm::vec4 r = glm::normalize(-intersection.position + light.pos);
  glm::vec4 n = triangle_normal(triangles[intersection.triangleIndex]);
  // TODO: if moved, use <glm/gtx/norm.hpp> length2
  float radius = glm::length(-intersection.position + light.pos);

  Intersection to_light;
  if (ClosestIntersection(light.pos, -r, triangles, to_light)) {
    // light -> triangles for floating point lights normally removes the need to
    // for a small normal bias
    if (to_light.triangleIndex != intersection.triangleIndex) {
      glm::vec3 ambient = light.ambient();
      return (model_c * (ambient));
    }
  }

  return (model_c * phong(light, radius, glm::vec3(r), glm::vec3(n),
                          glm::normalize(glm::vec3(ray))));
}

glmt::rgbf01 pathtrace_light(
    const Model &model,
    const std::vector<std::array<glm::vec4, 3>> &triangles, // camera space
    const PointLight &light, glm::mat4 view, const glm::vec4 &ray,
    const Intersection &intersection) {
  std::vector<std::tuple<glm::vec3, float>> light_samples;

  // should this have a different weighting?
  const float cw = 1 / 6.f;
  const float sw = (1 - cw) / 6.f;
  light_samples.push_back(
      std::make_tuple(light.diffusion * glm::vec3(+0.0f, +0.0f, +0.0f), cw));
  light_samples.push_back(
      std::make_tuple(light.diffusion * glm::vec3(+1.0f, +0.0f, +0.0f), sw));
  light_samples.push_back(
      std::make_tuple(light.diffusion * glm::vec3(+0.0f, +1.0f, +0.0f), sw));
  light_samples.push_back(
      std::make_tuple(light.diffusion * glm::vec3(+0.0f, +0.0f, +1.0f), sw));
  light_samples.push_back(
      std::make_tuple(light.diffusion * glm::vec3(-1.0f, +0.0f, +0.0f), sw));
  light_samples.push_back(
      std::make_tuple(light.diffusion * glm::vec3(+0.0f, -1.0f, +0.0f), sw));
  light_samples.push_back(
      std::make_tuple(light.diffusion * glm::vec3(+0.0f, +0.0f, -1.0f), sw));

  glm::vec3 l_col(0);
  for (const auto light_sample : light_samples) {
    PointLight pl = light;
    // absolutely disgusting, it would be much nicer for light.pos to
    // be in glmt::vec3w
    pl.pos = view * (glm::vec4(std::get<0>(light_sample), 0) +
                     glm::inverse(view) * light.pos);

    l_col += pathtrace_light(model, triangles, pl, ray, intersection) *
             std::get<1>(light_sample);
  }

  return tm_aces(l_col);
}

void filledtriangle(sdw::window window, PointLight light,
                    std::array<glmt::vec3s, 3> ss, std::array<glm::vec3, 3> cs,
                    std::array<glm::vec3, 3> normals, glmt::rgbf01 colour) {

  std::array<glmt::vec2s, 3> s_tri{glm::vec2(ss[0]), glm::vec2(ss[1]),
                                   glm::vec2(ss[2])};
  glmt::bound2s bounds(s_tri.begin(), s_tri.end());

  bounds.min.x = glm::max(glm::floor(bounds.min.x), 0.f);
  bounds.min.y = glm::max(glm::floor(bounds.min.y), 0.f);
  bounds.max.x = glm::min(glm::ceil(bounds.max.x), (float)window.width);
  bounds.max.y = glm::min(glm::ceil(bounds.max.y), (float)window.height);

  for (int y = bounds.min.y; y <= bounds.max.y; y++) {
    for (int x = bounds.min.x; x <= bounds.max.x; x++) {
      glm::vec3 bc = barycentric(glmt::vec2s(x, y), s_tri);
      if (bc[0] <= 0 || bc[1] <= 0 || bc[2] <= 0) {
        // outside of triangle
        continue;
      }

      float zinv = bc[0] / ss[0].z + bc[1] / ss[1].z + bc[2] / ss[2].z;

      glm::vec3 bcs;
      {
        // perspective corrected normal
        for (int i = 0; i < 3; ++i) {
          bcs += bc[i] * cs[i] / ss[i].z;
        }
        bcs /= zinv;
      }
      glm::vec3 bnormal(0);
      {
        // perspective corrected normal
        for (int i = 0; i < 3; ++i) {
          bnormal += bc[i] * normals[i] / ss[i].z;
        }
        bnormal /= zinv;
      }

      float d = glm::length(glm::vec3(light.pos) - bcs);
      glm::vec3 r = glm::normalize(glm::vec3(light.pos) - bcs);
      glm::vec3 n = bnormal;
      glm::vec3 c = glm::normalize(bcs); // in camera space camera, so
                                         // camera is at (0, 0)
      glm::vec3 col = colour * phong(light, d, r, n, c);
      glmt::rgbf01 tm_col = tm_aces(col);

      window.setPixelColour(glmt::vec2p(x, y), zinv, tm_col.argb8888());
    }
  }
}