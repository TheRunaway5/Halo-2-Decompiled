#include "halo2/source_effect_seed_location.hpp"
#include <cmath>

namespace halo2 {
Transform source_effect_seed_location(const Transform *node,Vec3 position,Vec3 forward) {
  if(node) {
    Vec3 offset{};
    if(node->scale!=0.f) {
      offset={position.x-node->position.x,position.y-node->position.y,position.z-node->position.z};
      if(node->scale!=1.f) {
        const float inverse=1.f/node->scale;
        offset={inverse*offset.x,inverse*offset.y,inverse*offset.z};
      }
    }
    const auto dot=[](Vec3 a,Vec3 b){return (a.z*b.z+a.y*b.y)+a.x*b.x;};
    position=node->scale!=0.f?Vec3{dot(node->basis[0],offset),dot(node->basis[1],offset),dot(node->basis[2],offset)}:Vec3{};
    forward={dot(node->basis[0],forward),dot(node->basis[1],forward),dot(node->basis[2],forward)};
  }
  // Complete11F9C0 selects the least absolute component, preserving ties.
  const Vec3 absolute{std::fabs(forward.x),std::fabs(forward.y),std::fabs(forward.z)};
  Vec3 up;
  if(absolute.y>=absolute.x && absolute.z>=absolute.x)up={0.f,forward.z,0.f-forward.y};
  else if(absolute.z>=absolute.y)up={0.f-forward.z,0.f,forward.x};
  else up={forward.y,0.f-forward.x,0.f};
  // Original x87 retains binary64 products/sum/sqrt, then stores magnitude
  // before the SSE reciprocal. Do not replace this with float length().
  const double magnitude=std::sqrt((double(up.z)*double(up.z)+double(up.y)*double(up.y))+double(up.x)*double(up.x));
  if(!(std::fabs(magnitude)<double(.0001f))) {
    const float inverse=1.f/float(magnitude);
    up={inverse*up.x,up.y*inverse,up.z*inverse};
  }
  const Vec3 left{up.y*forward.z-up.z*forward.y,up.z*forward.x-forward.z*up.x,
    forward.y*up.x-up.y*forward.x};
  return {1,{forward,left,up},position};
}
}
