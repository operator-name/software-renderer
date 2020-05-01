#pragma once

#include <glm/gtc/type_precision.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/io.hpp>

#include <array>
#include <cctype>
#include <fstream>
#include <iostream>
#include <istream>
#include <limits>
#include <map>
#include <numeric>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

namespace glmt {

  inline namespace common {
    enum class COLOUR_SPACE {
      RGB888,
      RGBA8888,
      RBGFLOAT01,
    };

    template <COLOUR_SPACE c> class colour {
    private:
      colour();
    };
    template <> class colour<COLOUR_SPACE::RGBA8888> : public glm::u8vec4 {
    public:
      using glm::u8vec4::u8vec4;
      colour(glm::u8vec4 c) : glm::u8vec4(c){};
      uint32_t argb8888() {
        uint32_t packed = ((*this).a << 24) + ((*this).r << 16) +
                          ((*this).g << 8) + (*this).b;
        return packed;
      }
    };
    template <> class colour<COLOUR_SPACE::RGB888> : public glm::u8vec3 {
    public:
      using glm::u8vec3::u8vec3;
      colour(glm::u8vec3 c) : glm::u8vec3(c){};
      uint32_t argb8888() {
        colour<COLOUR_SPACE::RGBA8888> c(*this, 255);
        return c.argb8888();
      }
    };
    template <> class colour<COLOUR_SPACE::RBGFLOAT01> : public glm::vec3 {
    public:
      using glm::vec3::vec3;
      colour(glm::vec3 c) : glm::vec3(c){};
      uint32_t argb8888() {
        colour<COLOUR_SPACE::RGB888> c(*this * 255.f);
        return c.argb8888();
      };
    };

    typedef colour<COLOUR_SPACE::RGBA8888> rgb8888;
    typedef colour<COLOUR_SPACE::RGB888> rgb888;
    typedef colour<COLOUR_SPACE::RBGFLOAT01> rgbf01;

  } // namespace common

  inline namespace d2 {
    // TODO: consider vec3 for homogeneous coordinates
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

    typedef bound2<px> bound2p; // pixel space bound2
    typedef bound2<sc> bound2s; // screen space bound2
    typedef bound2<uv> bound2t; // texture space bound2
  }                             // namespace d2

  inline namespace d3 {
    // all of these are vec4, but we need a way to distinguish between them
    enum class COORDINATE_SYSTEM_3D {
      LOCAL_SPACE,
      WORLD_SPACE,
      CAMERA_SPACE,
    };

    template <COORDINATE_SYSTEM_3D CS> class vec3 : public glm::vec4 {
    public:
      using glm::vec4::vec4;
      vec3(glm::vec4 vec4) : glm::vec4(vec4) {}
    };

    typedef vec3<COORDINATE_SYSTEM_3D::LOCAL_SPACE>
        vec3l; // vec3 in local space
    typedef vec3<COORDINATE_SYSTEM_3D::WORLD_SPACE>
        vec3w; // vec3 in world space
    typedef vec3<COORDINATE_SYSTEM_3D::CAMERA_SPACE>
        vec3c; // vec3 in camera space

  } // namespace d3

  namespace parser {
    namespace detail {
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

      template <size_t N> class pixel : public std::array<uint8_t, N> {
      public:
        friend std::istream &operator>>(std::istream &input, pixel &px) {
          for (size_t i = 0; i < N; i++) {
            px[i] = input.get();
          }

          return input;
        }

        friend std::ostream &operator<<(std::ostream &output, const pixel &px) {
          for (size_t i = 0; i < N; i++) {
            output.put(px[i]);
          }

          return output;
        }
      };

