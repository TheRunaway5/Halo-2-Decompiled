#include "halo2/native_weapon_projectile_scene.hpp"
#include "halo2/content.hpp"
#include "halo2/collision.hpp"
#include "halo2/source_frustum_camera.hpp"
#include <algorithm>
#include <iostream>
using namespace halo2;
namespace {
void check(bool condition,const char *message) {if(!condition)throw FormatError(message);}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply prepared MCC Coagulation map");
    if(!*argv[1] || !std::filesystem::is_regular_file(argv[1]))return 77;
    Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path());
    check(cache.is_mcc(),"Projectile presentation requires prepared MCC runtime content");
    const auto definition=[&](std::string_view suffix) {
        const auto found=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto &tag) {
            return tag.group=="proj" && tag.name.ends_with(suffix);
        });
        check(found!=cache.tags().end(),"Actual projectile definition is absent");return found->datum;
    };
    const std::array definitions{definition("rocket_launcher\\projectiles\\rocket"),
        definition("ghost\\weapon\\ghost_bolt")};
    const auto content=cache.load_bsp_content(0);CollisionWorld collision(content.collision);Mesh mesh;
    NativeWeaponProjectileScene scene(cache,0,mesh,collision,definitions,2048);
    // Declared presentation inputs, not fabricated source births or contacts.
    const auto point=cache.spawns().front().position+Vec3{0,0,2};
    std::array<NativeWeaponProjectile,2> live{{
        {0x12340001u,definitions[0],point,{10,0,0},{1,0,0},{0,0,1},1},
        {0x12340002u,definitions[1],point+Vec3{0,1,0},{10,0,0},{1,0,0},{0,0,1},1}}};
    SourceObjectViewLod view;view.input=[](auto,auto,auto,auto){return SourceObjectLodInput{0,0x80000u,0};};
    SourceFrustumProjectionInput input;input.projection_angle=.8655884265899658f;
    input.outer=input.safe={0,0,480,640};
    const auto projection=source_frustum_projection(input);
    const auto camera=source_view_projection({point-Vec3{3,0,0},{1,0,0},{0,0,1},
        .015625f,1024.f,{},projection.projection_program});
    for(unsigned n=0;n<2;++n) {
        scene.synchronize(live);
        const auto &draws=scene.draw_view(n,view);
        const auto rocket=std::find_if(draws.begin(),draws.end(),[&](const auto &draw) {
            return draw.owner==live[0].datum && draw.definition==definitions[0];
        });
        check(rocket!=draws.end() && !rocket->vertices.empty(),"Fired rocket has no authored model draw");
        const auto &lights=scene.light_volume_view(0,{1,0,0},camera,{0,0,640,480},1024.f);
        check(std::any_of(lights.begin(),lights.end(),[&](const auto &draw) {
            return draw.object==live[1].datum && draw.volume && !draw.volume->volumes.empty();
        }),"Ghost bolt has no authored glow draw");
        live[0].position.x+=1;live[1].position.x+=1;
    }
    scene.synchronize({});
    check(scene.draw_view(3,view).empty(),"Retired rocket leaves a stale model draw");
    check(scene.light_volume_view(0,{1,0,0},camera,{0,0,640,480},1024.f).empty(),
        "Retired Ghost bolt leaves a stale glow draw");
    std::cout<<"PASS authored rocket model, Ghost bolt glow, movement and retirement presentation\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
