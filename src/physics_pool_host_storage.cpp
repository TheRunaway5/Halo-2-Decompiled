#include "halo2/physics_pool_host_storage.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &m, std::uint32_t p) {
  std::array<std::byte, 4> b;
  m.read(p,b);
  std::uint32_t v=0;
  for(unsigned i=0;i<4;++i) v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);
  return v;
}
void write(SourcePhysicsContactMemory &m,std::uint32_t p,std::uint32_t v) {
  std::array<std::byte,4> b;
  for(unsigned i=0;i<4;++i) b[i]=std::byte(v>>(8*i));
  m.write(p,b);
}
}
SourcePhysicsPoolHostStorage::SourcePhysicsPoolHostStorage(SourceRuntimeMemory &memory,
    std::uint32_t first,const SourcePhysicsPoolResourceObserver &observe)
    :memory_(memory),raw_(memory,first),observe_(observe) {}
SourcePhysicsPoolHostStorage::~SourcePhysicsPoolHostStorage() {
  for(const auto &[base,bytes]:allocations_) {
    // Only this owner's banks may be released. Do not throw while unwinding.
    try { memory_.release(base); } catch(...) {}
  }
}
bool SourcePhysicsPoolHostStorage::try_call(SourcePhysicsContactMemory &m,
    const SourcePhysicsContactRuntimeCall &call,SourcePhysicsWorldRuntimeAbi &abi) {
  const bool allocate=call.instruction==0x33786fu && call.target==0x800000b8u;
  const bool release=call.instruction==0x3378abu && call.target==0x800000c7u;
  if(!allocate && !release) return false;
  if(&m!=&memory_ || call.tail)
    throw std::runtime_error("Original physics pool storage/call owner mismatch");
  const auto sp=abi.gp[4];
  if(read(m,sp)!=(allocate?0x33b3c5u:0x33b401u))
    throw std::runtime_error("Original TU1.5 virtual-memory return owner mismatch");
  const auto base_cell=read(m,sp+4);
  if(allocate) {
    const auto bytes_cell=read(m,sp+12);
    const SourceVirtualAllocationRequest request{read(m,base_cell),read(m,sp+8),
        read(m,bytes_cell),read(m,sp+16),read(m,sp+20)};
    const auto result=raw_.allocate(request);
    try { allocations_.emplace(result.base,result.bytes); }
    catch(...) {memory_.release(result.base);throw;}
    write(m,base_cell,result.base);write(m,bytes_cell,result.bytes);
    if(observe_) observe_({true,{request.base,request.zero_bits,request.bytes,
        request.allocation_type,request.protection},{result.status,result.base,result.bytes}});
    abi.gp[0]=result.status;abi.gp[4]+=24u;
  } else {
    const auto bytes_cell=read(m,sp+8),base=read(m,base_cell),
        bytes=read(m,bytes_cell),type=read(m,sp+12);
    const auto found=allocations_.find(base);
    if(bytes || type!=0x8000u || found==allocations_.end())
      throw std::runtime_error("Unowned original physics pool release request");
    memory_.release(base);allocations_.erase(found);
    // These by-reference input cells are dead in the complete source caller.
    // This native ABI leaves them unchanged; no Xbox output-cell claim.
    if(observe_) observe_({false,{base,bytes,type,0,0},{0,base,bytes}});
    abi.gp[0]=0;abi.gp[4]+=16u;
  }
  return true;
}
void source_run_physics_pool_startup_with_native_storage(SourceRuntimeMemory &m,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices &services,
    SourcePhysicsPoolHostStorage &host,const SourcePhysicsContactRecordObserver &observe) {
  if(!host.backs(m)) throw std::runtime_error("Original physics pool backing identity mismatch");
  auto joined=services;
  joined.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &original_observer) {
    if(host.try_call(bank,call,state)) return;
    if(!services.required_owner)
      throw std::runtime_error("Required original physics pool callee unavailable after native storage join");
    services.required_owner(bank,call,state,original_observer);
  };
  source_run_physics_pool_startup_runtime(m,entry,abi,SourcePhysicsContactImage::TitleUpdate15,
      fp,joined,observe);
}
} // namespace halo2
