#include "halo2/source_decorator_decals_tu15.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>

namespace {
using namespace halo2;
void require(bool value,const char* message) {if(!value)throw FormatError(message);}
const SourceDecoratorDecalGroupTu15& first(const SourceDecoratorDecalBspTu15& b) {
    require(b.definitions.size()==1,"Beaver Creek decorator definition count changed");
    const auto& d=b.definitions.front();require(d.groups.size()==87,"Beaver Creek authored group count changed");
    require(!d.groups.empty()&&d.groups[0].selection.has_value(),"actual first decorator route absent");
    return d.groups[0];
}
void cache(const std::filesystem::path& file,bool mcc) {
    Cache c(file);require(c.is_mcc()==mcc,"decorator fixture cache format changed");
    const auto metadata=c.load_source_bsp_metadata(0);const auto address=metadata.source_address();
    const Bytes root(metadata.bytes_at(address,metadata.size()));
    const auto containers=TagAddressSpace(root,address).block(16+0x234,48,1);
    require(containers.count==1,"real Beaver Creek decorator metadata absent before adapter");
    const auto at=containers.offset;
    require(root.u32(at+0x18)==87&&root.u32(at+0x10)>1,"real decorator group/cache resource owner absent");
    const auto header=TagAddressSpace(root,address).offset(root.u32(at+0x14),mcc?104:88);
    require(root.u32(header+4)>0&&root.u32(header+(mcc?52:44)+4)>0,"actual second cache header requires platform stride");
    const auto resource=c.load_source_runtime_resource(root.u32(header),root.u32(header+4));
    require(resource.size()==(mcc?456u:408u),"real first decorator resource bytes changed");
    const Bytes bytes(resource);require(bytes.u32(8)==1&&bytes.u32(16)==4&&bytes.u32(24)==6,
        "real floating geometry preconditions absent before native adapter");
    std::cout<<(mcc?"MCC":"Xbox")<<" actual asset:87 groups, first cache1 placement/4 vertices/6 indices, header stride"<<(mcc?52:44)<<'\n';
    const auto out=c.load_source_decorator_decals_tu15(0);const auto& g=first(out);const auto& d=out.definitions[0];
    require(out.bsp==c.bsps()[0].datum&&g.ordinal==0,"decorator BSP/group source identity changed");
    require(g.selection->type==1&&g.selection->class_index==1&&g.selection->permutation_index==5&&
        g.selection->flags==4&&g.selection->fade_distance==2&&g.selection->sprite_index==2,
        "class/permutation/type/shader join differs from actual41630 routing");
    const auto* shader=c.find_tag(TagHandle{g.selection->shader},tag_groups::shader);
    require(shader&&shader->name=="scenarios\\decorators\\dirty_interior\\stains",
        "first decorator selected a shader from group byte2 instead of actual permutation");
    require(g.cluster==30&&g.cache_block==0&&g.vertex_first==0&&g.vertex_count==4&&g.index_first==0&&g.index_count==6,
        "first actual floating group range changed");
    const auto& bank=d.caches.at(0);require(bank.header_bytes==(mcc?52u:44u)&&bank.vertices.size()==4&&bank.indices.size()==6,
        "actual compiled floating cache was not loaded");
    require(bank.stream_type==(mcc?51:53)&&bank.stream_stride==(mcc?32:20),"compiled source stream descriptor changed");
    const auto v=source_decorator_decal_vertex_tu15(bank.vertices[0]);
    require(std::bit_cast<std::uint32_t>(v.position.x)==0xc1803a69u&&
        std::bit_cast<std::uint32_t>(v.position.z)==(mcc?0x3e531db2u:0x3e531dd2u),
        "compiled XYZ bits were guessed or original/MCC source distinction lost");
    require(v.uv.x==12927.f/32767.f&&v.uv.y==18175.f/32767.f,
        "source S1 UV fetch normalization differs");
    require(v.color.x==26.f/255.f&&v.color.y==16.f/255.f&&v.color.z==16.f/255.f&&v.source_color_alpha==0,
        "source UB_D3D radiance BGRA/normalization differs");
    require(bank.indices==std::vector<std::uint16_t>({0,2,1,2,0,3}),"actual floating index resource changed");
    const auto retained=out;const auto again=c.load_source_decorator_decals_tu15(0);
    require(again.definitions[0].caches[0].vertices==retained.definitions[0].caches[0].vertices,
        "owning repeat decorator asset changed");
}
void projected_and_source_differences(const std::filesystem::path& root,bool mcc) {
    {
        Cache c(root/"03a_oldmombasa.map");unsigned projected{},vertices{},indices{};
        for(unsigned bsp=0;bsp<c.bsps().size();++bsp) {
            const auto data=c.load_source_decorator_decals_tu15(bsp);
            for(const auto& d:data.definitions)for(const auto& g:d.groups)if(g.selection&&g.selection->type==2) {
                ++projected;vertices+=unsigned(g.vertex_count);indices+=unsigned(g.index_count);
            }
        }
        require(projected==8&&vertices==0&&indices==0,"authored projected decorators fabricated compiled geometry");
    }
    {
        Cache c(root/"07b_forerunnership.map");const auto data=c.load_source_decorator_decals_tu15(2);
        require(data.definitions.size()==1&&data.definitions[0].caches.at(80).vertices.size()==(mcc?36u:40u),
            "known original/MCC removed-quad source distinction was lost");
    }
}
void fetch_boundaries() {
    SourceDecoratorDecalVertexTu15 packed;
    const auto word=[&](unsigned p,std::uint32_t value){for(unsigned n=0;n<4;++n)packed.packed[p+n]=std::byte(value>>(n*8));};
    word(12,0x7fff8000u);word(16,0x11223344u);
    const auto v=source_decorator_decal_vertex_tu15(packed);
    require(v.uv.x==-1&&v.uv.y==1,"source S1 min/max normalization changed");
    require(v.color.x==34.f/255.f&&v.color.y==51.f/255.f&&v.color.z==68.f/255.f&&v.source_color_alpha==17.f/255.f,
        "source UB_D3D4 channel swizzle or alpha preservation changed");
    std::array<std::byte,32> expanded{};const auto f=[&](unsigned p,float value){const auto bits=std::bit_cast<std::uint32_t>(value);
        for(unsigned n=0;n<4;++n)expanded[p+n]=std::byte(bits>>(n*8));};
    f(12,-1);f(16,1);std::copy_n(packed.packed.begin()+16,4,expanded.begin()+28);
    auto canonical=source_decorator_decal_vertex_mcc_tu15(expanded);require(canonical==packed,"MCC source SHORT2 inverse changed endpoint codes");
    f(20,std::numeric_limits<float>::quiet_NaN());f(24,std::numeric_limits<float>::infinity());
    require(source_decorator_decal_vertex_mcc_tu15(expanded)==canonical,"unfetched MCC UV1 affected source stream53");
    const auto reject=[&](unsigned p,float value){f(p,value);bool failed{};
        try{(void)source_decorator_decal_vertex_mcc_tu15(expanded);}catch(const FormatError&){failed=true;}
        require(failed,"nonrepresentable consumed decorator fetch was accepted");};
    reject(12,1.01f);reject(12,std::numeric_limits<float>::quiet_NaN());f(12,0);reject(0,std::numeric_limits<float>::infinity());
}
}
int main(int argc,char** argv)try {
    if(argc!=3)throw halo2::FormatError("expected original and prepared MCC Beaver Creek maps");
    unsigned failures{};
    for(unsigned n=0;n<2;++n)try{cache(argv[n+1],n!=0);}
        catch(const std::exception& e){std::cerr<<(n?"MCC: ":"Xbox: ")<<e.what()<<'\n';++failures;}
    if(failures)return 1;
    projected_and_source_differences(std::filesystem::path(argv[1]).parent_path(),false);
    projected_and_source_differences(std::filesystem::path(argv[2]).parent_path(),true);
    fetch_boundaries();
    std::cout<<"PASS actual original/MCC floating decorator resources, routing and fetch\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
