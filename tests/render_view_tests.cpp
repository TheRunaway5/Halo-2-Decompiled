#include "halo2/render_view.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}
}
int main() {
  try {
    using namespace halo2;
    for (const auto dimensions : {std::array{640.f,480.f}, {1280.f,960.f},
                                  {1280.f,720.f}, {1920.f,1080.f},
                                  {3840.f,2160.f}}) {
      const float w=dimensions[0],h=dimensions[1];
      for (const float scale : {.0625f,.5f,1.f}) {
        const auto c=viewport_constants({13,27,w,h,scale,true});
        // Clip-space corners must land at the active target's pixel centers.
        // Texture dimensions are deliberately absent from this transform.
        const float left=(-c[0][0]+c[0][3])/(w*scale);
        const float right=(c[0][0]+c[0][3])/(w*scale);
        const float top=(c[1][1]+c[1][3])/(h*scale);
        const float bottom=(-c[1][1]+c[1][3])/(h*scale);
        check(std::abs(left-(13.5f*scale+.03125f)/(w*scale))<1e-6f &&
              std::abs(top-(27.5f*scale+.03125f)/(h*scale))<1e-6f &&
              std::abs(right-left-1)<1e-6f &&
              std::abs(bottom-top-1)<1e-6f,
              "projected texture corners drift with viewport resolution/scale");
        check(c[2][2]==16777215 && c[3][0]==(w*.5f+13)*scale+.5f,
              "retail depth/pixel constants changed with resolution");
      }
      const auto ordinary=viewport_constants({13,27,w,h});
      check(viewport_constants({13,27,w,h,.5f,true,true})==ordinary,
            "disabled render scaling changed pixel constants");
      check(viewport_constants({13,27,w,h,.001f,true})==
            viewport_constants({13,27,w,h,.0625f,true}),
            "retail minimum render scale was not retained");
      check(viewport_constants({13,27,w,h,2,true})==ordinary,
            "render scale greater than one enlarged the target");
      check(viewport_constants({13,27,w,h,.5f,true,false,2})==
            std::array<ShaderVector,4>{{{64,0,0,64.5f}, {0,-64,0,64.5f},
                                       {64,-64,16777215,0},{64.5f,64.5f,0,0}}},
            "fixed retail shadow viewport incorrectly follows display resolution");
    }
    std::cout << "Retail viewport resolution/scale/corner checks passed\n";
    return 0;
  } catch (const std::exception &e) {std::cerr << e.what() << '\n';return 1;}
}
