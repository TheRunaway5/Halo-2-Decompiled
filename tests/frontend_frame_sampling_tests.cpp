#include "halo2/frontend_canvas.hpp"
#include "halo2/frontend_frame_sampling.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>

namespace {
void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
template <class F> void rejects(F function) {
  bool rejected = false;
  try { function(); } catch (const std::invalid_argument &) { rejected = true; }
  require(rejected, "unsupported frame sampling is admitted");
}
float sampled(float u, const halo2::FrontendFrameSampling &bounds) {
  if (bounds.left_u) u = std::max(u, *bounds.left_u);
  if (bounds.right_u) u = std::min(u, *bounds.right_u);
  return u;
}
}
int main() {
  try {
    // Actual mainmenu screen5 framing_center has no sprites or scroll, scale
    // (0,0)->(1,1), x=0,width=1024,U=[0,1]. Its320ms intro/outro animate only
    // alpha, with zero position in every key. These are source cache inputs,
    // not fitted screenshot coordinates; see frame-seam-cpu-diagnosis.json.
    const auto stock = halo2::frontend_frame_sampling(0,1024,0,1,1024);
    require(stock.left_u == .5f/1024 && stock.right_u == 639.5f/1024,
            "stock frame sampling differs from the actual inward source boundary");
    // Controls preserve the old edge admission while exercising translated,
    // scaled, cropped and scrolled quads independently of the source fixture.
    for (const auto &c : std::array<std::array<float,4>,8>{{
             {0,1024,0,1},{20,1024,0,1},{-400,1024,0,1},
             {641,1024,0,1},{-1100,1024,0,1},{-50,800,.1f,.9f},
             {0,640,.25f,.875f},{-2.5f,1024,2,3}}}) {
      const auto actual=halo2::frontend_frame_sampling(c[0],c[1],c[2],c[3],1024);
      for (unsigned side=0; side<2; ++side) {
        const float boundary=side ? 640.f : 0.f;
        const auto value=side ? actual.right_u : actual.left_u;
        const bool touches=c[0]<=boundary && boundary<=c[0]+c[1];
        require(value.has_value()==touches,"frame sampling changes animated boundary admission");
        if (touches) {
          const float expected=c[2]+(boundary-c[0])/c[1]*(c[3]-c[2])+
                               (side ? -.5f : .5f)/1024;
          require(std::bit_cast<std::uint32_t>(*value)==std::bit_cast<std::uint32_t>(expected),
                  "frame sampling changes the existing strip U arithmetic");
        }
      }
    }
    require(!halo2::frontend_frame_sampling(0,0,0,1,1024).left_u &&
            !halo2::frontend_frame_sampling(0,-1024,0,1,1024).right_u,
            "nonpositive-width frame acquires an extension");

    struct Case { unsigned width,height; };
    unsigned samples{}, modified{}, partitions{};
    for (const auto c : std::array<Case,9>{{
             {640,480},{960,720},{1280,720},{1280,800},{1720,720},
             {2560,720},{2560,1370},{1001,601},{1000,800}}}) {
      const auto canvas=halo2::frontend_canvas(c.width,c.height);
      const float right=canvas.left+canvas.width;
      const int first=int(std::ceil(canvas.left-.5f));
      const int last=int(std::ceil(right-.5f));
      const bool extended=canvas.left>0;
      for (unsigned x=0; x<c.width; ++x) {
        const float center=float(x)+.5f;
        const bool left=center<canvas.left;
        const bool middle=center>=canvas.left && center<right;
        const bool outer_right=center>=right;
        require(unsigned(left)+unsigned(middle)+unsigned(outer_right)==1,
                "frame viewports have overlapping or absent pixel centers");
        require(left==(int(x)<first) && middle==(int(x)>=first && int(x)<last) &&
                outer_right==(int(x)>=last),
                "fractional scissor split disagrees with viewport pixel-center coverage");
        ++partitions;
        if (!middle) continue;
        const float logical=(center-canvas.left)/canvas.scale;
        const float original=logical/1024;
        const float corrected=extended ? sampled(original,stock) : original;
        if (!extended || (logical>=.5f && logical<=639.5f))
          require(std::bit_cast<std::uint32_t>(corrected)==
                      std::bit_cast<std::uint32_t>(original),
                  "frame fix changes the4:3 route or interior source sample");
        else {
          require(corrected==((logical<.5f) ? *stock.left_u : *stock.right_u),
                  "frame boundary does not match its constant-U independent strip");
          ++modified;
        }
        ++samples;
      }
    }
    require(modified>0,"named aspects do not exercise a filtered center boundary");
    rejects([]{halo2::frontend_frame_sampling(0,1024,0,1,0);});
    rejects([]{halo2::frontend_frame_sampling(0,1024,1,0,1024);});
    rejects([]{halo2::frontend_frame_sampling(0,1024,0,0,1024);});
    rejects([]{halo2::frontend_frame_sampling(0,1024,0,.0001f,1024);});
    rejects([]{halo2::frontend_frame_sampling(std::numeric_limits<float>::infinity(),1024,0,1,1024);});
    std::cout << "Frame boundary sampling: " << samples << " centered samples, "
              << modified << " local limits, " << partitions << " pixel-center partitions\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n'; return 1;
  }
}
