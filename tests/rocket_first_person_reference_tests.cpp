#include "halo2/content.hpp"
#include "halo2/model.hpp"
#include "halo2/first_person_ik.hpp"
#include "halo2/transform.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
struct Sample {std::vector<Vertex> vertices;Vec3 offset;};
Sample sample(const std::filesystem::path &maps) {
    Cache cache(maps/"lockout.map");const auto content=cache.load_content(0,true);
    const auto selected=std::find_if(content.weapons.begin(),content.weapons.end(),
        [](const auto &row){return row.second.name.ends_with("\\rocket_launcher");});
    check(selected!=content.weapons.end(),"Rocket reference tag is absent");
    const auto &weapon=selected->second;
    const auto gun=cache.load_model(weapon.first_person_model,true);
    const auto hands=cache.load_model(content.first_person_hands,true);
    const auto idle=cache.load_action(weapon.first_person_animation,"any","any","any","idle");
    const auto ik=cache.load_first_person_ik(weapon.first_person_animation,
        weapon.first_person_model,content.first_person_hands);
    auto posed=pose_first_person_models_world(gun,hands,idle,0,weapon.first_person_offset,
        camera_transform({0,0,0},{1,0,0},{0,0,1}),ik,0);
    check(!posed.weapon.empty(),"Authored rocket idle has no geometry");
    return {std::move(posed.weapon),weapon.first_person_offset};
}
}
int main(int argc,char **argv)try {
    check(argc==4,"XBOX_MAPS MCC_MAPS REPORT required");
    const auto original=sample(argv[1]),prepared=sample(argv[2]);
    nlohmann::json report{{"passed",false},{"scope","Offline original Xbox and prepared MCC rocket mesh/idle pose"},
        {"original_vertices",original.vertices.size()},{"prepared_vertices",prepared.vertices.size()},
        {"rendered_framebuffer_parity",false}};
    const auto bounds=[](const auto &vertices) {
        std::array<float,6> out{INFINITY,INFINITY,INFINITY,-INFINITY,-INFINITY,-INFINITY};
        for(const auto &v:vertices){const std::array p{v.position.x,v.position.y,v.position.z};
            for(unsigned i=0;i<3;++i){out[i]=std::min(out[i],p[i]);out[i+3]=std::max(out[i+3],p[i]);}}
        return out;
    };
    report["original_bounds"]=bounds(original.vertices);report["prepared_bounds"]=bounds(prepared.vertices);
    float maximum=0;
    const bool same_count=original.vertices.size()==prepared.vertices.size();
    if(same_count)for(unsigned i=0;i<original.vertices.size();++i)
        maximum=std::max(maximum,length(original.vertices[i].position-prepared.vertices[i].position));
    report["maximum_corresponding_vertex_distance"]=maximum;
    report["first_person_offset_distance"]=length(original.offset-prepared.offset);
    report["passed"]=same_count && maximum<.0001f && length(original.offset-prepared.offset)<.000001f;
    std::ofstream output(argv[3]);output<<report.dump(2)<<'\n';
    check(report["passed"],"Prepared rocket mesh/idle pose differs from original Xbox reference");
    std::cout<<"PASS original Xbox rocket mesh/idle vertices: "<<original.vertices.size()
        <<", maximum distance="<<maximum<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
