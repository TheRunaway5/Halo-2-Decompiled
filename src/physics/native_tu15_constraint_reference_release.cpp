#include "halo2/native_tu15_constraint_reference_release.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;}
}
void native_tu15_release_constraint_reference(SourcePhysicsContactMemory&memory,std::uint32_t instance,
 std::uint32_t datum,NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ConstraintReferenceReleaseServices&services,
 const SourcePhysicsContactRecordObserver&observer,NativeTu15ConstraintReferenceReleaseStats*stats){
    if(stats)++stats->calls;
    if(word(memory,instance+8u,2)==1u&&word(memory,instance+0x10u,1)==1u){
        const auto slots=native_tu15_constraint_body_slots(memory,instance,datum,stats?&stats->slots:nullptr);
        const auto scalar=word(memory,instance+0x14u);native_tu15_update_constraint_cached_scalar(memory,instance,scalar,fp,depth,observer,stats?&stats->cache:nullptr);
        if(!services.remove_runtime_constraint){if(stats)++stats->unbound_runtime_removals;throw std::runtime_error("Native TU1.5 constraint reference release requires actual22FEA0 runtime ownership");}
        if(stats)++stats->runtime_removals;services.remove_runtime_constraint(instance,datum,slots,fp);
    }
    const auto value=word(memory,instance+8u,2)-1u;std::array<std::byte,2>bytes{std::byte(value),std::byte(value>>8u)};
    SourcePhysicsContactRecordStore event{0x22e201u,instance+8u,{},{}};if(observer){event.before.resize(2);memory.read(instance+8u,event.before);event.written.assign(bytes.begin(),bytes.end());}memory.write(instance+8u,bytes);if(observer)observer(event);if(stats)++stats->decrements;
}
} // namespace halo2
