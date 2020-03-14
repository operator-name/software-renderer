#pragma once

#include <glm/gtc/type_precision.hpp> 
#include <glm/gtx/io.hpp>

#include <string>
#include <sstream>
// #include <glm/gtx/string_cast.hpp> 

namespace glmt {
  namespace common {
    // TODO: different color spaces
    class colour : public glm::vec3 {
      private:
        std::string _name;
      protected:
        void _gen_name() {
            std::ostringstream stream;
            stream << *this << " : color";
            _name = stream.str();
        }
      public:
        using glm::vec3::vec3;

        colour(glm::vec3 colour) : glm::vec3(colour) {
          _gen_name();
        }
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
  }

  class PPM : public std::vector<std::vector<glm::i8vec3>> {};


}