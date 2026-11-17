// Read-only inventory of every resident `ligh` definition in the given
// caches, classified by what the pStencil light path can consume today.
// Usage: pstencil_audit_light_coverage OUTPUT.json MAP...
// A definition is "sphere" when the native effect-light owner accepts it
// (source_effect_light_definition); every other definition is listed with
// the exact reason that owner declines it. Authored colours are the literal
// upper/lower diffuse bounds; nothing is inferred from a tag name.
#include "halo2/cache.hpp"
#include "halo2/effect_light.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <fstream>
#include <iostream>
#include <map>

using namespace halo2;
namespace {
std::string escaped(const std::string& text) {
    std::string out;for(char c:text){if(c=='\\'||c=='"')out+='\\';out+=c;}return out;
}
}
int main(int argc,char** argv){try {
    if(argc<3){std::cerr<<"usage: OUTPUT.json MAP...\n";return 2;}
    std::ofstream output(argv[1]);if(!output)throw FormatError("cannot write light coverage report");
    std::map<std::string,unsigned> totals;
    output<<"{\"schema\":\"H2_PSTENCIL_LIGHT_COVERAGE_V1\",\"maps\":[";
    for(int index=2;index<argc;++index) {
        Cache cache(argv[index]);
        std::map<std::string,unsigned> classes;
        output<<(index>2?",":"")<<"{\"map\":\""<<escaped(cache.name())<<"\",\"lights\":[";
        bool first=true;
        for(const auto& tag:cache.tags()) {
            if(tag.group!="ligh")continue;
            const auto metadata=cache.load_source_tag_metadata(tag.datum);
            const auto raw=metadata.bytes_at(metadata.source_address(),228);
            const Bytes p(raw);
            std::string reason="sphere";
            try{(void)source_effect_light_definition(raw);}
            catch(const FormatError& e){reason=e.what();}
            const bool timed=p.f32(0xb0)!=0;
            const auto key=reason=="sphere"?(timed?std::string("sphere timed"):std::string("sphere")):reason;
            ++classes[key];++totals[key];
            output<<(first?"":",")<<"{\"name\":\""<<escaped(tag.name)<<"\",\"class\":\""<<escaped(key)
                <<"\",\"shape\":"<<p.u16(4)<<",\"flags\":"<<p.u32(0)<<",\"radius\":"<<p.f32(0x18)
                <<",\"specular_radius\":"<<p.f32(0x1c)<<",\"size\":["<<p.f32(8)<<','<<p.f32(0xc)
                <<"],\"lens\":"<<(p.u32(0x94)!=0xffffffffu)
                <<",\"duration\":"<<p.f32(0xb0)<<",\"hsv\":"<<(p.u32(0x34)!=0)
                <<",\"animated\":"<<bool(p.u32(0xc4)||p.u32(0xcc)||p.u32(0xd4))
                <<",\"animation\":{\"flags\":"<<p.u32(0xc0)<<",\"brightness\":"<<p.u32(0xc4)
                <<",\"color\":"<<p.u32(0xcc)<<",\"gel\":"<<p.u32(0xd4)<<"}"
                <<",\"diffuse_upper\":["<<p.f32(0x64)<<','<<p.f32(0x68)<<','<<p.f32(0x6c)
                <<"],\"diffuse_lower\":["<<p.f32(0x58)<<','<<p.f32(0x5c)<<','<<p.f32(0x60)
                <<"],\"brightness\":["<<p.f32(0x70)<<','<<p.f32(0x74)<<"]}";
            first=false;
        }
        output<<"],\"classes\":{";first=true;
        std::cout<<cache.name()<<':';
        for(const auto& [name,count]:classes) {
            output<<(first?"":",")<<'"'<<escaped(name)<<"\":"<<count;first=false;
            std::cout<<' '<<count<<" ["<<name<<']';
        }
        output<<"}}";std::cout<<'\n';
    }
    output<<"],\"totals\":{";bool first=true;
    for(const auto& [name,count]:totals){output<<(first?"":",")<<'"'<<escaped(name)<<"\":"<<count;first=false;}
    output<<"}}\n";
    std::cout<<"TOTAL:";for(const auto& [name,count]:totals)std::cout<<' '<<count<<" ["<<name<<']';std::cout<<'\n';
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
