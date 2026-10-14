#include "halo2/source_draw_unit_cull.hpp"
#include "halo2/source_bsp_ranges.hpp"
#include <bit>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace halo2;
namespace {
using Words=std::vector<std::uint32_t>;
struct Fixture {SourceDrawUnitCullContext context;std::vector<SourceCullDrawUnit> units;std::vector<SourceCullDrawBanks> banks;std::vector<SourceCullPartMask> masks;std::vector<std::size_t> mask_lengths;std::vector<std::shared_ptr<SourceBspRangeGeometry>> accelerators;bool shared_arena{};std::uint32_t arena_address{};SourceBspRangeArena arena;};
Fixture parse(const Words& v) {
    std::size_t at=0;
    const auto take=[&](){if(at==v.size())throw FormatError("draw-unit fixture truncated");return v[at++];};
    const auto real=[&](){return std::bit_cast<float>(take());};
    const auto point=[&](){return Vec3{real(),real(),real()};};
    const auto transform=[&](){Transform t;t.scale=real();for(auto& p:t.basis)p=point();t.position=point();return t;};
    const auto version=take();if(version<1||version>3)throw FormatError("draw-unit fixture version");
    Fixture f;auto& c=f.context;c.main_prefix=take();c.draw_count=take();c.mode=take();c.query_mode=std::int32_t(take());c.center=point();c.radius=real();
    const auto n=take();if(n>850)throw FormatError("draw-unit fixture exceeds actual unit bank");
    f.units.resize(n);f.banks.resize(n);f.mask_lengths.resize(n);
    for(unsigned i=0;i<n;++i) {
        for(auto& w:f.units[i].words)w=take();auto& b=f.banks[i];
        b.geometry_resolved=true;
        if(take())b.transform=transform();
        if(!take())continue;
        b.geometry.emplace();auto& g=*b.geometry;g.accelerator_count=std::int32_t(take());g.subpart_count=std::int32_t(take());
        const auto np=take(),nb=take(),nm=take(),owner=take();
        if(np>2048||nb>2048||nm>64)throw FormatError("draw-unit fixture exceeds part bank");
        for(unsigned j=0;j<np;++j)g.subpart_bounds.push_back(std::uint16_t(take()));
        for(unsigned j=0;j<nb;++j){SourceCullPartBound p;p.center=point();p.radius=real();p.node=std::uint8_t(take());g.bounds.push_back(p);}
        f.mask_lengths[i]=nm;SourceCullPartMask mask{f.units[i].words[5],{}};for(unsigned j=0;j<nm;++j)mask.words.push_back(take());
        const auto existing=std::find_if(f.masks.begin(),f.masks.end(),[&](const auto& p){return p.source_address==mask.source_address;});if(existing==f.masks.end())f.masks.push_back(mask);else if(version==3){if(!std::equal(existing->words.begin(),existing->words.begin()+std::min(existing->words.size(),mask.words.size()),mask.words.begin()))throw FormatError("aliased draw-unit fixture mask bytes differ");if(existing->words.size()<mask.words.size())existing->words=mask.words;}else if(existing->words!=mask.words)throw FormatError("aliased draw-unit fixture mask bytes differ");
        if(owner){g.owner_pose.emplace();auto& p=*g.owner_pose;p.owner=take();const auto nn=take(),ni=take();if(nn>256||ni>256)throw FormatError("draw-unit fixture node bank");for(unsigned j=0;j<nn;++j)p.nodes.push_back(transform());for(unsigned j=0;j<ni;++j)p.inverse_bind.push_back(transform());}
        if(version>=2) {
            const auto size=take(),mapping=take(),parts=take();
            if(size>1048576||mapping>65536||parts>2048)throw FormatError("draw-unit accelerator fixture bank");
            if(size) {
                auto geometry=std::make_shared<SourceBspRangeGeometry>();
                geometry->subpart_count=g.subpart_count;geometry->accelerator_count=g.accelerator_count;
                geometry->part_count=parts;geometry->parts.resize(parts);geometry->subparts.resize(std::size_t(g.subpart_count));
                for(unsigned j=0;j<size;j+=4){const auto word=take();for(unsigned k=0;k<4&&j+k<size;++k)geometry->accelerator.push_back(std::byte(word>>(k*8)));}
                for(unsigned j=0;j<mapping;++j)geometry->mapping.push_back(std::uint16_t(take()));
                g.accelerator=geometry.get();f.accelerators.push_back(std::move(geometry));
            } else if(mapping||parts)throw FormatError("partial draw-unit accelerator fixture");
        }
    }
    if(version==3) {
        f.shared_arena=true;f.arena_address=take();const auto used=take();
        if(used>512)throw FormatError("draw-unit fixture arena used extent");
        f.arena.used=std::uint16_t(used);for(auto& w:f.arena.words)w=take();
        for(const auto& mask:f.masks) {
            if(mask.source_address<f.arena_address || ((mask.source_address-f.arena_address)&3))throw FormatError("draw-unit fixture arena pointer");
            const auto offset=(mask.source_address-f.arena_address)/4;
            if(offset>512 || mask.words.size()>512-offset)throw FormatError("draw-unit fixture arena mask extent");
            if(!std::equal(mask.words.begin(),mask.words.end(),f.arena.words.begin()+offset))throw FormatError("draw-unit fixture shared initial bytes differ");
        }
    }
    if(at!=v.size())throw FormatError("draw-unit fixture trailing words");return f;
}
Words solve(Fixture f,bool lazy_pose=false) {
    if(lazy_pose)for(auto& bank:f.banks)if(bank.geometry && bank.geometry->owner_pose) {
        auto& geometry=*bank.geometry;
        geometry.resolve_owner_pose=[pose=*geometry.owner_pose]{return pose;};
        geometry.owner_pose.reset();
    }
    const auto calls=f.shared_arena?
        source_draw_unit_sphere_cull(f.units,f.banks,f.arena,f.arena_address,f.context):
        source_draw_unit_sphere_cull(f.units,f.banks,f.masks,f.context);Words out;
    for(unsigned i=0;i<f.units.size();++i){out.insert(out.end(),f.units[i].words.begin(),f.units[i].words.end());if(f.banks[i].geometry){const auto m=std::find_if(f.masks.begin(),f.masks.end(),[&](const auto& p){return p.source_address==f.units[i].words[5];});if(m==f.masks.end())throw FormatError("fixture output mask missing");if(f.shared_arena){const auto offset=(m->source_address-f.arena_address)/4;out.insert(out.end(),f.arena.words.begin()+offset,f.arena.words.begin()+offset+f.mask_lengths[i]);}else out.insert(out.end(),m->words.begin(),m->words.end());}}
    out.push_back(std::uint32_t(calls.size()));
    for(const auto& c:calls){out.push_back(c.unit);for(auto x:{c.center.x,c.center.y,c.center.z,c.radius})out.push_back(std::bit_cast<std::uint32_t>(x));}
    if(f.shared_arena){out.push_back(f.arena.used);out.insert(out.end(),f.arena.words.begin(),f.arena.words.end());}
    return out;
}
Words words(const std::string& s){std::istringstream in(s);Words out;std::uint32_t v;while(in>>std::hex>>v)out.push_back(v);return out;}
void reject(const Fixture& f){bool rejected=false;try{(void)solve(f);}catch(const FormatError&){rejected=true;}if(!rejected)throw std::runtime_error("unresolved draw-unit input was accepted");}
}
int main(int argc,char** argv){try{
    if(argc==2 && std::string(argv[1])=="--probe"){std::string line;while(std::getline(std::cin,line)){const auto out=solve(parse(words(line)));for(auto w:out)std::cout<<std::hex<<w<<' ';std::cout<<'\n';}return 0;}
    std::ifstream in(argc==2?argv[1]:std::string(HALO2_SOURCE_DIR)+"/tests/fixtures/retail_source_draw_unit_cull.tsv");if(!in)throw std::runtime_error("draw-unit original fixture missing");
    unsigned cases=0;std::size_t checked=0;std::string line;Fixture eligible;
    while(std::getline(in,line)){if(line.empty()||line[0]=='#')continue;const auto split=line.find('|');if(split==std::string::npos)throw std::runtime_error("draw-unit fixture separator");const auto f=parse(words(line.substr(0,split)));const auto expected=words(line.substr(split+1)),out=solve(f);if(out!=expected)throw std::runtime_error("source draw-unit words differ at case"+std::to_string(cases));if(solve(f,true)!=expected)throw std::runtime_error("lazy source pose differs at case"+std::to_string(cases));checked+=out.size();++cases;if(f.units.size()==1 && f.context.query_mode==1 && (f.units[0].words[0]&0x41)==0x40 && f.banks[0].geometry && f.banks[0].geometry->subpart_count>0 && f.banks[0].geometry->accelerator_count<=0 && f.units[0].words[1]==0xffffffffu)eligible=f;}
    if(cases<1000||checked<20000||eligible.units.empty())throw std::runtime_error("draw-unit fixture lacks meaningful coverage");
    if(eligible.shared_arena) {
        auto invalid=eligible;invalid.units[0].words[5]=invalid.arena_address-4;reject(invalid);
        invalid=eligible;invalid.units[0].words[5]=invalid.arena_address+1;reject(invalid);
        invalid=eligible;invalid.units[0].words[5]=invalid.arena_address+512*4;reject(invalid);
        eligible.shared_arena=false;
    }
    auto bad=eligible;bad.context.query_mode=0;reject(bad);
    bad=eligible;bad.banks[0].geometry->accelerator_count=1;reject(bad);
    bad=eligible;bad.banks.clear();reject(bad);
    bad=eligible;bad.banks[0].geometry_resolved=false;reject(bad);
    bad=eligible;bad.units[0].words[7]=1;bad.banks[0].transform.reset();reject(bad);
    bad=eligible;bad.masks[0].words.clear();reject(bad);
    bad=eligible;bad.banks[0].geometry->subpart_bounds[0]=42;bad.masks[0].words[0]=0;bad.banks[0].geometry->bounds.clear();reject(bad);
    bad=eligible;bad.units[0].words[1]=0xe000003f;bad.banks[0].geometry->subpart_bounds[0]=0;bad.banks[0].geometry->bounds={{{},1.f,0}};bad.masks[0].words[0]=0;bad.banks[0].geometry->owner_pose.reset();reject(bad);
    bad=eligible;bad.masks.push_back(bad.masks[0]);reject(bad);
    bad=eligible;bad.units[0].words[1]=0xe000003f;bad.banks[0].geometry->subpart_bounds[0]=0;bad.banks[0].geometry->bounds={{{},1.f,0}};bad.masks[0].words[0]=0;bad.banks[0].geometry->owner_pose=SourceCullOwnerPose{0xe0000040,{Transform{}},{}};reject(bad);
    // A live pose provider may have no backing in branches that never read
    // B8C00. The original masked/sentinel paths must leave it untouched.
    auto lazy=eligible;lazy.units[0].words[1]=0xe000003f;
    auto& geometry=*lazy.banks[0].geometry;
    geometry.owner_pose.reset();
    geometry.resolve_owner_pose=[]()->SourceCullOwnerPose {
        throw FormatError("unconsumed live pose lookup");
    };
    std::fill(geometry.subpart_bounds.begin(),geometry.subpart_bounds.end(),0xffffu);
    (void)solve(lazy);
    geometry.subpart_bounds[0]=0;geometry.bounds={{{},1.f,0}};
    for(auto& mask:lazy.masks)std::fill(mask.words.begin(),mask.words.end(),0xffffffffu);
    (void)solve(lazy);
    lazy.masks[0].words[0]&=~1u;reject(lazy);
    // The same unavailable services are unread for an already admitted unit.
    bad.units[0].words[0]|=1;bad.context.query_mode=0;bad.banks.clear();(void)source_draw_unit_sphere_cull(bad.units,bad.banks,bad.masks,bad.context);
    std::cout<<cases<<" unchanged44720/45560 cases, "<<checked<<" exact words through supplied and lazy pose banks;10 unresolved-service controls and unread admitted-unit control passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
