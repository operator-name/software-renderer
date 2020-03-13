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

#include <DrawingWindow.h>

void line(DrawingWindow window, CanvasPoint start, CanvasPoint end,
          Colour color) {
  glm::vec2 diff{end.x - start.x, end.y - start.y};
  float steps = glm::max(glm::abs(diff.x), glm::abs(diff.y));
  glm::vec2 stepsize = diff / steps;

  for (float i = 0; i < steps; i++) {

    float x = start.x + stepsize.x * i;
    float y = start.y + stepsize.y * i;

    uint32_t colour = (255 << 24) + (int(color.red) << 16) +
                      (int(color.green) << 8) + int(color.blue);
    window.setPixelColour(glm::round(x), glm::round(y), colour);
  }
}

void triangle(DrawingWindow window, CanvasTriangle triangle) {
  line(window, triangle.vertices[0], triangle.vertices[1], triangle.colour);
  line(window, triangle.vertices[1], triangle.vertices[2], triangle.colour);
  line(window, triangle.vertices[2], triangle.vertices[0], triangle.colour);
}