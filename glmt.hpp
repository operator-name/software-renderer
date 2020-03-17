#pragma once

#include <glm/gtc/type_precision.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/io.hpp>

#include <array>
#include <cctype>
#include <iostream>
#include <istream>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace glmt {

  inline namespace common {
    enum class COLOR_SPACE {
      RGB888,
      RGBA8888,
      FLOAT01,
    };

    typedef float float01;

    // TODO: different colour spaces
    // template <COLOR_SPACE c>
    // class colour : public glm::vec3 {
    // private:
    //   std::string _name;

    // protected:
    //   void _gen_name() {
    //     std::ostringstream stream;
    //     stream << *this << " : colour";
    //     _name = stream.str();
    //   }

    // public:
    //   using glm::vec3::vec3;

    //   colour(glm::vec3 colour) : glm::vec3(colour) { _gen_name(); }
    //   colour(glm::vec3 colour, std::string name) : glm::vec3(colour) {
    //     _name = name;
    //   }

    //   std::string name() {
    //     if (_name.empty()) {
    //       _gen_name();
    //     }
    //     return _name;
    //   }
    // };
    namespace {}

    template <COLOR_SPACE c> class colour {
    private:
      colour();
    };
    template <> class colour<COLOR_SPACE::RGBA8888> : public glm::u8vec4 {
    public:
      using glm::u8vec4::u8vec4;
      uint32_t argb8888() {
        uint32_t packed = ((*this).a << 24) + ((*this).r << 16) +
                          ((*this).g << 8) + (*this).b;
        return packed;
      }
    };
    template <> class colour<COLOR_SPACE::RGB888> : public glm::u8vec3 {
    public:
      using glm::u8vec3::u8vec3;
      uint32_t argb8888() {
        colour<COLOR_SPACE::RGBA8888> c(*this, 255);
        return c.argb8888();
      }
    };
    template <> class colour<COLOR_SPACE::FLOAT01> : public glm::vec3 {
    public:
      uint32_t argb8888() {
        colour<COLOR_SPACE::RGB888> c(*this * 255.f);
        return c.argb8888();
      };
    };

  } // namespace common

  inline namespace d2 {
    typedef glm::uvec2::value_type px; // pixel space
    typedef glm::ivec2::value_type uv; // texture space
    typedef glm::vec2::value_type sc;  // screen space

    template <typename T> class vec2 : public glm::tvec2<T> {
    public:
      using glm::tvec2<T>::tvec2;
      vec2(glm::tvec2<T> vec2) : glm::tvec2<T>(vec2) {}
    };
    template <typename T> class bound2 {
    public:
      vec2<T> min;
      vec2<T> max;
      template <typename Iter> bound2(Iter begin, Iter end) {
        std::array<vec2<T>, 2> minmax{
            vec2<T>(std::numeric_limits<T>::max()),
            vec2<T>(std::numeric_limits<T>::lowest())};
        minmax = std::accumulate(
            begin, end, minmax, [](std::array<vec2<T>, 2> acc, vec2<T> v) {
              acc[0] = glm::min(static_cast<glm::tvec2<T>>(acc[0]),
                                static_cast<glm::tvec2<T>>(v));
              acc[1] = glm::max(static_cast<glm::tvec2<T>>(acc[1]),
                                static_cast<glm::tvec2<T>>(v));
              return acc;
            });
        min = minmax[0];
        max = minmax[1];
      }
    };

    typedef vec2<px> vec2p; // pixel space vec2
    typedef vec2<sc> vec2s; // screen space vec2
    typedef vec2<uv> vec2t; // texture spac vec2
  }                         // namespace d2

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
  class PPM {
  protected:
    std::vector<glm::vec3> _body;

  public:
    parser::PPM_header header;
    glm::vec3 &operator[](glmt::vec2<glmt::uv> ix) {
      // GL_REPEAT
      ix.x %= header.width;
      ix.y %= header.height;
      return _body[ix.y * header.width + ix.x];
    }
    friend std::istream &operator>>(std::istream &input, PPM &ppm) {
      input >> ppm.header;
      ppm._body.reserve(ppm.header.height * ppm.header.width);

      for (size_t h = 0; h < ppm.header.height; h++) {
        for (size_t w = 0; w < ppm.header.width; w++) {
          if (ppm.header.maxval < 256) {
            parser::rgb<1> c;
            input >> c;
            ppm._body.push_back(glm::vec3(c) /
                                static_cast<float>(ppm.header.maxval));
          } else {
            parser::rgb<2> c;
            input >> c;
            ppm._body.push_back(glm::vec3(c) /
                                static_cast<float>(ppm.header.maxval));
          }
        }
      }

      return input;
    }
  };
} // namespace glmt