#include "halo2/cache.hpp"
#include "halo2/collision.hpp"
#include "halo2/content.hpp"
#include "halo2/math.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;
using Json=nlohmann::json;
Json xyz(Vec3 p){return Json::array({p.x,p.y,p.z});}
int main(int argc,char **argv)try {
    if(argc!=6)throw FormatError("Supply MCC_MAP MCC_MAPS BSP DEFINITION REPORT");
    const Cache cache(argv[1],argv[2]);
    if(!cache.is_mcc())throw FormatError("Input survey requires the prepared MCC cache");
    const auto bsp=std::stoul(argv[3]);const auto content=cache.load_content(bsp,false);
    const CollisionWorld terrain(content.collision);
    Json report{{"scope","Read-only authored placement/BSP firing-lane input survey; no runtime creation, projectile, damage or physics result"},
        {"full_goal_complete",false},{"bsp",bsp},{"definition",argv[4]},{"placements",Json::array()}};
    for(const auto &object:content.objects) {
        const auto tag=cache.query_tag({.handle=TagHandle{object.definition}});
        if(object.type!=1 || object.model_attachment || !tag || tag->name!=argv[4])continue;
        const auto center=object.position+authored_basis(object.rotation).rotate(object.bounding_offset*object.scale);
        const auto basis=authored_basis(object.rotation);
        Json placement{{"placement",object.placement},{"authored_root",xyz(object.position)},
            {"authored_bound_center",xyz(center)},{"origin_bsp",object.origin_bsp},{"distances",Json::array()}};
        for(float distance:{12.f,8.f,6.f,4.f,3.f,2.f}) {
            unsigned top_supported{},top_clear{},lower_supported{},lower_clear{};
            Json first=Json::array(),clear=Json::array();
            for(unsigned side=0;side<32;++side) {
                const auto angle=float(side)*.19634954084936207f;
                auto origin=center-(basis.forward*std::cos(angle)+basis.left*std::sin(angle))*distance+Vec3{0,0,10};
                float reach=20;
                for(unsigned layer=0;layer<8 && reach>0;++layer) {
                    const auto floor=terrain.raycast(origin,{0,0,-1},reach);if(!floor)break;
                    const auto foot=floor->position+Vec3{0,0,content.player.radius};
                    const bool supported=floor->normal.z>=content.player.maximum_slope_cosine &&
                        !terrain.overlaps(foot,content.player.height,content.player.radius);
                    const auto eye=foot+Vec3{0,0,content.player.camera_height},delta=center-eye;
                    const auto obstruction=terrain.raycast(eye,normalized(delta),length(delta));
                    const bool visible=supported && !obstruction;
                    if(layer==0){top_supported+=supported;top_clear+=visible;}
                    else {lower_supported+=supported;lower_clear+=visible;}
                    Json sample{{"side",side},{"layer",layer},{"floor",xyz(floor->position)},
                        {"normal",xyz(floor->normal)},{"supported",supported},{"unobstructed",!obstruction}};
                    if(obstruction)sample["obstruction"]={{"position",xyz(obstruction->position)},{"surface",obstruction->source.surface}};
                    if(layer==0 && side<2)first.push_back(sample);
                    if(visible && clear.size()<4)clear.push_back(sample);
                    const auto step=floor->distance+content.player.radius*.1f;
                    reach-=step;origin=floor->position-Vec3{0,0,content.player.radius*.1f};
                }
            }
            placement["distances"].push_back({{"distance",distance},{"top_supported",top_supported},
                {"top_clear",top_clear},{"lower_supported",lower_supported},{"lower_clear",lower_clear},
                {"first_layer_examples",first},{"clear_examples",clear}});
        }
        report["placements"].push_back(std::move(placement));
    }
    if(report["placements"].empty())throw FormatError("No authored target placements in the surveyed content");
    std::ofstream output(argv[5]);output<<report.dump(2)<<'\n';
    if(!output)throw FormatError("Cannot write the bounded input survey");
    std::cout<<"PASS read-only BSP input survey; placements="<<report["placements"].size()<<'\n';
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
