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

glm::vec3 tm_ACESFitted(glm::vec3 color) {
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