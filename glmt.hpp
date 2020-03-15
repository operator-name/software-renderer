#pragma once

#include <glm/gtc/type_precision.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/io.hpp>

#include <array>
#include <cctype>
#include <iostream>
#include <istream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace glmt {

inline namespace common {
enum class COLOR_SPACE {
  RBG888,
  ARGB8888,
  FLOAT01,
};

// TODO: different colour spaces
class colour : public glm::vec3 {
private:
  std::string _name;

protected:
  void _gen_name() {
    std::ostringstream stream;
    stream << *this << " : colour";
    _name = stream.str();
  }

public:
  using glm::vec3::vec3;

  colour(glm::vec3 colour) : glm::vec3(colour) { _gen_name(); }
  colour(glm::vec3 colour, std::string name) : glm::vec3(colour) {
    _name = name;
  }

  std::string name() {
    if (_name.empty()) {
      _gen_name();
    }
    return _name;
  }
};
} // namespace common

inline namespace d2 {
typedef glm::uvec2::value_type px;
typedef glm::vec2::value_type sc;

template <typename T> class vec2 : public glm::tvec2<T> {
public:
  using glm::tvec2<T>::tvec2;
  vec2(glm::tvec2<T> vec2) : glm::tvec2<T>(vec2) {}
};
template <typename T> class bound2 : public std::array<vec2<T>, 2> {};

typedef vec2<px> vec2p;
typedef vec2<sc> vec2s;

template <typename T>
std::vector<vec2p> naiveline(glmt::vec2<T> start, glmt::vec2<T> end) {
  size_t steps = glm::ceil(glm::compMax(glm::abs(end - start)));

  std::vector<vec2p> points;
  points.reserve(steps);

  for (size_t i = 0; i < steps; i++) {
    points.push_back(glm::round(glm::mix(glm::vec2(start), glm::vec2(end),
                                         static_cast<sc>(i) / steps)));
  }

  return points;
}
} // namespace d2

namespace parser {
template <char x> class ch {
public:
  friend std::istream &operator>>(std::istream &input, ch empty) {
    if (input.peek() == x) {
      input.get();
    } else {
      input.setstate(std::ios::failbit);
    }
    return input;
  }
};

class ws {
  static bool isspace(char ch) {
    return std::isspace(static_cast<unsigned char>(ch));
  }

public:
  friend std::istream &operator>>(std::istream &input, ws empty) {
    if (isspace(input.peek())) {
      input.get();
    } else {
      input.setstate(std::ios::failbit);
    }
    return input;
  }
};

class comment {
public:
  friend std::istream &operator>>(std::istream &input, comment empty) {
    if (input.peek() == '#') {
      char ch;
      do {
        ch = input.get();
      } while (ch != '\n' && ch != '\r');
    }
    return input;
  }
};

class PPM_header {
public:
  size_t width;
  size_t height;
  uint16_t maxval;
  friend std::istream &operator>>(std::istream &input, PPM_header &header) {
    input >> parser::ch<'P'>() >> parser::ch<'6'>() >> parser::ws() >>
        parser::comment();
    input >> header.width >> parser::ws() >> parser::comment();
    input >> header.height >> parser::ws() >> parser::comment();
    input >> header.maxval >> parser::ws() >> parser::comment();

    return input;
  }

  friend std::ostream &operator<<(std::ostream &output,
                                  const PPM_header &header) {
    output << "PPM_header { "
           << ".width = " << header.width << ", "
           << ".height = " << header.height << ", "
           << ".maxval = " << header.maxval << " }";

    return output;
  }
};

template <size_t N> class pixel : public std::array<uint8_t, N> {
public:
  friend std::istream &operator>>(std::istream &input, pixel &px) {
    for (size_t i = 0; i < N; i++) {
      px[i] = input.get();
    }

    return input;
  }
};

template <size_t N> class rgb : public std::array<pixel<N>, 3> {
public:
  friend std::istream &operator>>(std::istream &input, rgb &c) {
    input >> c[0] >> c[1] >> c[2];
    return input;
  }
};
template <> class rgb<1> : public glm::i8vec3 {
public:
  friend std::istream &operator>>(std::istream &input, rgb &c) {
    pixel<1> r;
    pixel<1> g;
    pixel<1> b;

    input >> r >> g >> b;

    c.r = r[0];
    c.g = g[0];
    c.b = b[0];

    return input;
  }
};
template <> class rgb<2> : public glm::i16vec3 {
public:
  friend std::istream &operator>>(std::istream &input, rgb &c) {
    pixel<2> r;
    pixel<2> g;
    pixel<2> b;

    input >> r >> g >> b;

    c.r = (static_cast<uint16_t>(r[0]) << 8) + static_cast<uint16_t>(r[1]);
    c.g = (static_cast<uint16_t>(g[0]) << 8) + static_cast<uint16_t>(g[1]);
    c.b = (static_cast<uint16_t>(b[0]) << 8) + static_cast<uint16_t>(b[1]);

    return input;
  }
};

} // namespace parser
// http://netpbm.sourceforge.net/doc/ppm.html
class PPM : public std::vector<std::vector<glm::vec3>> {

public:
  using std::vector<std::vector<glm::vec3>>::operator[];

  parser::PPM_header header;
  glm::vec3 &operator[](glm::ivec2 ix) {
    // GL_REPEAT
    ix.x %= header.width;
    ix.y %= header.height;
    return (*this)[ix.y][ix.x];
  }
  friend std::istream &operator>>(std::istream &input, PPM &ppm) {
    input >> ppm.header;

    ppm.reserve(ppm.header.height);

    for (size_t h = 0; h < ppm.header.height; h++) {
      ppm[h].reserve(ppm.header.width);
      for (size_t w = 0; w < ppm.header.width; w++) {
        if (ppm.header.maxval < 256) {
          parser::rgb<1> c;
          input >> c;
          ppm[h][w] = glm::vec3(c) / static_cast<float>(ppm.header.maxval);
        } else {
          parser::rgb<2> c;
          input >> c;
          ppm[h][w] = glm::vec3(c) / static_cast<float>(ppm.header.maxval);
        }
      }
    }

    return input;
  }
};
} // namespace glmt