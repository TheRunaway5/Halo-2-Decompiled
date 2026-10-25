#include "halo2/source_objective_presentation_tu15.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace halo2;
namespace {
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
Vec3 vector(const nlohmann::json& row){return {row.at(0),row.at(1),row.at(2)};}
}
int main(int argc,char** argv)try {
    need(argc==2,"Supply original goal geometry reference");nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
    need(fixture.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        fixture.at("supplied_service_results").empty(),"Goal geometry reference owner differs");
    unsigned cases{};
    for(const auto& row:fixture.at("cases")) {
        SourceObjectiveGoalTu15 goal;goal.center=vector(row.at("center"));goal.color=vector(row.at("color"));
        goal.radius=row.at("radius");goal.distance_scale=1.5f;goal.designator=row.at("designator");
        const SourceKingBoundaryFloatingPoint fp{row.at("fp").at(0),row.at("fp").at(1)};
        const auto polygon=source_objective_goal_polygon_tu15(goal.center,goal.radius,fp);
        for(unsigned n=0;n<32;++n)for(unsigned axis=0;axis<2;++axis)
            need(std::bit_cast<std::uint32_t>(polygon.points[n][axis])==row.at("points").at(n*2+axis),"Original goal circle coordinate differs");
        need(std::bit_cast<std::uint32_t>(polygon.perimeter)==row.at("perimeter"),"Original goal perimeter differs");
        const auto eye=vector(row.at("eye"));
        auto wall=source_objective_goal_boundary_tu15(goal,eye,10117,30,fp);
        if(row.at("height").get<float>()==.8f) {
            SourceKingState king;king.marker_count=4;king.polygon=polygon.points;
            king.center={goal.center.x,goal.center.y,goal.center.z};king.radius=goal.radius;king.perimeter=polygon.perimeter;
            wall=source_king_boundary(king,{eye.x,eye.y,eye.z},{goal.color.x,goal.color.y,goal.color.z},10117,30,fp);
        }
        need(bool(wall),"Original visible goal lost its wall");
        need(std::bit_cast<std::uint32_t>(wall->outer)==row.at("outer") && std::bit_cast<std::uint32_t>(wall->inner)==row.at("inner"),"Original goal fade/width prefix differs");
        for(unsigned n=0;n<66;++n) {
            const auto& v=wall->strip[n];const auto& expected=row.at("vertices").at(n);
            std::array<float,10> actual{v.position[0],v.position[1],v.position[2],v.position[3],v.uv[0],v.uv[1],v.rgba[0],v.rgba[1],v.rgba[2],v.rgba[3]};
            for(unsigned i=0;i<actual.size();++i)if(std::bit_cast<std::uint32_t>(actual[i])!=expected.at(i)) {
                std::cerr<<"case "<<cases<<" vertex "<<n<<" component "<<i<<" actual "<<std::hex<<std::bit_cast<std::uint32_t>(actual[i])<<" expected "<<expected.at(i)<<'\n';
                throw std::runtime_error("Original goal wall position, UV or RGBA differs");
            }
        }
        ++cases;
    }
    need(cases==96,"Original goal CPU geometry cohort incomplete");
    std::cout<<"PASS 48 objective and 48 King-height original wall cases; 6336 complete native vertices\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
