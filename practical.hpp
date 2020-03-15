#pragma once

#include <glm/glm.hpp>

#include <vector>

// strange lerp
template <typename T>
std::vector<T> interpolate(T start, T end, std::size_t N) {
  std::vector<T> result(N);
  T step = (end - start) / std::max(N - 1, static_cast<size_t>(1));

  for (std::size_t i = 0; i < N; i++) {
    result[i] = start + step * i;
  }

  return result;
}
template <typename T>
std::vector<glm::tvec2<T>> interpolate(glm::tvec2<T> start, glm::tvec2<T> end,
                                       std::size_t N) {
  std::vector<glm::tvec2<T>> result(N);
  glm::tvec2<T> step =
      (end - start) / static_cast<T>(std::max(N - 1, static_cast<size_t>(1)));

  for (std::size_t i = 0; i < N; i++) {
    result[i] = start + step * static_cast<T>(i);
  }

  return result;
}
template <typename T>
std::vector<glm::tvec3<T>> interpolate(glm::tvec3<T> start, glm::tvec3<T> end,
                                       std::size_t N) {
  std::vector<glm::tvec3<T>> result(N);
  glm::tvec3<T> step =
      (end - start) / static_cast<T>(std::max(N - 1, static_cast<size_t>(1)));

  for (std::size_t i = 0; i < N; i++) {
    result[i] = start + step * static_cast<T>(i);
  }

  return result;
}
template <typename T>
std::vector<glm::tvec4<T>> interpolate(glm::tvec4<T> start, glm::tvec4<T> end,
                                       std::size_t N) {
  std::vector<glm::tvec2<T>> result(N);
  glm::tvec4<T> step =
      (end - start) / static_cast<T>(std::max(N - 1, static_cast<size_t>(1)));

  for (std::size_t i = 0; i < N; i++) {
    result[i] = start + step * static_cast<T>(i);
  }

  return result;
}

#include <glm/gtx/component_wise.hpp>
#include <sdw/Colour.h>
#include <sdw/window.h>

// naive, why not one of these:
// https://en.wikipedia.org/wiki/Line_drawing_algorithm
void line(sdw::window window, glm::vec2 start, glm::vec2 end, Colour colour) {
  glm::vec2 diff = end - start;
  float steps = glm::compMax(glm::abs(diff));
  glm::vec2 step = diff / steps;

  for (float i = 0; i < steps; i++) {
    glm::vec2 p = start + step * i;

    uint32_t packed = (255 << 24) + (int(colour.red) << 16) +
                      (int(colour.green) << 8) + int(colour.blue);
    window.setPixelColour(glm::round(p.x), glm::round(p.y), packed);
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
  glm::vec2 top(triangle.vertices[0].x, triangle.vertices[0].y);
  glm::vec2 bottom1(triangle.vertices[1].x, triangle.vertices[1].y);
  glm::vec2 bottom2(triangle.vertices[2].x, triangle.vertices[2].y);

  std::size_t dy = glm::abs(bottom1.y - top.y) + 2;

  auto line1 = interpolate(top, bottom1, dy);
  auto line2 = interpolate(top, bottom2, dy);
  // oh wouldn't it be nice to have python style zip?
  // well boost has it, but I'm not going to import boost just for this little
  // task
  for (size_t i = 0; i < dy; i++) {
    line(window, line1[i], line2[i], triangle.colour);
  }
}

// why not barycentric coordinates
// http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
void filledtriangle(sdw::window window, CanvasTriangle triangle) {
  std::sort(std::begin(triangle.vertices), std::end(triangle.vertices),
            [](const glm::vec2 &a, const glm::vec2 &b) { return a.y < b.y; });

  glm::vec2 top(triangle.vertices[0].x, triangle.vertices[0].y);
  glm::vec2 mid(triangle.vertices[1].x, triangle.vertices[1].y);
  glm::vec2 bot(triangle.vertices[2].x, triangle.vertices[2].y);

  glm::vec2 midi(top.x + ((mid.y - top.y) / (bot.y - top.y)) * (bot.x - top.x),
                 mid.y);

  CanvasTriangle top_triangle(triangle.vertices[0], triangle.vertices[1], midi,
                              triangle.colour);
  CanvasTriangle bot_triangle(triangle.vertices[2], triangle.vertices[1], midi,
                              triangle.colour);

  filledtriangleflat(window, top_triangle);
  filledtriangleflat(window, bot_triangle);
}