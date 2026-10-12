#include "halo2/source_weather_assets.hpp"
#include "halo2/vertex_program.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <set>

using namespace halo2;
namespace {
std::uint32_t word(float f){return std::bit_cast<std::uint32_t>(f);}
float scalar(std::uint64_t v){return std::bit_cast<float>(std::uint32_t(v));}
std::uint64_t hash(std::span<const std::byte> bytes){
    std::uint64_t out=14695981039346656037ull;
    for(const auto b:bytes)out=(out^std::to_integer<unsigned>(b))*1099511628211ull;
    return out;
}
std::vector<std::uint64_t> words(const std::string &line){
    std::istringstream stream(line);std::uint64_t v;std::vector<std::uint64_t> out;
    while(stream>>std::hex>>v)out.push_back(v);return out;
}
std::vector<std::uint64_t> probe(const std::vector<std::uint64_t> &in){
    std::size_t at=0;const auto take=[&](){if(at>=in.size())throw std::runtime_error("weather probe truncated");return in[at++];};
    const auto mode=take();std::vector<std::uint64_t> out;
    if(mode==0){
        SourceWeatherParticles p;p.type=take();p.opacity={scalar(take()),scalar(take())};
        p.streak_scale=scalar(take());p.line_width=scalar(take());const auto elapsed=scalar(take());
        const auto views=std::int16_t(take()),view=std::int16_t(take());const auto count=take();
        for(unsigned i=0;i<count;++i){
            p.render.push_back({{scalar(take()),scalar(take()),scalar(take())},scalar(take()),std::uint32_t(take())});
            p.state.push_back({{scalar(take()),scalar(take()),scalar(take())},
                {scalar(take()),scalar(take()),scalar(take())},scalar(take()),scalar(take())});
        }
        const auto result=source_weather_submission(p,elapsed,views,view);
        out={result.lines,word(result.line_width),result.vertices.size()};
        for(const auto &v:result.vertices){for(const auto f:v.position)out.push_back(word(f));out.push_back(v.color);}
    }else if(mode==1){
        const auto p=source_weather_pixel_program(take()!=0);for(const auto w:p.words)out.push_back(w);
    }else if(mode==2){
        const auto current=std::int32_t(take());const Vec3 eye{scalar(take()),scalar(take()),scalar(take())};
        const auto palettes=take(),clusters=take(),portals=take(),visible=take();SourceWeatherBsp b;BspAtmosphere a;
        for(unsigned i=0;i<palettes;++i){SourceWeatherBsp::Palette p;p.datum=take();b.palette.push_back(p);}
        for(unsigned i=0;i<clusters;++i){b.cluster_palette.push_back(std::int16_t(take()));
            BspAtmosphere::Cluster c;const auto count=take();for(unsigned j=0;j<count;++j)c.portals.push_back(take());a.clusters.push_back(c);}
        for(unsigned i=0;i<portals;++i){const auto back=std::int32_t(take()),front=std::int32_t(take());
            a.planes.push_back({scalar(take()),scalar(take()),scalar(take()),scalar(take())});
            a.portals.push_back({back,front,i,{},0,0});}
        std::vector<unsigned> vis;for(unsigned i=0;i<visible;++i)vis.push_back(take());
        for(const auto &s:source_weather_admission(b,a,current,vis,eye)){out.push_back(s.visible);out.push_back(word(s.portal_distance));}
    }else if(mode==3){
        const auto left=std::int16_t(take()),top=std::int16_t(take());
        const auto right=std::int16_t(take()),bottom=std::int16_t(take());
        const auto render_mode=std::int32_t(take());
        for(const auto &c:source_weather_viewport_constants(left,top,right,bottom,render_mode))
            for(const auto v:c)out.push_back(word(v));
    }else throw std::runtime_error("unknown weather probe");
    if(at!=in.size())throw std::runtime_error("weather probe trailing words");return out;
}
std::vector<std::uint64_t> assets(const std::filesystem::path &path){
    Cache cache(path);const auto b=cache.load_source_weather_bsp();
    std::vector<std::uint64_t> out{b.palette.size(),b.cluster_palette.size(),b.polyhedra.size()};
    std::set<std::uint32_t> definitions;
    for(const auto &p:b.palette){out.push_back(p.datum);out.push_back(hash(p.authored));if(p.datum!=0xffffffffu)definitions.insert(p.datum);}
    out.push_back(hash(std::as_bytes(std::span(b.cluster_palette))));
    for(const auto &p:b.polyhedra){out.insert(out.end(),{word(p.center.x),word(p.center.y),word(p.center.z),word(p.radius),p.planes.size()});
        for(const auto &plane:p.planes)for(const auto f:plane)out.push_back(word(f));}
    out.push_back(definitions.size());
    for(const auto datum:definitions){const auto d=cache.load_source_weather_definition(datum);
        out.insert(out.end(),{datum,d.particles.size(),d.plates.size(),hash(std::as_bytes(std::span(&d.wind,1))),word(d.fade_radius)});
        for(unsigned i=0;i<2;++i){out.push_back(d.vertex_datums[i]);out.push_back(hash(d.vertex_programs[i]));
            if(!decode_vertex_program(d.vertex_programs[i]).count)throw std::runtime_error("empty original weather vertex program");}
        for(const auto &p:d.particles){out.insert(out.end(),{hash(p.authored),p.sprites,p.type,p.maximum_particles,p.render.size(),p.state.size(),
                hash(std::as_bytes(std::span(p.render))),hash(std::as_bytes(std::span(p.state))),p.resources.size()});
            for(const auto &r:p.resources){out.push_back(hash(r.descriptor));out.push_back(hash(r.bytes));}}
        for(const auto &p:d.plates)out.push_back(hash(p.authored));
    }
    return out;
}
void print(const std::vector<std::uint64_t>&out){for(const auto w:out)std::cout<<std::hex<<w<<' ';std::cout<<'\n';}
}
int main(int argc,char **argv){try{
    if(argc==2 && std::string(argv[1])=="--probe"){
        for(std::string line;std::getline(std::cin,line);)if(!line.empty())print(probe(words(line)));return 0;}
    if(argc==3 && std::string(argv[1])=="--assets"){print(assets(argv[2]));return 0;}
    if(argc!=4)throw std::runtime_error("weather geometry/assets fixture and workspace root required");
    std::ifstream geometry(argv[1]),metadata(argv[2]);if(!geometry||!metadata)throw std::runtime_error("weather fixture missing");
    unsigned cases=0,maps=0;
    for(std::string line;std::getline(geometry,line);){if(line.empty()||line[0]=='#')continue;
        const auto split=line.find('|');if(split==std::string::npos)throw std::runtime_error("malformed weather geometry fixture");
        if(probe(words(line.substr(0,split)))!=words(line.substr(split+1)))throw std::runtime_error("original weather geometry/pixel/visibility differs at case "+std::to_string(cases));++cases;}
    for(std::string line;std::getline(metadata,line);){if(line.empty()||line[0]=='#')continue;
        const auto split=line.find('|');if(split==std::string::npos)throw std::runtime_error("malformed weather assets fixture");
        if(assets(std::filesystem::path(argv[3])/line.substr(0,split))!=words(line.substr(split+1)))
            throw std::runtime_error("original weather assets differ for "+line.substr(0,split));++maps;}
    if(!cases||maps!=21)throw std::runtime_error("weather fixtures do not cover all multiplayer maps");
    std::cout<<cases<<" original weather submission/pixel/visibility cases and "<<maps<<" map asset banks passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
