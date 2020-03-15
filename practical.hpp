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
                 [](glm::vec2 v) -> glmt::vec2<T> { return glmt::vec2<T>(v); });

  return result;
}

#include "glmt.hpp"
#include <glm/gtx/component_wise.hpp>
#include <sdw/Colour.h>
#include <sdw/window.h>

// naive, why not one of these:
// https://en.wikipedia.org/wiki/Line_drawing_algorithm
void line(sdw::window window, glmt::vec2s start, glmt::vec2s end,
          Colour colour) {
  uint32_t packed = (255 << 24) + (int(colour.red) << 16) +
                    (int(colour.green) << 8) + int(colour.blue);
  for (auto const &p : glmt::naiveline(start, end)) {
    window.setPixelColour(p.x, p.y, packed);
  }
}

void linetriangle(sdw::window window, CanvasTriangle triangle) {
  line(window, triangle.vertices[0], triangle.vertices[1], triangle.colour);
  line(window, triangle.vertices[1], triangle.vertices[2], triangle.colour);
  line(window, triangle.vertices[2], triangle.vertices[0], triangle.colour);
}

#include <glm/gtc/random.hpp>

CanvasTriangle randomtriangleinside(sdw::window window) {
  CanvasTriangle triangle;

  triangle.colour = Colour(glm::linearRand(0, 255), glm::linearRand(0, 255),
                           glm::linearRand(0, 255));
  for (auto &vertex : triangle.vertices) {
    // linearRand is [a, b]
    // window is [a, b)
    vertex.x = glm::linearRand<float>(0, window.width - 1);
    vertex.y = glm::linearRand<float>(0, window.height - 1);
  }

  return triangle;
}

#include <algorithm>

// assumes trangle.vertices[1].y == trangle.vertices[2].y
void filledtriangleflat(sdw::window window, CanvasTriangle triangle) {
  glmt::vec2s top(triangle.vertices[0].x, triangle.vertices[0].y);
  glmt::vec2s bottom1(triangle.vertices[1].x, triangle.vertices[1].y);
  glmt::vec2s bottom2(triangle.vertices[2].x, triangle.vertices[2].y);

  std::size_t dy = glm::ceil(glm::abs(bottom1.y - top.y));

  auto line1 = interpolate(top, bottom1, dy);
  auto line2 = interpolate(top, bottom2, dy);
  // oh wouldn't it be nice to have python style zip?
  // well boost has it, but I'm not going to import boost just for this little
  // task
  for (size_t i = 0; i < dy; i++) {
    line(window, line1[i], line2[i], triangle.colour);
  }
  linetriangle(window, triangle);
}

// why not barycentric coordinates
// http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
void filledtriangle(sdw::window window, CanvasTriangle triangle) {
  std::sort(std::begin(triangle.vertices), std::end(triangle.vertices),
            [](const glm::vec2 &a, const glm::vec2 &b) { return a.y < b.y; });

  glmt::vec2s top(triangle.vertices[0].x, triangle.vertices[0].y);
  glmt::vec2s mid(triangle.vertices[1].x, triangle.vertices[1].y);
  glmt::vec2s bot(triangle.vertices[2].x, triangle.vertices[2].y);

  glmt::vec2s midi(
      top.x + ((mid.y - top.y) / (bot.y - top.y)) * (bot.x - top.x), mid.y);

  CanvasTriangle top_triangle(triangle.vertices[0], triangle.vertices[1], midi,
                              triangle.colour);
  CanvasTriangle bot_triangle(triangle.vertices[2], triangle.vertices[1], midi,
                              triangle.colour);

  filledtriangleflat(window, top_triangle);
  filledtriangleflat(window, bot_triangle);
}