      template <size_t N> class rgb : public std::array<pixel<N>, 3> {
      public:
        friend std::istream &operator>>(std::istream &input, rgb &c) {
          input >> c[0] >> c[1] >> c[2];
          return input;
        }
        friend std::ostream &operator<<(std::ostream &output, const rgb &c) {
          output << c[0] << c[1] << c[2];

          return output;
        }
      };
      template <> class rgb<1> : public glm::i8vec3 {
      public:
        using glm::i8vec3::i8vec3;
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
        friend std::ostream &operator<<(std::ostream &output, const rgb &c) {
          pixel<1> r;
          pixel<1> g;
          pixel<1> b;

          r[0] = c.r;
          g[0] = c.g;
          b[0] = c.b;

          output << r << g << b;

          return output;
        }
      };
      template <> class rgb<2> : public glm::i16vec3 {
      public:
        using glm::i16vec3::i16vec3;
        friend std::istream &operator>>(std::istream &input, rgb &c) {
          pixel<2> r;
          pixel<2> g;
          pixel<2> b;

          input >> r >> g >> b;

          c.r =
              (static_cast<uint16_t>(r[0]) << 8) + static_cast<uint16_t>(r[1]);
          c.g =
              (static_cast<uint16_t>(g[0]) << 8) + static_cast<uint16_t>(g[1]);
          c.b =
              (static_cast<uint16_t>(b[0]) << 8) + static_cast<uint16_t>(b[1]);

          return input;
        }

        friend std::ostream &operator<<(std::ostream &output, const rgb &c) {
          pixel<2> r;
          pixel<2> g;
          pixel<2> b;

          r[0] = c.r >> 8;
          r[1] = c.r;
          g[0] = c.g >> 8;
          g[1] = c.g;
          b[0] = c.b >> 8;
          b[1] = c.b;

          output << r << g << b;

          return output;
        }
      };

      class ws_until {
      protected:
        std::string _matches;

      public:
        ws_until(std::string chs) { _matches = chs; }

        friend std::istream &operator>>(std::istream &input, ws_until empty) {
          while (empty._matches.find(input.peek()) == std::string::npos) {

            if (isspace(input.peek())) {
              input.get();
            } else {
              input.setstate(std::ios::failbit);
              break;
            }
          }
          return input;
        }
      };

    } // namespace detail
    class PPM_header {
    public:
      size_t width;
      size_t height;
      uint16_t maxval;
      friend std::istream &operator>>(std::istream &input, PPM_header &header) {
        input >> detail::ch<'P'>() >> detail::ch<'6'>() >> detail::ws() >>
            detail::comment();
        input >> header.width >> detail::ws() >> detail::comment();
        input >> header.height >> detail::ws() >> detail::comment();
        input >> header.maxval >> detail::ws() >> detail::comment();

        return input;
      }

      friend std::ostream &operator<<(std::ostream &output,
                                      const PPM_header &header) {
        output << "P6\n# Generated by software-renderer\n"
               << header.width << " " << header.height << "\n"
               << header.maxval << "\n";

        return output;
      }
    };

    // http://paulbourke.net/dataformats/mtl/
    class MTL_colour {
    public:
      colour<COLOUR_SPACE::RBGFLOAT01> rgb;
      friend std::istream &operator>>(std::istream &input, MTL_colour &colour) {
        switch (input.peek()) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          float r;
          float g;
          float b;
          input >> r >> g >> b;
          colour.rgb = glm::vec3(r, g, b);
          break;
        case 's':
        case 'x':
        default:
          input.setstate(std::ios::failbit);
          std::string kw;
          input >> kw;
          std::cout << ".mtl parser does not support colour argument \"" << kw
                    << "\"" << std::endl;
          break;
        }
        return input;
      }
    };

    // http://paulbourke.net/dataformats/mtl/
    class MTL_newmtl {
    public:
      std::string name;
      // MTL_colour Ka; // ambient
      MTL_colour Kd; // diffuse
      // MTL_colour Ks; // specular
      // float Ns;      // specular exponent

      friend std::istream &operator>>(std::istream &input, MTL_newmtl &newmtl) {
        input >> detail::ch<'n'>() >> detail::ch<'e'>() >> detail::ch<'w'>() >>
            detail::ch<'m'>() >> detail::ch<'t'>() >> detail::ch<'l'>() >>
            detail::ch<' '>() >> newmtl.name;
        while (!input.eof() && !input.fail() && input.peek() != 'n') {
          input >> detail::ws_until("KTidNsmbrn");
          switch (input.peek()) {
          case 'n':
            break;
          case 'K': {
            input >> detail::ch<'K'>();
            switch (input.peek()) {
            case 'd':
              input >> detail::ch<'d'>() >> detail::ws() >> newmtl.Kd;
              break;
            default:
              input.setstate(std::ios::failbit);
              std::cout << ".mtl parser does not support statement \"K"
                        << static_cast<char>(input.get()) << "\"" << std::endl;
              break;
            }
            break;
          }
          default: {
            input.setstate(std::ios::failbit);
            std::string kw;
            input >> kw;
            std::cout << ".mtl parser does not support statement \"" << kw
                      << "\"" << std::endl;
            break;
          }
          }
        }

        return input;
      }

