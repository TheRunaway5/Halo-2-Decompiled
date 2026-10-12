#include "halo2/bsp_surface_lighting.hpp"
#include "halo2/object_light_query.hpp"
#include "halo2/source_transparent_parts.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
using namespace halo2;
namespace {
void require(bool b,const std::string& s){if(!b)throw std::runtime_error(s);}
float f(std::uint32_t w){return std::bit_cast<float>(w);}
std::array<std::uint32_t,5> words(const ObjectLightmapSample& s) {
    return {std::bit_cast<std::uint32_t>(s.incident.x),std::bit_cast<std::uint32_t>(s.incident.y),
        std::bit_cast<std::uint32_t>(s.incident.z),s.diffuse_rgb,s.lightmap_rgb};
}
ObjectLightmapSample sample(const std::uint32_t* p){return {{f(p[0]),f(p[1]),f(p[2])},p[3],p[4]};}
struct Row {std::string label;char kind{};std::array<std::uint32_t,22> in{};std::array<std::uint32_t,20> out{};};
}
int main(int argc,char** argv){try {
    require(argc==3,"original fixture and Zanzibar cache required");
    if(!std::filesystem::exists(argv[2]))return 77;
    std::ifstream input(argv[1]);require(bool(input),"source fixture missing");
    std::map<std::uint32_t,Texture> residents;std::vector<Row> rows;std::string line;
    while(std::getline(input,line)) {
        if(line.empty()||line[0]=='#')continue;
        std::istringstream s(line);char kind;s>>kind;
        if(kind=='D') {
            std::uint32_t key,width,height,format,flags;std::string hex;
            s>>std::hex>>key>>width>>height>>format>>flags>>hex;
            require(bool(s)&&hex.size()%2==0,"invalid resident source texture row");
            std::vector<std::byte> bytes;bytes.reserve(hex.size()/2);
            for(std::size_t i=0;i<hex.size();i+=2)bytes.push_back(std::byte(std::stoul(hex.substr(i,2),nullptr,16)));
            // 136140 dispatches compressed blocks before the byte/word
            // bit3 swizzled-address branch. 1D2F0's bit0 is not that bit.
            const bool compressed=format>=14&&format<=16;
            require(residents.emplace(key,prepare_texture(Bytes(bytes),width,height,std::uint16_t(format),
                !compressed&&bool(flags&8))).second,"duplicate source resident texture");
        }else {
            Row row;row.kind=kind;s>>row.label>>std::hex;
            for(auto& w:row.in)s>>w;
            char split;s>>split;require(split=='|',"source row delimiter missing");
            for(auto& w:row.out)s>>w;
            require(bool(s),"truncated source row");rows.push_back(row);
        }
    }
    Cache cache(argv[2]);auto bank=cache.load_bsp_surface_lighting();
    const auto authored=cache.load_source_transparent_bsp_parts(0);
    std::size_t actual=0,controls=0,checked=0,uvs=0,resident_changes=0,base_mismatches=0;
    std::map<std::uint32_t,unsigned> resident_mip;
    for(const auto& row:rows) {
        const auto* p=row.in.data()+2;
        RenderSurfaceHit h{std::int32_t(p[0]),std::int32_t(p[1]),0,std::int32_t(p[8]),
            std::uint16_t(p[9]),std::uint16_t(p[10]),f(p[12]),f(p[13])};
        h.geometry=h.instance<0?h.cluster:bank.instances.at(h.instance).definition;
        auto& g=(h.instance<0?bank.clusters:bank.definitions).at(h.geometry);
        auto& part=g.parts.at(h.part);const auto saved_type=part.type;
        const auto raw_type=(h.instance<0?authored.clusters:authored.instance_definitions).at(h.geometry).parts.at(h.part).type;
        require(part.type==raw_type,row.label+": authored part type was not retained");
        if(row.kind=='T')part.type=std::uint16_t(row.in[21]);
        require(part.type==row.out[6],row.label+": source part metadata mismatch");++checked;
        if(row.kind=='Q') {
            ++actual;
            const auto uv=bank.interpolate(h);require(bool(uv),row.label+": missing native source geometry");
            require(std::bit_cast<std::uint32_t>(uv->diffuse_uv.x)==row.out[7]&&
                std::bit_cast<std::uint32_t>(uv->diffuse_uv.y)==row.out[8],row.label+": original FLOAT UV arithmetic differs");
            uvs+=2;
            for(unsigned i=0;i<3;++i) {
                const auto index=g.indices.at(std::size_t(h.lightmap_triangle)*3+i);
                require(index==row.out[17+i],row.label+": original incident triangle index differs");
                require(g.incident.at(index)==row.out[14+i],row.label+": source D31C0 selected packed word differs");
                checked+=2;
            }
        }else ++controls;
        const auto prior=sample(row.in.data()+16);
        const auto baseline=bank.sample(h,1,row.in[1]!=0,prior);
        if(baseline&&words(*baseline)!=std::array<std::uint32_t,5>{row.out[1],row.out[2],row.out[3],row.out[4],row.out[5]})++base_mismatches;
        std::optional<Texture> saved;
        if(row.out[10]!=0xffffffffu) {
            const auto& resident=residents.at(row.out[10]);
            auto& current=bank.diffuse.at(part.material);
            // Compare actual bytes with the independently decoded authored
            // chain. Some source software headers have separately resident
            // data, which remains an explicit service input, not an inferred
            // mip substitution or a changed native cache policy.
            if(!resident_mip.contains(row.out[10])) {
                bool found=false;
                for(unsigned level=0;level<current.mips.size();++level) {
                    const auto& mip=current.mips[level];
                    if(mip.width==resident.width&&mip.height==resident.height&&current.format==resident.format&&
                       mip.size==resident.pixels.size()&&std::equal(resident.pixels.begin(),resident.pixels.end(),current.pixels.begin()+mip.offset)) {
                        resident_mip[row.out[10]]=level;found=true;break;
                    }
                }
                if(!found)resident_mip[row.out[10]]=0xffffffffu;
            }
            if(resident_mip.at(row.out[10]))++resident_changes;
            saved=std::move(current);current=resident;
        }
        const auto got=bank.sample(h,1,row.in[1]!=0,prior);
        require(bool(got)&&row.out[0]==0,row.label+": source ready sample was rejected");
        const auto result=words(*got);
        for(unsigned i=0;i<5;++i){require(result[i]==row.out[i+1],row.label+": whole original D1E10 word differs at "+std::to_string(i)+" native="+std::to_string(result[i])+" original="+std::to_string(row.out[i+1]));++checked;}
        if(saved)bank.diffuse.at(part.material)=std::move(*saved);
        if(row.kind=='T'&&!row.in[1]) {
            bool rejected=false;try{(void)bank.sample(h,1,false,{});}catch(const FormatError&){rejected=true;}
            require(rejected,row.label+": missing retained target silently accepted");
        }
        part.type=saved_type;
    }
    require(actual==2438&&controls==96&&residents.size()==21,"incomplete whole-call corpus");
    // Native MAP orchestration uses the independently proved complete D2A50
    // helper. Empty explicit collision banks isolate guard/ray-mode control;
    // they do not stand in for a successful registration or a ready bitmap.
    const CollisionWorld empty_world({});BspSurfaceLighting empty;
    for(unsigned mode:{0u,1u,4u,5u}) {
        const auto r=query_object_lightmap(empty,empty_world,{},ObjectLightMapQueryContext{mode,false,1,{}});
        require(r.status==ObjectLightQueryStatus::miss&&r.rays==0&&!r.sample,"source bank guard did not precede ray validation");
    }
    empty.lighting.structure_matches=true;
    const auto no_bitmap=query_object_lightmap(empty,empty_world,{},ObjectLightMapQueryContext{1,false,1,{}});
    require(no_bitmap.status==ObjectLightQueryStatus::miss&&no_bitmap.rays==0&&!no_bitmap.sample,"missing bitmap bypassed source D2A50 guard");
    empty.lighting.bitmap_datum=0; //explicit valid guard; empty collision bank
    for(unsigned mode:{0u,1u,5u}) {
        const auto r=query_object_lightmap(empty,empty_world,{},ObjectLightMapQueryContext{mode,false,1,{}});
        require(r.status==ObjectLightQueryStatus::miss&&r.rays==(mode?5u:1u)&&!r.sample,"native source D2A50 mode precedence differs");
    }
    bool rejected=false;try{(void)query_object_lightmap(empty,empty_world,{},ObjectLightMapQueryContext{4,false,1,{}});}catch(const FormatError&){rejected=true;}
    require(rejected,"missing custom displacement silently accepted");
    const auto separate=std::count_if(resident_mip.begin(),resident_mip.end(),[](const auto& item){return item.second==0xffffffffu;});
    std::cout<<actual<<" whole original ready MAP samples + "<<controls<<" initial/retained part4/5 controls; "
        <<checked+uvs<<" metadata/UV/incident/sample words exact; "<<residents.size()<<" actual resident surfaces; "
        <<residents.size()-separate<<" match authored mips and "<<separate<<" require separately retained software data; "<<resident_changes<<" calls use a differing resident image ("
        <<base_mismatches<<" sample differences from full mip0).\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
