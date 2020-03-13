#pragma once

#include <glm/glm.hpp>

#include <vector>

// lerp
template<typename T>
std::vector<T> interpolate(T start, T end, std::size_t N) {
  std::vector<T> result(N);
  // std::size_t delta = std::max(N - 1, 1);
  T step = (end - start) / std::max(N - 1, static_cast<size_t>(1));

  for (std::size_t i = 0; i < N; i++) {
    result[i] = start + step * i;
  }

  return result;
}
template<typename T>
std::vector<glm::tvec2<T>> interpolate(glm::tvec2<T> start, glm::tvec2<T> end, std::size_t N) {
  std::vector<glm::tvec2<T>> result(N);
  // std::size_t delta = std::max(N - 1, 1);
  glm::tvec2<T> step = (end - start) / static_cast<T>(std::max(N - 1, static_cast<size_t>(1)));

  for (std::size_t i = 0; i < N; i++) {
    result[i] = start + step * static_cast<T>(i);
  }

  return result;
}
template<typename T>
std::vector<glm::tvec3<T>> interpolate(glm::tvec3<T> start, glm::tvec3<T> end, std::size_t N) {
  std::vector<glm::tvec3<T>> result(N);
  // std::size_t delta = std::max(N - 1, 1);
  glm::tvec3<T> step = (end - start) / static_cast<T>(std::max(N - 1, static_cast<size_t>(1)));

  for (std::size_t i = 0; i < N; i++) {
    result[i] = start + step * static_cast<T>(i);
  }

  return result;
}
template<typename T>
std::vector<glm::tvec4<T>> interpolate(glm::tvec4<T> start, glm::tvec4<T> end, std::size_t N) {
  std::vector<glm::tvec2<T>> result(N);
  // std::size_t delta = std::max(N - 1, 1);
  glm::tvec4<T> step = (end - start) / static_cast<T>(std::max(N - 1, static_cast<size_t>(1)));

  for (std::size_t i = 0; i < N; i++) {
    result[i] = start + step * static_cast<T>(i);
  }

  return result;
}