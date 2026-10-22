#include "halo2/physics_physical_storage_runtime.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory &m,std::uint32_t p) {
  std::array<std::byte,4>b;m.read(p,b);std::uint32_t v=0;
  for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;
}
}
void source_run_physics_physical_storage_with_native_pages(SourceRuntimeMemory &m,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices &services,
    SourceRuntimeHostPhysicalMemory &physical,const SourcePhysicsContactRecordObserver &observe,
    const SourcePhysicsPhysicalResourceObserver &resource) {
  if(!physical.backs(m))throw std::invalid_argument("Original physical stage bank owner mismatch");
  auto joined=services;
  joined.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &o) {
    const bool allocation=call.instruction==0x2d79d3u && call.target==0x800000a6u && !call.tail;
    const bool protection=call.instruction==0x2d7a21u && call.target==0x800000b6u && call.tail;
    if(allocation||protection) {
      if(&bank!=&m || word(m,allocation?0x41b640u:0x41b648u)!=call.target)
        throw std::invalid_argument("Original TU1.5 physical stage import owner mismatch");
      const auto sp=state.gp[4];
      if(allocation) {
        if(word(m,sp)!=0x2db489u)
          throw std::invalid_argument("Original TU1.5 physical allocation return differs");
        const SourcePhysicalAllocationRequest q{word(m,sp+4),word(m,sp+8),word(m,sp+12),word(m,sp+16),word(m,sp+20)};
        state.gp[0]=physical.allocate(q);state.gp[4]=sp+24;
        if(resource)resource({true,{q.bytes,q.lowest,q.highest,q.alignment,q.protection},state.gp[0]});
      } else {
        (void)word(m,sp); // The source caller owns the tail-dispatch return.
        const auto address=word(m,sp+4),bytes=word(m,sp+8),protection=word(m,sp+12);
        physical.protect(address,bytes,protection);state.gp[4]=sp+16;
        if(resource)resource({false,{address,bytes,protection,0,0},state.gp[0]});
      }
      return;
    }
    if(!services.required_owner)
      throw std::runtime_error("Required original physical stage callee unavailable");
    services.required_owner(bank,call,state,o);
  };
  source_run_physics_physical_storage_runtime(m,entry,abi,SourcePhysicsContactImage::TitleUpdate15,fp,joined,observe);
}
} // namespace halo2
