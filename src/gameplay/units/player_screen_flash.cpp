#include "halo2/player_screen_flash.hpp"
#include "halo2/kernels.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
namespace {
float transition(float x,unsigned curve,const ShaderFunctionTables &tables) {
  if(curve>=tables.transition.size())throw FormatError("Invalid player screen flash curve");
  return curve?fade_curve_fraction(x,tables.transition[curve]):std::clamp(x,0.f,1.f);
}
}
void PlayerScreenFlashPlayer::play(std::span<const PlayerScreenResponse> rows,
    bool shielded,bool seated,float scale,const ShaderFunctionTables &tables) {
  if(!std::isfinite(scale))throw FormatError("Invalid player screen flash scale");
  for(const auto &row:rows)if(row.type!=int(shielded)) {
    // These are compiled TU1.5 overrides, even when the tag requests NONE or
    // another blend type. The first eligible row ends selection at both gates.
    if(row.flash.duration>.05f && row.flash.intensity>.05f) {
      PlayerScreenFlashDefinition flash;
      flash.type=flash.priority=1;
      flash.color=shielded?std::array<float,4>{1,0,.6f,.8f}:std::array<float,4>{1,1,0,0};
      if(row.flash.duration>.5f && !seated) {
        flash.duration=.8f;flash.intensity=.6f;flash.curve=1;
      } else {
        flash.duration=shielded?.2f:.3f;flash.intensity=.3f;
      }
      submit(flash,transition(scale,2,tables));
    }
    return;
  }
}
void PlayerScreenFlashPlayer::submit(const PlayerScreenFlashDefinition &request,float scale) {
  if(request.type<0 || request.type>6 || request.curve<0 || request.curve>=16 ||
      !std::isfinite(request.duration) || request.duration<0 || request.duration>1000 ||
      !std::isfinite(request.intensity) || !std::isfinite(scale) ||
      !std::ranges::all_of(request.color,[](float x){return std::isfinite(x);}))
    throw FormatError("Invalid player screen flash request");
  if((request_.priority>request.priority && float(remaining_)*(1.f/30.f)>request.duration) || !request.type)return;
  request_=request;
  request_.duration=30.f*request_.duration;
  remaining_=std::int16_t(request_.duration); // Original CVTTSS2SI, not nearest-even.
  request_.intensity=request_.intensity*scale;
  fresh_=true;
}
PlayerScreenFlash PlayerScreenFlashPlayer::evaluate(const ShaderFunctionTables &tables) const {
  if(remaining_<=0 && !fresh_)return {};
  const float amount=request_.duration>0
      ?transition(float(remaining_)/request_.duration,unsigned(request_.curve),tables)*request_.intensity
      :request_.intensity;
  return {request_.type,std::clamp(amount,0.f,1.f),request_.color};
}
void PlayerScreenFlashPlayer::advance(std::uint64_t ticks) {
  if(!ticks)return;
  fresh_=false;
  remaining_=std::int16_t(std::max(0,int(remaining_)-int(std::min<std::uint64_t>(ticks,32767))));
}
void PlayerScreenFlashPlayer::clear() {*this={};}
std::uint32_t player_screen_flash_argb(const PlayerScreenFlash &flash) {
  std::uint32_t result{};
  for(unsigned c=0;c<4;++c) {
    const float scaled=flash.color[c]*flash.intensity;
    if(!std::isfinite(scaled) || scaled<0 || scaled>1)throw FormatError("Invalid player screen flash color");
    //134960 multiplies in x87 double precision before FISTP. Rounding the
    //product to float first changes .3f*255 from77 to the tie-to-even76.
    result|=std::uint32_t(std::nearbyint(255.0*double(scaled)))<<(24-c*8);
  }
  return result;
}
}
