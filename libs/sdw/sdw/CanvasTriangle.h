#include "Colour.h"
#include <iostream>
#include <array>
#include <glm/gtx/io.hpp>

class CanvasTriangle {
public:
  std::array<glm::vec2, 3> vertices;
  Colour colour;

  CanvasTriangle() {}

  CanvasTriangle(glm::vec2 v0, glm::vec2 v1, glm::vec2 v2) {
    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    colour = Colour(255, 255, 255);
  }

  CanvasTriangle(glm::vec2 v0, glm::vec2 v1, glm::vec2 v2, Colour c) {
    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    colour = c;
  }
};

std::ostream &operator<<(std::ostream &os, const CanvasTriangle &triangle) {
  os << "CanvasTriangle = { vertices = [" <<
    triangle.vertices[0] << ", " <<
    triangle.vertices[1] << ", " << 
    triangle.vertices[2] << "]}" << std::endl;
  return os;
}
