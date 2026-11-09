#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include <bit>

namespace halo2::native_model_default_pose_detail {
inline std::uint32_t word(const SourcePhysicsContactMemory& memory,std::uint32_t at) {
    std::array<std::byte,4> bytes{};memory.read(at,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);return value;
}
inline void publish(SourcePhysicsContactMemory& memory,const SourcePhysicsContactRecordObserver& observe,
    std::uint32_t site,std::uint32_t at,std::uint32_t value) {
    SourcePhysicsContactRecordStore event{site,at,{},{}};std::array<std::byte,4> bytes{};
    if(observe){event.before.resize(4);memory.read(at,event.before);}
    for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8u*n));memory.write(at,bytes);
    if(observe){event.written.assign(bytes.begin(),bytes.end());observe(event);}
}
struct CopyResult {
    std::uint32_t model{},destination{},source_offset{},copied{},count{};
};
enum class CopyPhase {nodes,node,read_component,write_component,reload_model,scale,count,advanced};
struct CopyProgress {
    CopyResult result;
    std::uint32_t records{},source{},lane{},value{};
};
// Actual1707A0 gathers quaternion/translation defaults from 96-byte model
// nodes into 32-byte pose rows. Source fields stay lazy: outputs may alias
// later source words, the model descriptor or the caller's model cell.
template<class ReloadModel,class Progress>
CopyResult copy(SourcePhysicsContactMemory& memory,std::uint32_t model,std::uint32_t destination,
    std::uint32_t one,ReloadModel&& reload_model,const SourcePhysicsContactRecordObserver& observe,Progress progress) {
    CopyProgress data{{model,destination}};auto& result=data.result;
    do {
        progress(CopyPhase::nodes,data);data.records=word(memory,result.model+0x4cu);
        data.source=data.records+result.source_offset;progress(CopyPhase::node,data);
        constexpr std::array sites{0x1707cfu,0x1707d4u,0x1707dau,0x1707e0u,
                                   0x1707ebu,0x1707f0u,0x1707f6u};
        for(unsigned n=0;n<7;++n) {
            data.lane=n;progress(CopyPhase::read_component,data);
            const auto offset=n<4?0x18u+n*4u:0xcu+(n-4u)*4u;
            data.value=word(memory,data.source+offset);progress(CopyPhase::write_component,data);
            publish(memory,observe,sites[n],result.destination+n*4u,data.value);
        }
        progress(CopyPhase::reload_model,data);
        result.model=reload_model();
        progress(CopyPhase::scale,data);
        publish(memory,observe,0x1707fdu,result.destination+28u,one);
        progress(CopyPhase::count,data);
        result.count=word(memory,result.model+0x48u);
        ++result.copied;result.source_offset+=0x60u;result.destination+=0x20u;
        progress(CopyPhase::advanced,data);
    } while(std::bit_cast<std::int32_t>(result.copied)<std::bit_cast<std::int32_t>(result.count));
    return result;
}
} // namespace halo2::native_model_default_pose_detail
