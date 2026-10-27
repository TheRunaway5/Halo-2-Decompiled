// Offline maintenance only: execute the current TU1.5 native state producers,
// preserving complete program words before the Python registry normalization.
#include "halo2/source_scene_copy.hpp"
#include "halo2/scope_copy.hpp"
#include "halo2/source_decal_draw.hpp"
#include "halo2/source_decal_pass.hpp"
#include "halo2/source_contrail_state.hpp"
#include "halo2/source_lens_program_tu15.hpp"
#include "halo2/motion_sensor.hpp"
#include <iostream>

namespace {
bool first=true;
void emit(const char* family,const std::array<std::uint32_t,60>& words,
          unsigned a=0,unsigned b=0,unsigned c=0) {
    if(!first)std::cout<<',';first=false;
    std::cout<<"{\"family\":\""<<family<<"\",\"arguments\":["<<a<<','<<b<<','<<c<<"],\"words\":[";
    for(unsigned n=0;n<words.size();++n){if(n)std::cout<<',';std::cout<<words[n];}
    std::cout<<"]}";
}
}
int main() {
    using namespace halo2;
    std::cout<<'[';
    emit("scene-copy",source_scene_copy_program().words);
    emit("scope-copy",scope_copy_frame({0,0,1,1}).program.words);
    emit("lens",source_lens_pixel_words);
    emit("hud",source_motion_sensor_final_program_tu15().words);
    for(unsigned layer=0;layer<6;++layer)emit("decal",source_decal_layer_state_tu15(layer).pixel.words,layer);
    // NativeWorldEffects also submits the original projected layers through
    // world.frag with its fixed vertex inputs. This is a distinct compiled
    // wrapper role; the cached-decal family above has a different draw ABI.
    for(unsigned layer=2;layer<=5;++layer)emit("world-fixed",source_decal_pass(layer).program.words,layer);
    // These are complete control-flow equivalence classes of the literal
    // producer: only properties&2 and declaration&4 are consumed; blends0..7
    // are explicit and every larger unsigned16 value takes the same branch.
    for(unsigned properties:{0u,2u})for(unsigned declaration:{0u,4u})
      for(unsigned blend=0;blend<=8;++blend)
        emit("contrail",source_contrail_pixel_program(blend,properties,declaration).words,
             blend,properties,declaration);
    std::cout<<"]\n";
}
