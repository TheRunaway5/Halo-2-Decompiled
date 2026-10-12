#include "halo2/source_shadow_submission.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
namespace {
using namespace halo2;using Words=std::vector<std::uint32_t>;
void require(bool v,const char*m){if(!v)throw FormatError(m);}
Words evaluate(const Words &words) {
    unsigned at=0;const auto next=[&]{return words.at(at++);};
    const unsigned mode=next(),count=next(),queue_count=next();SourceShadowCandidateContext c;c.mode=mode;
    c.clock.tick=std::bit_cast<std::int32_t>(next());c.clock.seconds_per_tick=std::bit_cast<float>(next());
    for(auto &v:c.depth_plane)v=std::bit_cast<float>(next());c.pixel_scale=std::bit_cast<float>(next());
    std::vector<SourceShadowOwnerState> owners;std::map<unsigned,unsigned> priorities;
    for(unsigned i=0;i<count;++i){SourceShadowOwnerState o;o.identity=next();o.parent=next();o.sibling=next();o.child=next();o.object_flags=next();o.definition_mode=std::uint16_t(next());o.object_type=std::uint8_t(next());o.header_flags=std::uint8_t(next());o.header_kind=std::uint8_t(next());o.lod_flags_10a=std::uint8_t(next());o.death_tick=std::bit_cast<std::int32_t>(next());o.sphere={{std::bit_cast<float>(next()),std::bit_cast<float>(next()),std::bit_cast<float>(next())},std::bit_cast<float>(next())};const auto has_camo=next(),camo=next(),has_fade=next(),fade=next();if(has_camo)o.camouflage=std::bit_cast<float>(camo);if(has_fade)o.model_fade=std::uint8_t(fade);priorities[o.identity]=next();owners.push_back(o);}
    std::vector<SourceShadowCollectorEntry> collector;for(unsigned i=0;i<queue_count;++i)collector.push_back({next(),std::uint16_t(next())});
    std::array<std::uint8_t,32> scratch;for(auto&v:scratch)v=std::uint8_t(next());c.retained_fourth_flag_bytes=scratch;
    c.priority=[&](unsigned handle){return std::uint8_t(priorities.at(handle));};require(at==words.size(),"trailing shadow submission input");
    Words out;
    for(const auto&o:owners){const auto fade=source_shadow_owner_fade(owners,o.identity,c.clock);out.insert(out.end(),{unsigned(fade.eligible),std::bit_cast<std::uint32_t>(fade.fade),unsigned(fade.local_shadow),unsigned(source_shadow_owner_blocked(owners,o.identity))});}
    const auto hierarchy=source_shadow_owner_hierarchy(owners,owners.at(0).identity);out.push_back(unsigned(hierarchy.size()));
    for(const auto&s:hierarchy)for(const auto f:{s.center.x,s.center.y,s.center.z,s.radius})out.push_back(std::bit_cast<std::uint32_t>(f));
    const auto candidates=source_shadow_candidates(owners,collector,c);out.push_back(unsigned(candidates.size()));
    for(const auto&r:candidates)out.insert(out.end(),{r.flags,r.identity,std::bit_cast<std::uint32_t>(r.owner),std::bit_cast<std::uint32_t>(r.requested_fade),std::bit_cast<std::uint32_t>(r.coverage)});
    return out;
}
void controls(){
    SourceShadowOwnerState owner;owner.identity=0xe0000000;owner.object_type=owner.header_kind=6;owner.definition_mode=2;
    const std::array bank{owner};bool rejected=false;try{(void)source_shadow_owner_fade(bank,owner.identity,{});}catch(const FormatError&){rejected=true;}require(rejected,"unknown source fade byte guessed");
    owner.model_fade=255;owner.child=owner.identity;rejected=false;try{(void)source_shadow_owner_hierarchy(std::array{owner},owner.identity);}catch(const FormatError&){rejected=true;}require(rejected,"cyclic source shadow hierarchy admitted");
    owner.child=0xffffffffu;SourceShadowCandidateContext c;c.priority=[](unsigned){return 0;};rejected=false;try{(void)source_shadow_candidates(std::array{owner},std::array{SourceShadowCollectorEntry{owner.identity,0}},c);}catch(const FormatError&){rejected=true;}require(rejected,"unretained candidate flag3 replaced");
}
}
int main(int argc,char**argv){try{
    const bool probe=argc==2 && std::string_view(argv[1])=="--probe";std::ifstream file;if(!probe){require(argc==2,"shadow submission fixture required");file.open(argv[1]);require(bool(file),"shadow submission fixture missing");controls();}
    auto&stream=probe?std::cin:file;std::string line;unsigned cases=0,words=0;
    while(std::getline(stream,line)){if(line.empty() || line[0]=='#')continue;const auto sep=line.find('|');std::istringstream in(line.substr(0,sep));Words row;unsigned value;while(in>>std::hex>>value)row.push_back(value);const auto out=evaluate(row);if(probe){for(auto v:out)std::cout<<std::hex<<v<<' ';std::cout<<'\n';}else{require(sep!=std::string::npos,"shadow submission output missing");std::istringstream expected(line.substr(sep+1));Words golden;while(expected>>std::hex>>value)golden.push_back(value);if(out!=golden)throw FormatError("shadow submission differs at case "+std::to_string(cases));}++cases;words+=unsigned(out.size());}
    if(!probe){require(cases>200,"shadow submission fixture coverage missing");std::cout<<cases<<" original owner/hierarchy/collector cases, "<<words<<" words exact\n";}return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
