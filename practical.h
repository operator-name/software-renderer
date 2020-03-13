#pragma once

#include <glm/glm.hpp>

#include <array>

// lerp
template<std::size_t N, typename T>
std::array<T, N> interpolate(T start, T end) {
  std::array<T, N> result;
  // std::size_t delta = std::max(N - 1, 1);
  T step = (end - start) / std::max(N - 1, static_cast<size_t>(1));

  for (std::size_t i = 0; i < N; i++) {
    result[i] = start + step * i;
  }

  return result;
}
template<std::size_t N, typename T>
std::array<glm::tvec2<T>, N> interpolate(glm::tvec2<T> start, glm::tvec2<T> end) {
  std::array<glm::tvec2<T>, N> result;
  // std::size_t delta = std::max(N - 1, 1);
  glm::tvec2<T> step = (end - start) / static_cast<T>(std::max(N - 1, static_cast<size_t>(1)));

  for (std::size_t i = 0; i < N; i++) {
    result[i] = start + step * static_cast<T>(i);
  }

  return result;
}
template<std::size_t N, typename T>
std::array<glm::tvec3<T>, N> interpolate(glm::tvec3<T> start, glm::tvec3<T> end) {
  std::array<glm::tvec3<T>, N> result;
  // std::size_t delta = std::max(N - 1, 1);
  glm::tvec3<T> step = (end - start) / static_cast<T>(std::max(N - 1, static_cast<size_t>(1)));

  for (std::size_t i = 0; i < N; i++) {
    result[i] = start + step * static_cast<T>(i);
  }

  return result;
}
template<std::size_t N, typename T>
std::array<glm::tvec4<T>, N> interpolate(glm::tvec4<T> start, glm::tvec4<T> end) {
  std::array<glm::tvec2<T>, N> result;
  // std::size_t delta = std::max(N - 1, 1);
  glm::tvec4<T> step = (end - start) / static_cast<T>(std::max(N - 1, static_cast<size_t>(1)));

  for (std::size_t i = 0; i < N; i++) {
    result[i] = start + step * static_cast<T>(i);
  }

  return result;
}