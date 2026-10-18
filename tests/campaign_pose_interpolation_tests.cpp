#include "halo2/model_node_interpolation.hpp"
#include "halo2/math.hpp"
#include "halo2/transform.hpp"
#include <bit>
#include <iostream>
#include <set>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
void near(float value,float expected,const char *message){check(std::abs(value-expected)<.0001f,message);}
auto pose(unsigned tick) {
    Transform root,child;root.position.x=float(tick)*2;child.position={1,0,0};
    return std::array{root,compose_transform(root,child)};
}
void render_gaps() {
    RenderModel model;model.bones.resize(2);model.bones[1].parent=0;
    // The campaign collector publishes at draw time, not on every game step.
    // Alternate 1/2/3 completed ticks between draws as real slow frames do.
    ModelNodeInterpolation history;history.capture_render(10,pose(10));
    unsigned tick=10;
    for(unsigned gap:{1u,2u,3u,1u,2u,1u}) {
        tick+=gap;history.capture_render(tick,pose(tick));
        check(history.paired(),"Campaign render gap discarded the completed NPC pose history");
        std::set<float> samples;
        for(float f:{0.f,.125f,.25f,.5f,.75f,.875f,1.f}) {
            const auto shown=history.sample_render(model,f);
            near(shown[0].position.x,2*(float(tick-1)+f),"NPC interpolation sampled the wrong render tick across a slow frame");
            near(length(shown[1].position-shown[0].position),1,"Interpolated NPC bone changed length");
            samples.insert(shown[0].position.x);
        }
        check(samples.size()==7,"NPC pose remained capped between completed frames");
        // Several high-refresh draws can see the same completed tick.
        history.capture_render(tick,pose(tick));
        near(history.sample_render(model,.5f)[0].position.x,2*float(tick)-1,"Same-tick view destroyed the NPC history");
    }
    history.capture_render(3,pose(3));check(!history.paired(),"Checkpoint rewind blended a future NPC pose");
    history=ModelNodeInterpolation{};history.capture_render(50,pose(50));
    near(history.sample_render(model,0)[0].position.x,100,"New scene/lifetime retained an old render pose");
    ModelNodeInterpolation ticks;ticks.capture(10,pose(10));ticks.capture(12,pose(12));
    check(!ticks.paired(),"Simulation-tick capture silently accepted missing authoritative endpoints");
}
}
int main(){try{render_gaps();std::cout<<"Campaign NPC render-gap interpolation passed\n";return 0;}
catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
