#include "halo2/source_grenade_scene.hpp"
#include <bit>
#include <cmath>
#include <iostream>
#include <limits>

using namespace halo2;
namespace {
void check(bool value,const char* message){if(!value)throw FormatError(message);}
void equal(Vec3 actual,Vec3 expected,const char* message) {
    check(std::bit_cast<std::array<std::uint32_t,3>>(actual)==
          std::bit_cast<std::array<std::uint32_t,3>>(expected),message);
}
}
int main(){try {
    for(std::int16_t type=0;type<2;++type) {
        NativeGrenadeProjectile old;old.datum=0x12340002u;old.definition=0xe7000000u+type;
        old.type=type;old.position={0,2,10};old.velocity={120,60,-60};
        auto current=old;current.position={4,4,8};current.age=old.age+1;
        const std::array previous{old};
        // Four draws within one 30 Hz interval (120 Hz presentation), without
        // another simulation update. Include both completed endpoints.
        for(unsigned frame=0;frame<=4;++frame) {
            const float fraction=float(frame)/4;
            equal(grenade_render_position(previous,current,fraction),
                  {float(frame),2+float(frame)/2,10-float(frame)/2},
                  "Grenade did not move at each fractional render sample");
        }
        auto next=current;next.position={8,6,6};
        equal(grenade_render_position(previous,current,1),
              grenade_render_position(std::array{current},next,0),
              "Grenade presentation jumped at the next tick boundary");
        // An ordinary bounce reverses velocity without changing identity.
        next.velocity={-120,-60,60};next.flags|=0x800;
        equal(grenade_render_position(std::array{current},next,.5f),{6,5,7},
              "Contact flags prevented position interpolation");
        equal(grenade_render_position({},current,.5f),current.position,
              "New projectile blended from an invented prior position");
        auto changed=old;changed.datum+=0x10000u;
        equal(grenade_render_position(std::array{changed},current,.5f),current.position,
              "Recycled low16 slot blended across full salted owners");
        changed=old;changed.definition++;
        equal(grenade_render_position(std::array{changed},current,.5f),current.position,
              "Changed definition shared a position window");
        changed=old;changed.type=1-type;
        equal(grenade_render_position(std::array{changed},current,.5f),current.position,
              "Changed grenade type shared a position window");
        changed=old;changed.parent=0xe26f0000u;
        equal(grenade_render_position(std::array{changed},current,.5f),current.position,
              "Release blended across an attachment transition");
        changed=current;changed.parent=0xe26f0000u;
        equal(grenade_render_position(previous,changed,.5f),changed.position,
              "Attached projectile acquired released interpolation");
        // Vector order is not identity. A removed/detonated predecessor must
        // not supply the current object's starting position.
        changed=old;changed.datum++;changed.position={100,100,100};
        equal(grenade_render_position(std::array{changed,old},current,.5f),{2,3,9},
              "Reordered projectile bank changed interpolation identity");
        equal(grenade_render_position(std::array{changed},current,0),current.position,
              "Removed owner supplied another projectile's old position");
        changed=current;changed.datum=0xffffffffu;
        equal(grenade_render_position(std::array{changed},changed,.5f),changed.position,
              "NONE owner acquired an interpolation window");
        // Preserve endpoint bits, including negative zero.
        changed=old;changed.position.x=-0.f;
        equal(grenade_render_position(std::array{changed},current,0),changed.position,
              "Previous endpoint position bits changed");
        changed=current;changed.position.x=-0.f;
        equal(grenade_render_position(previous,changed,1),changed.position,
              "Current endpoint position bits changed");
        changed.position={-41.934f,18.173f,7.291f};
        for(float fraction:{.1f,.3f,.7f,.9f})
            equal(grenade_render_position(std::array{changed},changed,fraction),changed.position,
                  "Stationary grenade shimmered between identical positions");
        for(float bad:{-.1f,1.1f,std::numeric_limits<float>::infinity(),
                       std::numeric_limits<float>::quiet_NaN()}) {
            bool rejected=false;
            try{grenade_render_position(previous,current,bad);}catch(const FormatError&){rejected=true;}
            check(rejected,"Invalid fraction extrapolated a projectile");
        }
    }
    std::cout<<"PASS frag/plasma fractional positions, tick continuity, bounce, lifetime/attachment cuts and exact endpoints\n";
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
