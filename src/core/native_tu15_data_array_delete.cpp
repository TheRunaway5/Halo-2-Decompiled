#include "halo2/native_tu15_data_array_delete.hpp"
#include "halo2/native_tu15_datum_delete_kernel.hpp"
#include "halo2/physics_contact_runtime.hpp"
#include <array>
#include <bit>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &m,std::uint32_t p,unsigned n=4) {
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v{};
    for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8u);
    return v;
}
void write(SourcePhysicsContactMemory &m,std::uint32_t p,std::uint32_t v,unsigned n=4) {
    std::array<std::byte,4>b{};for(unsigned i=0;i<n;++i)b[i]=std::byte(v>>(i*8u));
    m.write(p,std::span(b).first(n));
}
std::int32_t signed32(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
}
void native_tu15_data_array_delete(SourcePhysicsContactMemory &m,
        std::uint32_t pool,std::uint32_t datum) {
    struct Access {
        SourcePhysicsContactMemory &memory;
        std::uint32_t read(NativeTu15DatumDeletePhase,const NativeTu15DatumDelete&,std::uint32_t at,unsigned width){return halo2::read(memory,at,width);}
        void write(NativeTu15DatumDeletePhase,const NativeTu15DatumDelete&,std::uint32_t at,std::uint32_t value,unsigned width){halo2::write(memory,at,value,width);}
        void prepare(NativeTu15DatumDeletePhase,const NativeTu15DatumDelete&){}
        void complete(NativeTu15DatumDeletePhase,const NativeTu15DatumDelete&){}
    }access{m};
    NativeTu15DatumDelete state;state.pool=pool;state.datum=datum;native_tu15_delete_datum(access,state);
}

void native_tu15_data_array_delete_abi(SourcePhysicsContactMemory &m,
        SourcePhysicsContactRuntimeAbi &a,const SourcePhysicsContactRecordObserver &o) {
    auto &g=a.gp;
    const auto publish=[&](std::uint32_t pc,std::uint32_t at,std::uint32_t value,unsigned n=4){
        SourcePhysicsContactRecordStore event{pc,at,std::vector<std::byte>(n),std::vector<std::byte>(n)};
        m.read(at,event.before);for(unsigned k=0;k<n;++k)event.written[k]=std::byte(value>>(k*8u));
        m.write(at,event.written);if(o)o(event);
    };
    const auto push=[&](std::uint32_t value){g[4]-=4u;write(m,g[4],value);};
    const auto pop=[&](){const auto value=read(m,g[4]);g[4]+=4u;return value;};
    push(g[3]);push(g[6]);g[3]=g[0];g[0]=read(m,g[2]+0x44u);push(g[7]);
    g[7]=read(m,g[2]+0x24u);g[3]&=0xffffu;g[6]=g[7]*g[3]+g[0];
    g[0]=(g[0]&0xffffff00u)|(read(m,g[2]+0x2au,1)>>3u);
    if(g[0]&1u){
        g[1]=g[7];push(g[5]);g[5]=g[1];g[1]>>=2u;g[0]=0xbabababau;g[7]=g[6];
        const bool backwards=(a.eflags&0x400u)!=0;
        while(g[1]){publish(0x16e8d1u,g[7],g[0]);g[7]+=backwards?0xfffffffcu:4u;--g[1];}
        g[1]=g[5]&3u;
        while(g[1]){publish(0x16e8d8u,g[7],g[0],1);g[7]+=backwards?0xffffffffu:1u;--g[1];}
        g[5]=pop();
    }
    g[0]=read(m,g[2]+0x48u);g[1]=std::uint32_t(signed32(g[3])>>5u);g[0]+=g[1]*4u;
    g[1]=g[3]&31u;g[7]=~(1u<<(g[1]&31u));g[1]=read(m,g[0])&g[7];
    publish(0x16e8f8u,g[0],g[1]);publish(0x16e8fau,g[6],0,2);
    if(signed32(g[3])<signed32(read(m,g[2]+0x34u)))publish(0x16e904u,g[2]+0x34u,g[3]);
    g[0]=read(m,g[2]+0x38u);++g[3];
    if(g[3]==g[0]){
        g[0]=read(m,g[2]+0x24u);
        do {
            g[7]=read(m,g[2]+0x38u);g[6]-=g[0];--g[7];g[1]=g[7];
            publish(0x16e91cu,g[2]+0x38u,g[7]);
        }while(signed32(g[1])>0&&read(m,g[6],2)==0);
    }
    g[0]=read(m,g[2]+0x3cu);g[7]=pop();--g[0];g[6]=pop();
    publish(0x16e92du,g[2]+0x3cu,g[0]);g[3]=pop();(void)pop();
}
}
