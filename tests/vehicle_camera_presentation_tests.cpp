#include "halo2/multiplayer_presentation.hpp"
#include "halo2/render_interpolation.hpp"
#include "halo2/math.hpp"
#include <bit>
#include <cstring>
#include <iostream>
#include <limits>

using namespace halo2;
namespace {
void check(bool value,const char *message) {if(!value)throw FormatError(message);}
void near(float a,float b,const char *message) {check(std::abs(a-b)<.0001f,message);}
void near(Vec3 a,Vec3 b,const char *message) {check(length(a-b)<.0001f,message);}
bool exact(const NativeDeathCameraView &a,const NativeDeathCameraView &b) {
    return !std::memcmp(&a.position,&b.position,sizeof(Vec3)) &&
        !std::memcmp(&a.forward,&b.forward,sizeof(Vec3)) && !std::memcmp(&a.up,&b.up,sizeof(Vec3)) &&
        std::bit_cast<std::uint32_t>(a.horizontal_fov)==std::bit_cast<std::uint32_t>(b.horizontal_fov) &&
        std::bit_cast<std::uint32_t>(a.aim_displacement)==std::bit_cast<std::uint32_t>(b.aim_displacement) &&
        a.camera_mode==b.camera_mode;
}
MultiplayerState seated(std::uint32_t tick) {
    MultiplayerState a;a.tick=tick;a.source_body_poses_authoritative=true;
    for(unsigned i=0;i<4;++i) {
        auto &p=a.players[i];p.connected=true;p.source_player=0x10000+i;p.source_unit=0x20000+i;
        p.source_parent=0x30002;p.source_seat=i;
        p.source_vehicle_camera=NativeDeathCameraView{{-3,float(i),2},{1,0,0},{0,0,1},1.2f,-0.f,1};
    }
    a.source_body_poses.push_back({0x30002,{Transform{}}});
    return a;
}
void moving_seat() {
    const auto a=seated(10);const auto &p=a.players[0];
    auto b=a;++b.tick;b.players[0].position={2,0,0};
    b.players[0].source_vehicle_camera->position.x+=2;
    b.source_body_poses.front().nodes.front().position.x=2;
    SourceWorldInterpolation bodies;
    capture_multiplayer_body_poses(bodies,a,a,true);capture_multiplayer_body_poses(bodies,a,b);
    for(float fraction:{0.f,.25f,.5f,.75f,1.f}) {
        const auto shown=multiplayer_presentation(a,b,fraction);
        const auto root=bodies.sample(p.source_parent,fraction).front();
        near(shown.players[0].source_vehicle_camera->position-root.position,{-3,0,2},
            "Vehicle camera jitters relative to the interpolated vehicle between completed ticks");
    }
}
void orbit_and_endpoints() {
    const auto a=seated(10);auto b=a;++b.tick;
    for(unsigned view=0;view<4;++view) {
        auto &camera=*b.players[view].source_vehicle_camera;
        camera.forward={0,1,0};camera.up={1,0,0};camera.horizontal_fov=1.4f;camera.aim_displacement=.2f;
    }
    for(unsigned view=0;view<4;++view) {
        const auto &before=*a.players[view].source_vehicle_camera,&after=*b.players[view].source_vehicle_camera;
        check(exact(*multiplayer_presentation(a,b,0).players[view].source_vehicle_camera,before),
            "Previous camera endpoint changed original observer words");
        check(exact(*multiplayer_presentation(a,b,1).players[view].source_vehicle_camera,after),
            "Current camera endpoint changed original observer words");
        const auto half=*multiplayer_presentation(a,b,.5f).players[view].source_vehicle_camera;
        check(length(half.forward-before.forward)>.1f && length(half.forward-after.forward)>.1f,
            "Parked vehicle orbit snapped between completed source directions");
        near(length(half.forward),1,"Camera orbit shrank the forward basis");
        near(length(half.up),1,"Camera roll shrank the up basis");
        near(dot(half.forward,half.up),0,"Camera orbit sheared its basis");
        near(half.horizontal_fov,1.3f,"Vehicle lens stepped between ticks");
        near(half.aim_displacement,.1f,"Vehicle aim displacement stepped between ticks");
        check(half.camera_mode==1,"Camera interpolation changed the source perspective");
    }
    const auto stationary=multiplayer_presentation(a,seated(11),.5f);
    check(exact(*stationary.players[0].source_vehicle_camera,*a.players[0].source_vehicle_camera),
        "Stationary camera changed original words");
    check(exact(*a.players[0].source_vehicle_camera,*seated(10).players[0].source_vehicle_camera),
        "Camera sampling mutated an authoritative endpoint");
    auto wrap_a=a,wrap_b=seated(11);
    wrap_a.players[0].source_vehicle_camera->forward=direction(3.1f);
    wrap_b.players[0].source_vehicle_camera->forward=direction(-3.1f);
    near(multiplayer_presentation(wrap_a,wrap_b,.5f).players[0].source_vehicle_camera->forward,{-1,0,0},
        "Camera orbit spun the long way across yaw wrap");
}
void lifecycle_cuts() {
    const auto a=seated(10);auto b=a;++b.tick;
    b.players[0].position={2,0,0};b.players[0].source_vehicle_camera->position.x+=2;
    const auto cut=[&](const MultiplayerState &before,const MultiplayerState &after) {
        for(float f:{0.f,.25f,.5f,.75f,1.f}) {
            const auto actual=multiplayer_presentation(before,after,f).players[0].source_vehicle_camera;
            const auto expected=after.players[0].source_vehicle_camera;
            check(bool(actual)==bool(expected) && (!expected || exact(*actual,*expected)),
                "Vehicle camera blended across a lifetime, seat, perspective or missing-tick boundary");
        }
    };
    auto changed=b;changed.players[0].connected=false;cut(a,changed);
    changed=b;changed.players[0].dead=true;cut(a,changed);
    changed=b;changed.players[0].source_player+=0x10000;cut(a,changed);
    changed=b;changed.players[0].source_unit+=0x10000;cut(a,changed);
    changed=b;changed.players[0].source_parent+=0x10000;cut(a,changed);
    changed=b;++changed.players[0].source_seat;cut(a,changed);
    changed=b;changed.players[0].source_parent=null_datum;changed.players[0].source_seat=-1;cut(a,changed);
    changed=b;++changed.players[0].source_teleport_serial;cut(a,changed);
    changed=b;changed.players[0].position.x=20;cut(a,changed);
    changed=b;changed.players[0].source_vehicle_camera->camera_mode=0;cut(a,changed);
    changed=b;changed.players[0].source_vehicle_camera->camera_mode=3;cut(a,changed);
    changed=b;changed.players[0].source_vehicle_camera.reset();cut(a,changed);
    changed=a;changed.players[0].source_vehicle_camera.reset();cut(changed,b);
    changed=b;++changed.tick;cut(a,changed);
    changed=b;changed.tick=a.tick;cut(a,changed);
    changed=b;changed.tick=a.tick-1;cut(a,changed);
    // A stable exit observer still has continuous channels after detachment.
    auto exit_a=a,exit_b=b;
    for(auto *s:{&exit_a,&exit_b}) {
        s->players[0].source_parent=null_datum;s->players[0].source_seat=-1;
        s->players[0].source_vehicle_camera->camera_mode=0;
    }
    near(multiplayer_presentation(exit_a,exit_b,.5f).players[0].source_vehicle_camera->position,
        {-2,0,2},"Authored post-exit observer stopped interpolating");
}
void high_refresh() {
    unsigned samples{};
    for(unsigned hz:{30u,60u,120u,144u,240u}) {
        const double tick_seconds=1.0/30;
        std::optional<Vec3> previous_camera;
        for(unsigned frame=0;frame<=hz*2;++frame) {
            const double presentation_tick=double(frame)*30/hz;
            const unsigned tick=unsigned(std::floor(presentation_tick));
            const float fraction=float(presentation_tick-tick);
            auto a=seated(100+tick),b=a;++b.tick;
            for(unsigned view=0;view<4;++view) {
                a.players[view].position.x=float(tick)*.5f;b.players[view].position.x=float(tick+1)*.5f;
                a.players[view].source_vehicle_camera->position.x+=float(tick)*.5f;
                b.players[view].source_vehicle_camera->position.x+=float(tick+1)*.5f;
                a.players[view].source_vehicle_camera->forward=direction(float(tick)*.03f);
                b.players[view].source_vehicle_camera->forward=direction(float(tick+1)*.03f);
            }
            const auto shown=multiplayer_presentation(a,b,fraction);
            for(unsigned view=0;view<4;++view) {
                const auto camera=shown.players[view].source_vehicle_camera->position;
                near(camera,{-3+float(presentation_tick)*.5f,float(view),2},
                    "High-refresh camera has a held frame or tick-boundary jump");
                near(shown.players[view].source_vehicle_camera->forward,direction(float(presentation_tick)*.03f),
                    "High-refresh camera orbit has a held frame or tick-boundary jump");
                if(!view && previous_camera)near(camera.x-previous_camera->x,float(.5/(hz*tick_seconds)),
                    "Vehicle camera velocity changes with the render/game clock phase");
                if(!view)previous_camera=camera;
                ++samples;
            }
        }
    }
    std::cout<<samples<<" high-refresh couch camera samples\n";
}
void death_handoff() {
    auto a=seated(10);
    auto &p=a.players[0];p.dead=true;p.source_previous_unit=p.source_unit;
    p.source_unit=null_datum;p.source_parent=null_datum;p.source_seat=-1;p.source_death_tick=9;
    p.source_death_camera=p.source_vehicle_camera;p.source_death_camera->camera_mode=3;
    p.source_vehicle_camera.reset();auto b=a;++b.tick;b.players[0].source_death_camera->position.x+=2;
    for(float fraction:{0.f,.25f,.5f,.75f,1.f}) {
        const auto shown=multiplayer_presentation(a,b,fraction);
        near(shown.players[0].source_death_camera->position,{-3+2*fraction,0,2},
            "Vehicle death handoff retained a stepped completed camera");
        check(!shown.players[0].source_vehicle_camera,"Death camera restored seated ownership");
    }
    for(unsigned boundary=0;boundary<5;++boundary) {
        auto changed=b;
        auto &player=changed.players[0];
        if(boundary==0)++player.source_death_tick;
        if(boundary==1)player.source_previous_unit+=0x10000;
        if(boundary==2)player.source_player+=0x10000;
        if(boundary==3)player.dead=false;
        if(boundary==4)++changed.tick;
        check(exact(*multiplayer_presentation(a,changed,0).players[0].source_death_camera,*player.source_death_camera),
            "Death observer blended across death, respawn, player or tick history");
    }
}
}
int main() {try {
    moving_seat();orbit_and_endpoints();lifecycle_cuts();high_refresh();death_handoff();
    for(float bad:{-1.f,2.f,std::numeric_limits<float>::quiet_NaN()}) {
        bool rejected=false;try {multiplayer_presentation(seated(10),seated(11),bad);}
        catch(const FormatError &) {rejected=true;}
        check(rejected,"Vehicle camera accepted an invalid render fraction");
    }
    std::cout<<"PASS vehicle camera timing, orbit/roll/lens, exact endpoints and lifecycle cuts\n";
}catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}}
