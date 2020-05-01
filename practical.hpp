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

template <glmt::COLOUR_SPACE CS>
void line(sdw::window window, glmt::vec2s start, glmt::vec2s end,
          glmt::colour<CS> colour) {
  for (auto const &p : naiveline(start, end)) {
    window.setPixelColour(p.x, p.y, colour.argb8888());
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

// why not barycentric coordinates
// http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
template <glmt::COLOUR_SPACE CS>
void filledtriangle(
    sdw::window window,
    std::tuple<std::array<glmt::vec2s, 3>, glmt::colour<CS>> triangle) {
  std::sort(std::begin(std::get<0>(triangle)), std::end(std::get<0>(triangle)),
            [](const glm::vec2 a, const glm::vec2 b) { return a.y < b.y; });

  glmt::vec2s top = std::get<0>(triangle)[0];
  glmt::vec2s mid = std::get<0>(triangle)[1];
  glmt::vec2s bot = std::get<0>(triangle)[2];
  glmt::vec2s midi(
      top.x + ((mid.y - top.y) / (bot.y - top.y)) * (bot.x - top.x), mid.y);

  glmt::colour<CS> colour = std::get<1>(triangle);
  linetriangle(window, triangle);
  line(window, mid, midi, colour);
  filledtriangleflat(window, top, mid, midi, colour);
  filledtriangleflat(window, bot, mid, midi, colour);
}

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

void texturedtriangle(sdw::window window, std::array<glmt::vec2s, 3> tri,
                      std::array<glmt::vec2t, 3> tex, glmt::PPM &ppm) {
  glmt::bound2s bounds(tri.begin(), tri.end());
  // TODO: glmt::bound2::operator+ // largest bound which fits both
  // TODO: glmt::bound2::operator- // smallest bound which fits both
  bounds.min = glm::max(glm::floor(bounds.min), glm::vec2(0, 0));
  bounds.max =
      glm::min(glm::ceil(bounds.max), glm::vec2(window.width, window.height));

  for (int y = bounds.min.y; y <= bounds.max.y; y++) {
    for (int x = bounds.min.x; x <= bounds.max.x; x++) {
      glm::vec3 bc = barycentric(glmt::vec2s(x, y), tri);
      if (bc[0] < 0 || bc[1] < 0 || bc[2] < 0) {
        // outside of triangle
        continue;
      }

      glmt::vec2t tx = bc[0] * glm::vec2(tex[0]) + bc[1] * glm::vec2(tex[1]) +
                       bc[2] * glm::vec2(tex[2]);
      window.setPixelColour(x, y, ppm[tx].argb8888());
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

  for (int h = 0; h < texture_window.height; h++) {
    for (int w = 0; w < texture_window.width; w++) {
      // PPM::operator[] is GL_REPEAT by default
      glm::ivec3 c = ppm[glm::ivec2(w, h)] * 255.0f;
      float red = c.r;
      float green = c.g;
      float blue = c.b;

      uint32_t packed =
          (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
      texture_window.setPixelColour(w, h, packed);
    }
  }

  // should this render or let the caller call render?
  texture_window.renderFrame();

  return texture_window; // caller needs to call .close()
}