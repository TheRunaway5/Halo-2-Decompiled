#include "halo2/source_model_node_bank.hpp"
#include "halo2/source_model_palette.hpp"
#include <bit>
#include <cstring>

namespace halo2 {
namespace {
template<class T>T read(const SourceModelResourceWorld& w,std::uint32_t at) {
    if(!w.banks.read)throw FormatError("source node bank reader absent");
    const auto raw=w.banks.read(at,sizeof(T));
    if(raw.size()!=sizeof(T))throw FormatError("source node input truncated");
    T value;std::memcpy(&value,raw.data(),sizeof(T));return value;
}
template<class T>void put(const SourceModelResourceWorld& w,std::uint32_t at,const T& value) {
    if(!w.write)throw FormatError("source node bank writer absent");
    auto raw=w.write(at,sizeof(T));
    if(raw.size()!=sizeof(T))throw FormatError("source node output truncated");
    std::memcpy(raw.data(),&value,sizeof(T));
}
std::uint32_t metadata(const SourceModelResourceWorld& w,std::uint32_t tag) {
    return read<std::uint32_t>(w,w.banks.tag_table+(tag&65535u)*16+8);
}
using Matrix=std::array<float,12>;
Matrix skin(const SourceModelResourceWorld& w,std::uint32_t nodes,std::uint32_t inverse) {
    static_assert(sizeof(Transform)==52);
    const std::array<Transform,1> world{read<Transform>(w,nodes)},bind{read<Transform>(w,inverse)};
    return source_model_skin_palette(world,bind).front();
}
std::uint32_t deferred(std::uint32_t owner,std::uint32_t selected,std::uint32_t nodes,
    const SourceModelNodeWorld& world) {
    const auto& w=world.memory;
    const auto headers=read<std::uint32_t>(w,w.banks.objects+0x44);
    const auto body=read<std::uint32_t>(w,headers+(owner&65535u)*12+8);
    if(read<std::int16_t>(w,body+0x112)==-1)return nodes;
    if(owner!=0xffffffffu && read<std::uint8_t>(w,body+0xb3)>0)return nodes;
    if(!world.deferred_nodes)throw FormatError("sourceBD610 deferred-node owner absent");
    return world.deferred_nodes(owner,selected,nodes);
}
}
std::int32_t source_model_skin_node_bank(std::uint32_t model,std::uint32_t nodes,
    std::uint32_t output,const SourceModelResourceWorld& w) {
    const auto m=metadata(w,model);auto count=read<std::int32_t>(w,m+0x48);
    for(std::int32_t i=0;i<count;++i) {
        if(i>=256)throw FormatError("source skin node bank exceeds256 nodes");
        const auto inverse=read<std::uint32_t>(w,m+0x4c)+std::uint32_t(i)*96+0x28;
        put(w,output+std::uint32_t(i)*48,skin(w,nodes+std::uint32_t(i)*52,inverse));
        count=read<std::int32_t>(w,m+0x48);
    }
    return read<std::int32_t>(w,m+0x48);
}
void source_model_geometry_node_bank(std::uint32_t owner,std::uint32_t model,
    std::uint32_t nodes,std::uint32_t selected,std::int32_t lod,
    std::uint32_t first_person,std::uint32_t output,const SourceModelNodeWorld& world) {
    const auto& w=world.memory;const auto m=metadata(w,model);
    const auto group=read<std::int8_t>(w,m+std::uint32_t(lod)+0x3c);
    if(!(first_person&255u) && owner!=0xffffffffu)nodes=deferred(owner,selected,nodes,world);
    if(!(first_person&255u) && owner!=0xffffffffu && world.observed_world_nodes)
        world.observed_world_nodes(owner,nodes,read<std::int32_t>(w,m+0x48));
    const bool compact=(read<std::uint8_t>(w,m+4)&4)!=0;
    put(w,output+2,read<std::uint16_t>(w,m+0x1c));
    // Compact palettes have a private255-matrix bank. Ordinary palettes use
    // the current output directly, including already appended region nodes.
    std::array<Matrix,255> local;std::uint32_t initialized=0;
    std::int32_t count;
    if(compact) {
        count=read<std::int32_t>(w,m+0x48);
        for(std::int32_t i=0;i<count;++i) {
            if(i>=255)throw FormatError("source compact node bank exceeds255 nodes");
            local[unsigned(i)]=skin(w,nodes+std::uint32_t(i)*52,
                read<std::uint32_t>(w,m+0x4c)+std::uint32_t(i)*96+0x28);
            ++initialized;count=read<std::int32_t>(w,m+0x48);
        }
        count=read<std::int32_t>(w,m+0x48);
    } else count=source_model_skin_node_bank(model,nodes,output+0x44,w);
    if(count>256)throw FormatError("source node bank invalid signed base count");
    const auto group_at=read<std::uint32_t>(w,m+0x38)+std::uint32_t(std::int32_t(group))*12;
    const auto get=[&](std::uint32_t index) {
        if(compact) {
            if(index>=initialized)throw FormatError("source compact node reads uninitialized matrix");
            return local[index];
        }
        return read<Matrix>(w,output+0x44+index*48);
    };
    for(std::uint32_t i=0;std::bit_cast<std::int32_t>(i)<read<std::int32_t>(w,group_at+4);++i) {
        const auto at=read<std::uint32_t>(w,group_at+8)+i*16;
        Matrix matrix{};float total=0;
        for(unsigned j=0;j<4;++j) {
            const auto index=read<std::uint8_t>(w,at+j);if(index==255)break;
            const float weight=j<3?read<float>(w,at+4+j*4):1.f-total;
            const auto prior=get(index);
            for(unsigned k=0;k<12;++k)matrix[k]=prior[k]*weight+matrix[k];
            total=weight+total;
        }
        if(count<0 || count>= (compact?255:256))throw FormatError("source compound node bank exceeds capacity");
        if(compact){local[unsigned(count)]=matrix;++initialized;}
        else put(w,output+0x44+std::uint32_t(count)*48,matrix);
        ++count;
    }
    if(read<std::uint8_t>(w,m+4)&4) {
        if(!initialized)throw FormatError("source compact node bank lacks first matrix");
        put(w,output+0x44,local[0]);count=1;
    }
    put(w,output,std::uint16_t(count));
    for(std::uint32_t region=0;std::bit_cast<std::int32_t>(region)<read<std::int32_t>(w,m+0x1c);++region) {
        if(region>=16)throw FormatError("source node bank region count exceeds16");
        const auto range=output+4+region*4;
        const auto section=read<std::uint8_t>(w,selected+region);
        if(section==255) {put<std::uint16_t>(w,range,0);put<std::uint16_t>(w,range+2,0);continue;}
        const auto at=read<std::uint32_t>(w,m+0x28)+std::uint32_t(section)*0x5c;
        std::uint32_t resident=0;
        if(!world.resource_cache_present)resident=read<std::uint32_t>(w,at+0x34);
        else {
            if(!w.banks.request)throw FormatError("source node resource publication owner absent");
            if(w.banks.request(0,at+0x38))resident=read<std::uint32_t>(w,at+0x34);
        }
        // A failed source request dereferences a null resident bank. Reject
        // that undefined native input rather than invent a fallback palette.
        if(!resident)throw FormatError("source node map resident bank absent");
        if(read<std::int32_t>(w,resident+0x64)>0) {
            const auto map=read<std::uint32_t>(w,resident+0x68);
            put(w,range,std::uint16_t(count));put(w,range+2,read<std::uint16_t>(w,resident+0x64));
            for(std::uint32_t i=0;std::bit_cast<std::int32_t>(i)<read<std::int32_t>(w,resident+0x64);++i) {
                if(count>=256)throw FormatError("source mapped node bank exceeds256 nodes");
                const auto index=read<std::uint8_t>(w,map+i);
                if(compact)put(w,output+0x44+std::uint32_t(count)*48,get(index));
                else for(unsigned k=0;k<12;++k)put(w,output+0x44+std::uint32_t(count)*48+k*4,
                    read<std::uint32_t>(w,output+0x44+std::uint32_t(index)*48+k*4));
                ++count;
            }
        } else {
            put<std::uint16_t>(w,range,0);put(w,range+2,std::uint16_t(read<std::uint8_t>(w,at+0x24)));
            if(!read<std::uint16_t>(w,range+2)) {
                const auto n=read<std::int32_t>(w,m+0x48);
                put(w,range+2,std::uint16_t(std::min(n,40)));
            }
        }
    }
}
}
