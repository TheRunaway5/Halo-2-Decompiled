#include "halo2/native_tu15_island_construction.hpp"
namespace halo2 {
namespace {
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,std::uint32_t site,
 std::uint32_t p,std::uint32_t value,unsigned size=4){std::array<std::byte,4>b{};for(unsigned n=0;n<size;++n)b[n]=std::byte(value>>(8u*n));SourcePhysicsContactRecordStore e{site,p,{},{}};if(o){e.before.resize(size);m.read(p,e.before);e.written.assign(b.begin(),b.begin()+size);}m.write(p,std::span(b).first(size));if(o)o(e);}
}
NativeTu15PhysicsIsland native_tu15_construct_physics_island(SourcePhysicsContactMemory&m,
 NativeTu15PhysicsIsland island,NativeTu15PhysicsWorld world,const SourcePhysicsContactRecordObserver&o){
 const auto p=island.identity;
 store(m,o,0x3212c8u,p+8u,0);store(m,o,0x3212cbu,p+0xcu,0);store(m,o,0x3212ceu,p+0x10u,0);store(m,o,0x3212d1u,p+0x14u,0);store(m,o,0x3212d4u,p+0x18u,0);
 store(m,o,0x3212d7u,p+0x1cu,0x41d92cu);store(m,o,0x3212deu,p+0x20u,world.identity);store(m,o,0x3212e1u,p,0x41f080u);store(m,o,0x3212e7u,p+0x1cu,0x41f074u);
 store(m,o,0x3212eeu,p+0x28u,0,1);store(m,o,0x3212f3u,p+0x29u,1,1);store(m,o,0x3212f6u,p+0x2cu,0,1);store(m,o,0x3212f9u,p+0x30u,1,1);
 store(m,o,0x3212fcu,p+0x34u,0);store(m,o,0x3212ffu,p+0x38u,0);store(m,o,0x321302u,p+0x40u,0);store(m,o,0x321305u,p+0x44u,0x80000001u);store(m,o,0x32130fu,p+0x3cu,p+0x48u);
 store(m,o,0x321312u,p+0x4cu,0);store(m,o,0x321315u,p+0x50u,0);store(m,o,0x32131du,p+0x54u,0x80000000u);store(m,o,0x321320u,p+0x5cu,0);store(m,o,0x321323u,p+0x60u,0x80000001u);store(m,o,0x32132eu,p+0x58u,p+0x64u);
 store(m,o,0x321331u,p+0x68u,0);store(m,o,0x321334u,p+0x6cu,0);store(m,o,0x321337u,p+0x70u,0x80000000u);store(m,o,0x32133au,p+0x74u,0);store(m,o,0x32133du,p+0x78u,0);store(m,o,0x321340u,p+0x7cu,0x80000000u);
 store(m,o,0x321343u,p+0x80u,0);store(m,o,0x321349u,p+0x84u,0);store(m,o,0x32134fu,p+0x88u,0x80000000u);store(m,o,0x321355u,p+0x8cu,0);store(m,o,0x32135bu,p+0x90u,0);store(m,o,0x321361u,p+0x94u,0x80000000u);
 store(m,o,0x321367u,p+0xa8u,0xffffffffu);store(m,o,0x321371u,p+0xacu,0,2);store(m,o,0x321378u,p+0xaeu,0,1);return island;
}
} // namespace halo2
