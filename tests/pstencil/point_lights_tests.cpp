#include "halo2/pstencil/point_lights.hpp"
#include "halo2/transform.hpp"
#include <algorithm>
#include <iostream>
using namespace halo2;
void require(bool ok,const char* why){if(!ok)throw FormatError(why);}
int main(int argc,char** argv){try {
    require(argc==2,"Supply current Xbox Lockout cache");Cache cache(argv[1]);
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto& t) {
        return t.group=="proj" && t.name=="objects\\weapons\\grenade\\plasma_grenade\\plasma_grenade";
    });require(tag!=cache.tags().end(),"authored plasma projectile missing");
    pstencil::PointLights lights(cache,0);require(lights.has(tag->datum),"plasma direct light not admitted");
    auto root=source_object_transform(cache.spawns().at(0).position,{1,0,0},{0,0,1},1,false);
    pstencil::PointLightOwner owner{0xe2710007u,tag->datum,root,{},{}};
    lights.synchronize(std::span(&owner,1),10);const auto born=lights.lights();
    require(born.size()==1,"plasma did not produce exactly its authored glow");
    const auto authored=load_effect_light(cache,born[0].definition);
    const auto expected=source_effect_light_state(authored,1,0,30,cache.original_data()->fade_curve);
    require(born[0].state.diffuse.x==expected.diffuse.x && born[0].state.diffuse.y==expected.diffuse.y &&
        born[0].state.diffuse.z==expected.diffuse.z,"plasma color differs from authored endpoints");
    require(expected.diffuse.z>expected.diffuse.x*3 && expected.diffuse.y>expected.diffuse.x*3,
        "plasma light lost its authored blue/cyan color");
    require(born[0].radius==.8f,"plasma light radius differs from authored value");
    for(unsigned tick=11;tick<80;++tick) {
        owner.root.position.x+=.002f;owner.root.position.y-=.001f;
        lights.synchronize(std::span(&owner,1),tick);
        const auto moving=lights.lights();require(moving.size()==1 && moving[0].identity==born[0].identity,
            "moving plasma glow lost its stable generation");
        require(moving[0].position.x==owner.root.position.x && moving[0].position.y==owner.root.position.y,
            "plasma glow did not follow its current source pose");
        require(moving[0].registration.reference_head!=0xffffffffu,"moving point light lost spatial registration");
    }
    lights.synchronize({},80);require(lights.lights().empty(),"retired plasma grenade left a phantom light");
    owner.object=0xe2720007u;lights.synchronize(std::span(&owner,1),81);
    require(lights.lights().at(0).identity!=born[0].identity,"reused object slot retained stale light generation");
    std::cout<<"PASS: authored plasma blue/cyan RGB, radius, 69 moving poses, stable identity, registration, retirement and slot reuse\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
