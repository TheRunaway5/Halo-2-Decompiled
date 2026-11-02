#include "halo2/native_tu15_joint_constraint_schema.hpp"
namespace halo2 {
void native_tu15_begin_joint_constraint_schemas(SourcePhysicsContactMemory&memory,
    NativeTu15ConstraintProducerInput input,NativeTu15ConstraintSchemaPacket builder,
    std::uint32_t runtime,std::uint32_t runtime_bytes,const SourcePhysicsContactRecordObserver&observe){
    const auto word=[&](std::uint32_t p){std::array<std::byte,4>b{};memory.read(p,b);
        std::uint32_t v{};for(unsigned n=0;n<4;++n)v|=std::to_integer<unsigned>(b[n])<<(n*8u);return v;};
    const auto store=[&](std::uint32_t pc,std::uint32_t p,std::uint32_t value){
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(observe){event.before.resize(4);memory.read(p,event.before);}
        std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8u));memory.write(p,b);
        if(observe){event.written.assign(b.begin(),b.end());observe(event);}
    };
    const auto schema=word(builder.identity+4u);const auto jacobians=word(builder.identity);
    const auto second=word(input.identity+4u);const auto first=word(input.identity);
    store(0x328884u,schema+12u,first);store(0x32888bu,schema+8u,runtime);
    store(0x328892u,schema+16u,second);store(0x328895u,schema+20u,runtime_bytes);
    store(0x328898u,schema,0x11801u);store(0x32889eu,schema+4u,jacobians);
    store(0x3288a5u,builder.identity+4u,schema+24u);
}
} // namespace halo2
