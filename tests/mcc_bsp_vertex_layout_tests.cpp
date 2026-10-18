#include "halo2/mcc_bsp_vertex_layout.hpp"
#include "halo2/menu_world_source_geometry.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>
using namespace halo2;
using Json=nlohmann::json;
namespace {
unsigned checks{},negatives{};
void check(bool value,const char* message){++checks;if(!value)throw FormatError(message);}
void put(std::vector<std::byte>& b,unsigned p,unsigned word){for(unsigned i=0;i<4;++i)b.at(p+i)=std::byte(word>>(i*8));}
struct Input {
    std::vector<std::byte> descriptors=std::vector<std::byte>(128),positions=std::vector<std::byte>(12),uv=std::vector<std::byte>(8),basis=std::vector<std::byte>(36);
    unsigned count=1;
    Input(){descriptors[0]=std::byte{1};descriptors[1]=std::byte{12};descriptors[32]=std::byte{23};descriptors[33]=std::byte{8};descriptors[64]=std::byte{25};descriptors[65]=std::byte{36};descriptors[96]=std::byte{29};descriptors[97]=std::byte{8};put(basis,0,0x3f800000);put(basis,16,0x3f800000);put(basis,32,0x3f800000);}
    MccBspVertexLayout run()const{return project_mcc_bsp_vertex_layout(count,descriptors,positions,uv,basis);}
};
void portable(){
    Input f;const auto old=f;const auto result=f.run();check(result.basis.size()==12 && Bytes(result.basis).u32(0)==1023 && Bytes(result.basis).u32(4)==(1023u<<11) && Bytes(result.basis).u32(8)==(511u<<22),"Physical N/T/B slots changed");
    check(result.descriptors[32]==std::byte{24} && result.descriptors[64]==std::byte{27} && result.descriptors[65]==std::byte{12},"Original declaration projection differs");
    check(f.descriptors==old.descriptors && f.positions==old.positions && f.uv==old.uv && f.basis==old.basis,"Pure projection mutated source");
    const auto again=f.run();check(again.basis==result.basis && again.descriptors==result.descriptors,"Repeated source projection is unstable");
    auto reject=[&](auto mutate){Input n;mutate(n);bool threw=false;try{(void)n.run();}catch(const FormatError&){threw=true;}check(threw,"Unknown/malformed MCC vertex bank admitted");++negatives;};
    for(unsigned d=0;d<3;++d){reject([&](Input& n){n.descriptors[d*32]^=std::byte{1};});reject([&](Input& n){n.descriptors[d*32+1]^=std::byte{1};});for(unsigned p=2;p<32;++p)reject([&](Input& n){n.descriptors[d*32+p]=std::byte{1};});}
    reject([](Input& n){n.count=0;});reject([](Input& n){n.count=65536;});
    for(unsigned i=0;i<3;++i){reject([&](Input& n){(i==0?n.positions:i==1?n.uv:n.basis).pop_back();});reject([&](Input& n){(i==0?n.positions:i==1?n.uv:n.basis).push_back(std::byte{});});}
    for(unsigned size:{0,95,97,513,544})reject([&](Input& n){n.descriptors.resize(size);});
    for(unsigned bits:{0x7fc00000u,0x7f800000u,0xff800000u})for(unsigned i=0;i<3;++i)reject([&](Input& n){put(i==0?n.positions:i==1?n.uv:n.basis,0,bits);});
    reject([&](Input& n){std::copy(result.descriptors.begin(),result.descriptors.end(),n.descriptors.begin());n.basis=result.basis;});
}
struct RawSection {
    unsigned bsp{},kind{},section{},vertices{};
    std::vector<MenuWorldSourceResource> resources;
};
std::vector<RawSection> raw_sections(const Cache& cache){
    std::vector<RawSection> out;
    for(unsigned bsp=0;bsp<cache.bsps().size();++bsp){
        const auto owned=cache.load_source_bsp_metadata(bsp);const Bytes root(owned.bytes_at(owned.source_address()+16,0x160));
        const auto allocations=cache.load_bsp_resources(bsp);unsigned ordinal{};
        for(unsigned k=0;k<2;++k)for(unsigned n=0;n<root.u32(k?0x138:0x9c);++n,++ordinal){
            const Bytes section(owned.bytes_at(root.u32(k?0x13c:0xa0)+n*(k?200:176),k?200:176));
            if(!section.u16(0))continue;
            RawSection row{bsp,k,n,section.u16(0),{}};const Bytes raw(allocations.at(ordinal));const auto base=section.u32(48)+8;
            for(unsigned r=0;r<section.u32(56);++r){const Bytes d(owned.bytes_at(section.u32(60)+r*16,16));const auto at=std::uint64_t(base)+d.u32(12);if(at>0xffffffffu)throw FormatError("Actual resource offset overflow");const auto bytes=raw.slice(at,d.u32(8));row.resources.push_back({d.u16(4),d.u16(6),unsigned(at),{bytes.begin(),bytes.end()}});}
            out.push_back(std::move(row));
        }
    }
    return out;
}
const MenuWorldSourceResource& channel(const RawSection& section,unsigned ch){
    const MenuWorldSourceResource* found=nullptr;
    for(const auto& r:section.resources)if(r.type==56 && r.channel==ch){check(!found,"Ambiguous actual typed stream");found=&r;}
    check(found,"Missing actual typed stream");return *found;
}
Json actual(const char* mcc,const char* original){
    Cache cache(mcc);check(cache.is_mcc(),"Actual fixture must be MCC");const auto rows=raw_sections(cache);
    std::optional<Cache> reference;std::vector<RawSection> original_rows;
    if(original){reference.emplace(original);check(!reference->is_mcc() && !reference->is_vista(),"Reference fixture must be original Xbox");original_rows=raw_sections(*reference);check(original_rows.size()==rows.size(),"Paired section count differs");}
    unsigned vertices{},words{},paired{};
    for(unsigned i=0;i<rows.size();++i){const auto& row=rows[i];const auto& d=channel(row,32).bytes;const auto& p=channel(row,0).bytes;const auto& u=channel(row,1).bytes;const auto& b=channel(row,2).bytes;
        const auto projected=project_mcc_bsp_vertex_layout(row.vertices,d,p,u,b);vertices+=row.vertices;words+=row.vertices*3;
        if(reference){const auto& r=original_rows[i];check(row.bsp==r.bsp && row.kind==r.kind && row.section==r.section && row.vertices==r.vertices,"Paired section ownership/count differs");check(p==channel(r,0).bytes && u==channel(r,1).bytes,"Paired position/UV source bytes differ");check(projected.basis==channel(r,2).bytes,"Projected basis words differ from actual original");const auto& original_descriptors=channel(r,32).bytes;check(original_descriptors.size()>=96 && std::equal(projected.descriptors.begin(),projected.descriptors.end(),original_descriptors.begin()),"Projected declarations differ from actual original");++paired;}
    }
    check(!rows.empty(),"No nonempty actual BSP sections");
    // Exercise the real failing adapter seam, not only the pure converter.
    const auto& first=rows.front();const auto adapter=load_menu_world_source_bsp_section(cache,first.bsp,first.kind?MenuWorldSourceSectionKind::bsp_instance_definition:MenuWorldSourceSectionKind::bsp_cluster,first.section);
    check(adapter.resources().size()==first.resources.size(),"Projection changed resource ownership/count");
    const auto projected=project_mcc_bsp_vertex_layout(first.vertices,channel(first,32).bytes,channel(first,0).bytes,channel(first,1).bytes,channel(first,2).bytes);
    for(unsigned i=0;i<first.resources.size();++i){const auto& a=adapter.resources()[i];const auto& s=first.resources[i];check(a.type==s.type && a.channel==s.channel && a.raw_offset==s.raw_offset,"Draw-input projection changed source provenance");auto expected=s.bytes;if(s.type==56 && s.channel==2)expected=projected.basis;if(s.type==56 && s.channel==32)std::copy(projected.descriptors.begin(),projected.descriptors.end(),expected.begin());check(a.bytes==expected,"Adapter changed unrelated resource/lighting/header bytes");}
    const auto after=raw_sections(cache);check(after.size()==rows.size(),"Source resource bank count changed");for(unsigned i=0;i<rows.size();++i)for(unsigned r=0;r<rows[i].resources.size();++r)check(after[i].resources[r].bytes==rows[i].resources[r].bytes,"Source resource bytes mutated");
    if(reference){const auto original_adapter=load_menu_world_source_bsp_section(*reference,first.bsp,first.kind?MenuWorldSourceSectionKind::bsp_instance_definition:MenuWorldSourceSectionKind::bsp_cluster,first.section);check(adapter.streams().size()==original_adapter.streams().size(),"Projected/Original stream count differs");}
    return {{"map",cache.name()},{"sections",rows.size()},{"vertices",vertices},{"packed_basis_words",words},{"paired_original_sections",paired},{"actual_adapter_passed",true},{"source_resources_unchanged",true},{"models_or_lighting_or_shaders_changed",false}};
}
}
int main(int argc,char**argv)try{portable();Json result;if(argc>1){if(std::string(argv[1]).empty() || !std::filesystem::is_regular_file(argv[1]))return 77;if(argc>2 && (std::string(argv[2]).empty() || !std::filesystem::is_regular_file(argv[2])))return 77;result=actual(argv[1],argc>2?argv[2]:nullptr);}std::cout<<Json{{"checks",checks},{"negative_cases",negatives},{"actual",result}}.dump()<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
