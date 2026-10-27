#include "halo2/native_tu15_constraint_schema_emitter.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t word(SourcePhysicsContactMemory&m,std::uint32_t p){std::array<std::byte,4>b{};m.read(p,b);std::uint32_t result{};for(unsigned n=0;n<4;++n)result|=std::to_integer<unsigned>(b[n])<<(n*8u);return result;}
void word(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&observe,std::uint32_t site,std::uint32_t p,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8u));SourcePhysicsContactRecordStore store{site,p,{},{}};if(observe){store.before.resize(4);m.read(p,store.before);store.written.assign(b.begin(),b.end());}m.write(p,b);if(observe)observe(store);}
}
std::uint32_t native_tu15_emit_custom_constraint_schema(SourcePhysicsContactMemory&m,std::uint32_t bytes,NativeTu15ConstraintSchemaPacket packet,const NativeTu15ConstraintSchemaCallbackSource&callback,const SourcePhysicsContactRecordObserver&observe,NativeTu15ConstraintSchemaEmitterStats*stats,const NativeTu15ConstraintSchemaPhasePublisher&phase){
    if(stats)++stats->calls;if(phase)phase(0x32a398u);const auto schema=word(m,packet.identity+4u);const auto encoded=((bytes+8u)<<8u)|0x2000eu;
    if(phase)phase(0x32a3a8u);
    word(m,observe,0x32a3a8u,schema,encoded);
    if(!callback)throw std::runtime_error("Native TU1.5 custom schema emission requires its actual late callback input");
    if(phase)phase(0x32a3aau);const auto owner=callback();if(phase)phase(0x32a3aeu);word(m,observe,0x32a3aeu,schema+4u,owner.identity);
    const auto payload=schema+8u;if(phase)phase(0x32a3b6u);word(m,observe,0x32a3b6u,packet.identity+4u,bytes+payload);return payload;
}
} // namespace halo2