      friend std::ostream &operator<<(std::ostream &output,
                                      const MTL_newmtl &newmtl) {
        output << "MTL_newmtl { "
               << ".name = " << newmtl.name << ", "
               << ".Kd = " << newmtl.Kd.rgb << " }";

        return output;
      }
    };

    // http://paulbourke.net/dataformats/mtl/
    class MTL {
    public:
      std::map<std::string, MTL_newmtl> mtls;
      friend std::istream &operator>>(std::istream &input, MTL &mtl) {
        while (!input.eof() && !input.fail()) {
          MTL_newmtl m;
          input >> m;

          mtl.mtls.emplace(m.name, m);
        }

        return input;
      }
      friend std::ostream &operator<<(std::ostream &output, const MTL &mtl) {
        output << "MTL [ ";
        for (const auto &newmtl : mtl.mtls) {
          output << newmtl.second << ", ";
        }
        output << "]";

        return output;
      }
    };

    class OBJ_mtllib {
      // Specifies the material library file for the material definitions
      // set with the usemtl statement. You can specify multiple filenames
      // with mtllib. If multiple filenames are specified, the first file
      // listed is searched first for the material definition, the second
      // file is searched next, and so on.
    protected:
      std::vector<MTL> mtls;

    public:
      OBJ_mtllib(){};
      OBJ_mtllib(std::vector<MTL> mtls) : mtls(mtls){};

      MTL_newmtl operator[](const std::string name) {
        for (const auto &mtl : mtls) {
          auto iter = mtl.mtls.find(name);
          if (iter != mtl.mtls.end()) {
            return iter->second;
          }
        }

        return MTL_newmtl{"undefined", {glm::vec3(0.5, 0, 0.5)}};
      }

      friend std::istream &operator>>(std::istream &input, OBJ_mtllib &mtllib) {
        input >> detail::ch<'m'>() >> detail::ch<'t'>() >> detail::ch<'l'>() >>
            detail::ch<'l'>() >> detail::ch<'i'>() >> detail::ch<'b'>() >>
            detail::ch<' '>();
        while (input.peek() != '\n' && !input.eof() && !input.fail()) {
          std::string filename;
          input >> filename;

          std::ifstream file(filename.c_str());
          MTL mtl;

          file >> mtl;

          if (file.fail()) {
            std::cerr << "Parsing MTL \"" << filename << "\" failed"
                      << std::endl;
            input.setstate(std::ios::failbit);
          }
          if (file.peek(), !file.eof()) {
            std::clog << "MTL \"" << filename
                      << "\" parsing did not consume entire file" << std::endl;
          }

          mtllib.mtls.push_back(mtl);
        }

        return input;
      }

      friend std::ostream &operator<<(std::ostream &output,
                                      const OBJ_mtllib &mtllib) {
        output << "OBJ_mtllib [ ";
        for (const auto &mtl : mtllib.mtls) {
          output << mtl << ", ";
        }
        output << "]";

        return output;
      }
    };

    class OBJ_v {
    public:
      glmt::vec3l v;
      friend std::istream &operator>>(std::istream &input, OBJ_v &v) {
        float x;
        float y;
        float z;
        input >> detail::ch<'v'>() >> x >> y >> z;
        v.v = glm::vec4(x, y, z,
                        1); // Homogeneous coordinates, this is a position
        return input;
      }
      friend std::ostream &operator<<(std::ostream &output, const OBJ_v &v) {
        std::cout << "OBJ_v " << v.v;
        return output;
      }
    };

