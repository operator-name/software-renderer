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
#include <sdw/Colour.h>
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

void line(sdw::window window, glmt::vec2s start, glmt::vec2s end,
          Colour colour) {
  uint32_t packed = (255 << 24) + (int(colour.red) << 16) +
                    (int(colour.green) << 8) + int(colour.blue);
  for (auto const &p : naiveline(start, end)) {
    window.setPixelColour(p.x, p.y, packed);
  }
}

#include <array>
#include <tuple>

void linetriangle(sdw::window window,
                  std::tuple<std::array<glmt::vec2s, 3>, Colour> triangle) {
  line(window, std::get<0>(triangle)[0], std::get<0>(triangle)[1],
       std::get<1>(triangle));
  line(window, std::get<0>(triangle)[1], std::get<0>(triangle)[2],
       std::get<1>(triangle));
  line(window, std::get<0>(triangle)[2], std::get<0>(triangle)[0],
       std::get<1>(triangle));
}

#include <glm/gtc/random.hpp>

std::tuple<std::array<glmt::vec2s, 3>, Colour>
randomtriangleinside(sdw::window window) {
  Colour colour(glm::linearRand(0, 255), glm::linearRand(0, 255),
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

void filledtriangleflat(sdw::window window, glmt::vec2s top,
                        glmt::vec2s bottom1, glmt::vec2s bottom2,
                        Colour colour) {
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
void filledtriangle(sdw::window window,
                    std::tuple<std::array<glmt::vec2s, 3>, Colour> triangle) {
  std::sort(std::begin(std::get<0>(triangle)), std::end(std::get<0>(triangle)),
            [](const glm::vec2 a, const glm::vec2 b) { return a.y < b.y; });

  glmt::vec2s top = std::get<0>(triangle)[0];
  glmt::vec2s mid = std::get<0>(triangle)[1];
  glmt::vec2s bot = std::get<0>(triangle)[2];
  glmt::vec2s midi(
      top.x + ((mid.y - top.y) / (bot.y - top.y)) * (bot.x - top.x), mid.y);

  Colour colour = std::get<1>(triangle);
  linetriangle(window, triangle);
  line(window, mid, midi, colour);
  filledtriangleflat(window, top, mid, midi, colour);
  filledtriangleflat(window, bot, mid, midi, colour);
}