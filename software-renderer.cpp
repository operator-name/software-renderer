#include <sdw/window.h>

#include "glmt.hpp"
#include "practical.hpp"

#include <glm/glm.hpp>

#include <glm/gtx/io.hpp>

#include <cstdlib>

#define N 2
#define WIDTH (320 * N)
#define HEIGHT (240 * N)

void setup();
void draw();
void update();
void handleEvent(SDL_Event event);

sdw::window window;
sdw::window texture_d;

struct State {
  std::tuple<std::array<glmt::vec2s, 3>,
             glmt::colour<glmt::COLOR_SPACE::RGB888>>
      unfilled_triangle;
  std::tuple<std::array<glmt::vec2s, 3>,
             glmt::colour<glmt::COLOR_SPACE::RGB888>>
      filled_triangle;

  struct ModelTriangle {
    std::array<glmt::vec2s, 3> triangle;
    std::array<glmt::vec2t, 3> texture;
    glmt::PPM ppm;
  } modeltriangle;

  glmt::PPM ppm;
} state;

// assumes trangle.vertices[1].y == trangle.vertices[2].y
// void texturedtriangleflat(sdw::window window, ModelTriangle model) {}

int main(int argc, char *argv[]) {
  setup();

  SDL_Event event;
  while (true) {
    // We MUST poll for events - otherwise the window will freeze !
    if (window.pollForInputEvents(&event))
      handleEvent(event);
    update();
    draw();
    // Need to render the frame at the end, or nothing actually gets shown on
    // the screen !
    window.renderFrame();
  }
}

glmt::parser::MTL parse_mtl(const std::string filename) {
  glmt::parser::MTL mtl;
  std::ifstream file(filename.c_str());

  file >> mtl;

  if (file.fail()) {
    std::cerr << "Parsing MTL \"" << filename << "\" failed" << std::endl;
  }
  if (file.peek(), !file.eof()) {
    std::clog << "MTL \"" << filename
              << "\" parsing did not consume entire file" << std::endl;
  }

  return mtl;
}

void setup() {
  {
    std::ifstream file("cornell-box.obj");
    glmt::OBJ obj;

    file >> obj;

    std::string str((std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());
    std::cout << "==== REST ====" << std::endl;
    std::cout << str << std::endl;
    std::cout << "==== REST ====" << std::endl;

    exit(1);
  }

  state.ppm = parse_ppm("texture.ppm");

  state.modeltriangle.ppm = state.ppm;
  state.modeltriangle.triangle = std::array<glmt::vec2s, 3>{
      glm::vec2(160, 10), glm::vec2(300, 230), glm::vec2(10, 150)};
  state.modeltriangle.texture = std::array<glmt::vec2t, 3>{
      glm::vec2(195, 5), glm::vec2(395, 380), glm::vec2(65, 330)};

  // seed random state to be the same each time (for debugging)
  // TODO: add proper random state
  std::srand(0);
  window = sdw::window(WIDTH, HEIGHT, false);
  texture_d =
      sdw::window(state.modeltriangle.ppm.header.width,
                  state.modeltriangle.ppm.header.height, false, "texture.ppm");
  for (int h = 0; h < texture_d.height; h++) {
    for (int w = 0; w < texture_d.width; w++) {
      // PPM::operator[] is GL_REPEAT by default
      glm::ivec3 c = state.ppm[glm::ivec2(w, h)] * 255.0f;
      float red = c.r;
      float green = c.g;
      float blue = c.b;

      uint32_t packed =
          (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
      texture_d.setPixelColour(w, h, packed);
    }
  }
  // unwrap texture space to screen space
  std::array<glmt::vec2s, 3> texture_s{
      glm::vec2(state.modeltriangle.texture[0]),
      glm::vec2(state.modeltriangle.texture[1]),
      glm::vec2(state.modeltriangle.texture[2])};
  linetriangle(
      texture_d,
      std::make_tuple(texture_s,
                      glmt::colour<glmt::COLOR_SPACE::RGB888>(0, 255, 0)));
  texture_d.renderFrame();
}

void draw() {
  // window.clearPixels();
  texturedtriangle(window, state.modeltriangle.triangle,
                   state.modeltriangle.texture, state.modeltriangle.ppm);
  linetriangle(window, std::make_tuple(
                           state.modeltriangle.triangle,
                           glmt::colour<glmt::COLOR_SPACE::RGB888>(255, 0, 0)));
}

void update() {
  // Function for performing animation (shifting artifacts or moving the camera)
}

void handleEvent(SDL_Event event) {
  switch (event.type) {
  case SDL_KEYDOWN:
    switch (event.key.keysym.sym) {
    case SDLK_LEFT:
      std::cout << "LEFT" << std::endl;
      break;
    case SDLK_RIGHT:
      std::cout << "RIGHT" << std::endl;
      break;
    case SDLK_UP:
      std::cout << "UP" << std::endl;
      break;
    case SDLK_DOWN:
      std::cout << "DOWN" << std::endl;
      break;
    case SDLK_c:
      window.clearPixels();
      break;
    case SDLK_b: {

    } break;
    case SDLK_u:
      state.unfilled_triangle = randomtriangleinside(window);
      linetriangle(window, state.unfilled_triangle);
      break;
    case SDLK_f:
      state.filled_triangle = randomtriangleinside(window);
      filledtriangle(window, state.filled_triangle);
      break;
    }
    break;
  case SDL_MOUSEBUTTONDOWN:
    std::cout << "MOUSE CLICKED" << std::endl;
    break;
  }
}
