#include "halo2/source_portal_frustum.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw FormatError(message);}
std::vector<std::uint32_t> words(const std::string &line) {
    std::istringstream stream(line);std::vector<std::uint32_t> out;std::uint32_t word;
    while(stream>>std::hex>>word)out.push_back(word);return out;
}
std::vector<std::uint32_t> probe(const std::vector<std::uint32_t> &input) {
    std::size_t at=0;auto take=[&](){require(at<input.size(),"truncated portal frustum probe");return input[at++];};
    auto scalar=[&](){return std::bit_cast<float>(take());};
    auto vec3=[&](){return Vec3{scalar(),scalar(),scalar()};};
    auto vector=[&](){return ShaderVector{scalar(),scalar(),scalar(),scalar()};};
    auto polygon=[&](){std::vector<Vec2> out;const auto n=take();for(unsigned i=0;i<n;++i)out.push_back({scalar(),scalar()});return out;};
    std::vector<std::uint32_t> result;auto floats=[&](const auto &values){for(float value:values)result.push_back(std::bit_cast<std::uint32_t>(value));};
    auto emit_polygon=[&](const auto &out){result.push_back(out.size());for(const auto p:out)floats(std::array{p.x,p.y});};
    const auto mode=take();
    if(mode==0){const std::array<float,3> plane{scalar(),scalar(),scalar()};const auto epsilon=scalar();const auto capacity=take();
        emit_polygon(source_clip_portal_polygon_2d(plane,polygon(),epsilon,capacity));}
    else if(mode==1||mode==3){const auto epsilon=scalar();const auto capacity=take();const auto subject=polygon(),clip=polygon();
        const auto out=source_intersect_portal_polygons(subject,clip,epsilon,capacity);
        if(mode==3)result.push_back(out.size()>=3);emit_polygon(out);}
    else if(mode==2) {
        SourcePortalProjectionView view;view.world_to_view.scale=scalar();for(auto &b:view.world_to_view.basis)b=vec3();view.world_to_view.position=vec3();
        view.eye=vec3();view.rectangle=vector();view.clipping_plane=vector();view.far_enabled=take()!=0;view.far_distance=scalar();view.initial_polygon=polygon();
        SourcePortalGeometry portal;portal.flags=take();portal.back=std::int16_t(take());portal.front=std::int16_t(take());const auto plane=vector();
        const auto n=take();for(unsigned i=0;i<n;++i)portal.vertices.push_back(vec3());
        const auto out=source_project_portal(view,portal,plane);
        result={out.flags,out.classification,std::uint32_t(std::uint16_t(out.back)),std::uint32_t(std::uint16_t(out.front))};
        floats(std::array{out.minimum_depth});floats(out.rectangle);result.push_back(out.clipped_count);result.push_back(out.arena_entries);
        emit_polygon(out.polygon);
    } else throw FormatError("unknown portal frustum probe");
    require(at==input.size(),"trailing portal frustum probe words");return result;
}
}
int main(int argc,char **argv){try {
    if(argc==2 && std::string_view(argv[1])=="--probe") {
        for(std::string line;std::getline(std::cin,line);)if(!line.empty()) {
            for(auto word:probe(words(line)))std::cout<<std::hex<<word<<' ';std::cout<<'\n';}return 0;
    }
    require(argc==2,"source portal frustum fixture required");std::ifstream file(argv[1]);require(bool(file),"source portal frustum fixture missing");
    std::array<unsigned,4> cases{};
    for(std::string line;std::getline(file,line);){if(line.empty()||line[0]=='#')continue;
        const auto split=line.find('|');require(split!=std::string::npos,"malformed source portal frustum fixture");
        const auto input=words(line.substr(0,split));require(!input.empty()&&input[0]<4,"invalid portal frustum fixture mode");
        if(probe(input)!=words(line.substr(split+1)))throw FormatError("original portal frustum differs at case "+std::to_string(cases[0]+cases[1]+cases[2]+cases[3]));
        ++cases[input[0]];
    }
    for(const auto n:cases)require(n>=64,"bounded portal frustum fixture coverage incomplete");
    bool rejected=false;try{source_intersect_portal_polygons(std::vector<Vec2>(65),std::vector<Vec2>(4));}catch(const FormatError&){rejected=true;}
    require(rejected,"portal frustum scratch capacity must remain explicit");
    std::cout<<cases[0]<<" plane clips, "<<cases[1]<<" convex intersections, "<<cases[2]<<" portal projections and "<<cases[3]<<" source173380 admission cases match original words\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
