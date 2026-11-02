#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
using namespace halo2;using J=nlohmann::json;
namespace {
struct Impl {std::uint64_t scalar{};bool scalar16{},scalar17{};};
using Pass=std::vector<std::vector<Impl>>;
void require(bool ok,const std::string& why){if(!ok)throw FormatError(why);}
struct Reader {
    const Cache& cache;
    std::uint32_t word(std::uint32_t at)const{return Bytes(cache.read_tag_bytes(at,4)).u32(0);}
    std::uint16_t half(std::uint32_t at)const{return Bytes(cache.read_tag_bytes(at,2)).u16(0);}
    std::vector<std::uint32_t> block(std::uint32_t at,unsigned stride)const {
        const auto count=word(at),base=word(at+4u);
        require(count<=65536u&&std::uint64_t(base)+std::uint64_t(count)*stride<=0x100000000ull,"Native shader census block extent overflow");
        if(count)(void)cache.read_tag_bytes(base,std::size_t(count)*stride);
        std::vector<std::uint32_t> out;for(unsigned n=0;n<count;++n)out.push_back(base+n*stride);return out;
    }
};
Pass pass(const Reader& r,const Tag& tag) {
    Pass out;
    for(const auto pp:r.block(tag.address+28u,88)) {
        const auto externs=r.block(pp+56u,4);std::vector<Impl> rows;
        for(const auto at:r.block(pp,306)) {
            Impl row{r.word(at+0x10cu)|(std::uint64_t(r.word(at+0x110u))<<32u)};
            for(const auto field:{0x11eu,0x122u,0x124u}) {
                const unsigned packed=r.half(at+field),first=packed&511u,count=packed>>9;
                require(first<=externs.size()&&count<=externs.size()-first,"Native shader extern range escaped its block");
                for(unsigned i=first;i<first+count;++i) {
                    const auto source=Bytes(r.cache.read_tag_bytes(externs[i]+2u,2));
                    const auto component=std::to_integer<unsigned>(source.slice(0,1)[0]),id=std::to_integer<unsigned>(source.slice(1,1)[0]);
                    const bool scalar=field==0x122u?(component&7u)<4u:field==0x124u?((r.half(externs[i])>>5u)&31u)<4u:true;
                    row.scalar16|=scalar&&id==16;row.scalar17|=scalar&&id==17;
                }
            }
            row.scalar16|=(row.scalar&(1ull<<16u))!=0;row.scalar17|=(row.scalar&(1ull<<17u))!=0;rows.push_back(row);
        }
        out.push_back(std::move(rows));
    }
    return out;
}
}
int main(int argc,char** argv)try {
    require(argc==4,"Supply TU15_XBE PREPARED_MCC_MAPS REPORT");
    if(!std::filesystem::is_regular_file(argv[1])||!std::filesystem::is_directory(argv[2]))return 77;
    const std::filesystem::path maps=argv[2];std::vector<std::filesystem::path> files;
    for(const auto& entry:std::filesystem::directory_iterator(maps))if(entry.is_regular_file()&&entry.path().extension()==".map") {
        const auto name=entry.path().stem().string();
        if(name!="shared"&&name!="single_player_shared")files.push_back(entry.path());
    }
    std::sort(files.begin(),files.end());require(!files.empty(),"No actual MCC gameplay/menu maps to qualify");
    const auto original=OriginalData::load_xbe(argv[1]);J results=J::array();
    std::set<std::string> requested17,shadow_templates;std::uint64_t total_passes{},light_impls{},shadow_uses{},light_shader_uses{};
    for(const auto& file:files) {
        Cache cache(file,maps,original);require(cache.is_mcc(),"Loaded shader census requires the prepared MCC runtime content");Reader r{cache};
        std::map<std::uint32_t,Pass> passes;std::map<std::uint32_t,std::string> names;unsigned nonnative_passes{},templates{};
        for(const auto& tag:cache.tags())if(tag.group=="spas"&&tag.address&&tag.size) {
            if(!cache.is_preserved_native_shader(tag)){++nonnative_passes;continue;}
            const auto name=cache.shader_semantic_name(tag);auto info=pass(r,tag);
            for(const auto& pp:info)for(const auto& impl:pp) {
                require(!impl.scalar16,file.filename().string()+": scalar16 request in "+name);
                if(impl.scalar17) {
                    require(name=="shaders\\shader_passes\\shadow\\shadow_apply_combined",file.filename().string()+": scalar17 request outside original shadow pass: "+name);
                    requested17.insert(name);
                }
            }
            passes.emplace(tag.datum,std::move(info));names.emplace(tag.datum,name);
        }
        // 3A540 can bind a light's explicit +DC shad independently of the
        // receiver layer. Qualify every implementation of that template too.
        std::map<std::uint32_t,const Tag*> tags;
        for(const auto& tag:cache.tags())tags.emplace(tag.datum,&tag);
        std::set<std::uint32_t> light_templates;
        unsigned map_light_shaders{};
        for(const auto& tag:cache.tags())if(tag.group=="ligh"&&tag.address&&tag.size>=0xe4u&&
                r.word(tag.address+0xdcu)==0x73686164u) {
            const auto shader=r.word(tag.address+0xe0u);
            if(shader==0xffffffffu||shader==0u)continue;
            require(tags.contains(shader)&&tags.at(shader)->group=="shad","Light shader reference lost its actual shad");
            for(const auto definition:r.block(tags.at(shader)->address+32u,124))
                light_templates.insert(r.word(definition));
            ++map_light_shaders;
        }
        std::uint64_t map_lights{},map_shadows{},map_light_shader_uses{};
        for(const auto& tag:cache.tags())if(tag.group=="stem"&&cache.is_preserved_native_shader(tag)) {
            ++templates;
            for(const auto tp:r.block(tag.address+88u,40)) {
                const auto layers=r.block(tp+8u,2),selected=r.block(tp+16u,10);
                for(const auto lod:r.block(tp,10)) {
                    const unsigned packed=r.half(lod),first=packed&511u,count=packed>>9;
                    const auto available=r.word(lod+2u);require(first<=layers.size()&&count<=layers.size()-first,"Native template layer range escaped its block");
                    for(unsigned layer=0;layer<count;++layer)if(layer<32u&&(available&(1u<<layer))) {
                        const unsigned range=r.half(layers[first+layer]),pfirst=range&511u,pcount=range>>9;
                        require(pfirst<=selected.size()&&pcount<=selected.size()-pfirst,"Native layer pass range escaped its block");
                        for(unsigned p=pfirst;p<pfirst+pcount;++p) {
                            const auto datum=r.word(selected[p]+4u);const unsigned implementations=r.half(selected[p]+8u)>>9;
                            require(passes.contains(datum),"Installed native template references an unqualified native pass: "+cache.shader_semantic_name(tag));
                            for(const auto& pp:passes.at(datum)) {
                                require(implementations<=pp.size(),"Native template implementation count escaped its pass");
                                for(unsigned i=0;i<implementations;++i) {
                                    const auto& impl=pp[i];
                                    if(light_templates.contains(tag.datum)) {
                                        ++map_light_shader_uses;
                                        require(!impl.scalar16&&!impl.scalar17,"Explicit light shader requested F8 scalar in "+names.at(datum));
                                    }
                                    if(layer==12u||layer==13u){++map_lights;require(!impl.scalar16&&!impl.scalar17,"Light/gel layer requested F8 scalar in "+names.at(datum));}
                                    if(impl.scalar17){require(layer==11u,"Shadow scalar17 reachable outside layer11");++map_shadows;shadow_templates.insert(cache.shader_semantic_name(tag));}
                                }
                            }
                        }
                    }
                }
            }
        }
        require(!passes.empty()&&templates,"Loaded map has no installed native shader closure: "+file.filename().string());
        total_passes+=passes.size();light_impls+=map_lights;shadow_uses+=map_shadows;light_shader_uses+=map_light_shader_uses;
        for(const auto datum:light_templates)require(tags.contains(datum)&&tags.at(datum)->group=="stem"&&cache.is_preserved_native_shader(*tags.at(datum)),
            "Explicit light shader references an unqualified native template");
        results.push_back(J{{"map",file.filename().string()},{"native_pass_tags",passes.size()},{"native_templates",templates},
            {"unpreserved_pass_tags",nonnative_passes},{"light_and_gel_implementations_checked",map_lights},{"shadow17_uses",map_shadows},
            {"explicit_light_shaders",map_light_shaders},{"explicit_light_shader_implementations_checked",map_light_shader_uses}});
    }
    require(total_passes&&light_impls&&shadow_uses&&requested17.size()==1,"Native shader scalar census is vacuous");
    std::ofstream(argv[3])<<J{{"passed",true},{"maps",results},{"installed_native_pass_copies",total_passes},
        {"light_and_gel_implementations_checked",light_impls},{"shadow17_uses",shadow_uses},{"scalar17_passes",requested17},
        {"explicit_light_shader_implementations_checked",light_shader_uses},
        {"shadow_templates",shadow_templates},{"retained_large_artifacts",J::array()},
        {"scope","Actual Cache-installed native shaders over every prepared gameplay/menu map; source scalar masks and pixel/vertex/state rows. No scalar16 requester; scalar17 only shadow_apply_combined in layer11; layers12/13 never consume a light F8. Unpreserved PC passes are counted and excluded from the native shader claim."}}.dump(2)<<'\n';
    std::cout<<"PASS "<<files.size()<<" actual loaded MCC maps, "<<total_passes<<" native pass copies, "<<light_impls<<" light/gel implementations, scalar17 confined to shadow layer11\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
