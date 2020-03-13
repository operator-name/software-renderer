#include <ModelTriangle.h>
#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>
#include <fstream>
#include <vector>
#include <array>

using namespace std;
using namespace glm;

#define WIDTH 320
#define HEIGHT 240

void draw();
void update();
void handleEvent(SDL_Event event);

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);

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


int main(int argc, char* argv[])
{
  SDL_Event event;
  while(true)
  {
    // We MUST poll for events - otherwise the window will freeze !
    if(window.pollForInputEvents(&event)) handleEvent(event);
    update();
    draw();
    // Need to render the frame at the end, or nothing actually gets shown on the screen !
    window.renderFrame();
  }
}

void draw()
{
  window.clearPixels();
  auto r = glm::vec3(255, 0, 0);
  auto g = glm::vec3(0, 255, 0);
  auto b = glm::vec3(0, 0, 250);
  auto y = r + g;
  auto redtoyellow = interpolate<HEIGHT>(r, y);
  auto bluetogreen = interpolate<HEIGHT>(b, g);
  for(int y=0; y<window.height ;y++) {
    auto row = interpolate<WIDTH>(redtoyellow[y], bluetogreen[y]);
    for(int x=0; x<window.width ;x++) {
      float red = row[x][0];
      float green = row[x][1];
      float blue = row[x][2];
      uint32_t colour = (255<<24) + (int(red)<<16) + (int(green)<<8) + int(blue);
      window.setPixelColour(x, y, colour);
    }
  }
}

void update()
{
  // Function for performing animation (shifting artifacts or moving the camera)
}

void handleEvent(SDL_Event event)
{
  if(event.type == SDL_KEYDOWN) {
    if(event.key.keysym.sym == SDLK_LEFT) cout << "LEFT" << endl;
    else if(event.key.keysym.sym == SDLK_RIGHT) cout << "RIGHT" << endl;
    else if(event.key.keysym.sym == SDLK_UP) cout << "UP" << endl;
    else if(event.key.keysym.sym == SDLK_DOWN) cout << "DOWN" << endl;
  }
  else if(event.type == SDL_MOUSEBUTTONDOWN) cout << "MOUSE CLICKED" << endl;
}