    class OBJ_f {
    public:
      std::string usemtl;
      std::vector<size_t> vs;
      OBJ_f(std::string usemtl) : usemtl(usemtl) {}
      friend std::istream &operator>>(std::istream &input, OBJ_f &f) {
        input >> detail::ch<'f'>();
        while (input.peek() != '\n' && !input.eof() && !input.fail()) {
          size_t v;

          input >> v >> detail::ch<'/'>();
          f.vs.push_back(v);
        }

        return input;
      }
      friend std::ostream &operator<<(std::ostream &output, const OBJ_f &f) {
        output << "OBJ_f { .usemtl = " << f.usemtl << " .vs = [";
        for (const auto &v : f.vs) {
          output << v << ", ";
        }
        output << "] }";
        return output;
      }
    };

    // http://paulbourke.net/dataformats/obj/
    class OBJ_data {
    public:
      OBJ_mtllib mtllib;
      std::string usemtl;
      std::vector<OBJ_v> vs;
      std::vector<OBJ_f> fs;

      friend std::istream &operator>>(std::istream &input, OBJ_data &data) {
        while (!input.eof() && !input.fail()) {
          input >> detail::ws_until("mouvf");
          switch (input.peek()) {
          case 'm': {
            OBJ_mtllib mtllib(data.mtllib);
            input >> mtllib;
            data.mtllib = mtllib;
            break;
          }
          case 'o': {
            // o object_name
            // *snip*
            // Optional statement; it is not processed by any Wavefront
            // programs. *snip*
            std::string name;
            input >> detail::ch<'o'>() >> name;
            break;
          }
          case 'u': {
            std::string mtlname;
            input >> detail::ch<'u'>() >> detail::ch<'s'>() >>
                detail::ch<'e'>() >> detail::ch<'m'>() >> detail::ch<'t'>() >>
                detail::ch<'l'>() >> detail::ch<' '>() >> mtlname;
            data.usemtl = mtlname;
            break;
          }
          case 'v': {
            OBJ_v v;
            input >> v;
            data.vs.push_back(v);
            break;
          }
          case 'f': {
            OBJ_f f(data.usemtl);
            input >> f;
            data.fs.push_back(f);
            break;
          }
          default: {
            input.setstate(std::ios::failbit);
            std::string kw;
            input >> kw;
            std::cout << ".obj parser does not support statement \"" << kw
                      << "\"" << std::endl;
            break;
          }
          }
        }
        return input;
      }
    };

  } // namespace parser

  // http://netpbm.sourceforge.net/doc/ppm.html
  class PPM {
  protected:
    std::vector<colour<COLOUR_SPACE::RBGFLOAT01>> _body;

  public:
    parser::PPM_header header;
    colour<COLOUR_SPACE::RBGFLOAT01> operator[](glmt::vec2<glmt::uv> ix) const {
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
            parser::detail::rgb<1> c;
            input >> c;
            ppm._body.push_back(glm::vec3(c) /
                                static_cast<float>(ppm.header.maxval));
          } else {
            parser::detail::rgb<2> c;
            input >> c;
            ppm._body.push_back(glm::vec3(c) /
                                static_cast<float>(ppm.header.maxval));
          }
        }
      }

      return input;
    }
    friend std::ostream &operator<<(std::ostream &output, const PPM &ppm) {
      output << ppm.header;

      for (size_t h = 0; h < ppm.header.height; h++) {
        for (size_t w = 0; w < ppm.header.width; w++) {
          if (ppm.header.maxval < 256) {
            parser::detail::rgb<1> c =
                ppm[vec2t(w, h)] * static_cast<float>(ppm.header.maxval);
            output << c;
          } else {
            parser::detail::rgb<2> c =
                ppm[vec2t(w, h)] * static_cast<float>(ppm.header.maxval);
            output << c;
          }
        }
      }

      return output;
    }
  };

  class OBJ {
  public:
    typedef std::array<vec3l, 3> tri_l;
    typedef std::tuple<tri_l, rgbf01> triangle;
    std::vector<triangle> triangles;
    friend std::istream &operator>>(std::istream &input, OBJ &obj) {
      parser::OBJ_data data;
      input >> data;

      for (auto const &f : data.fs) {
        // assume all fs are triangles
        tri_l tl;
        rgbf01 c = data.mtllib[f.usemtl].Kd.rgb;

        for (size_t i = 0; i < tl.size(); i++) {
          tl[i] = data.vs[f.vs[i] - 1].v; // fs are 1 indexed
        }

        obj.triangles.push_back(std::make_tuple(tl, c));
      }

      return input;
    }
  };

} // namespace glmt