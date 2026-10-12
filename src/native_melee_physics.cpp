#include "halo2/native_melee_physics.hpp"
#include "halo2/unit_animation_selection_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/bytes.hpp"
#include <bit>
namespace halo2 {
void source_run_melee_physics_1_1(SourcePhysicsContactMemory &,std::uint32_t,
    SourceUnitAnimationRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourceUnitAnimationRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
namespace {
void put(SourceRuntimeMemory &m,std::uint32_t p,std::uint32_t v){auto b=m.mutable_view(p,4);for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(i*8));}
void vec(SourceRuntimeMemory &m,std::uint32_t p,Vec3 v){put(m,p,std::bit_cast<std::uint32_t>(v.x));put(m,p+4,std::bit_cast<std::uint32_t>(v.y));put(m,p+8,std::bit_cast<std::uint32_t>(v.z));}
void mount(SourceRuntimeMemory &m,NativeMeleeLunge &s,const NativeMeleePhysicsOperands &o){
 for(const auto &[address,bytes]:o.literals)m.mount(address,bytes,SourceRuntimeMemory::Access::ReadOnly,"original melee physics literal");
 m.mount(0x60000000,std::vector<std::byte>(0x1000),SourceRuntimeMemory::Access::ReadWrite,"native melee retained state and arguments");
 m.mount(0x70000000,std::vector<std::byte>(0x10000),SourceRuntimeMemory::Access::ReadWrite,"native melee physics stack");
 for(unsigned i=0;i<3;++i){
  const std::array<std::uint32_t,3> pointers{0x46fc28,0x46fc44,0x46fc48};
  const auto address=0x60000200+i*16;std::vector<std::byte> pointer(4);
  for(unsigned j=0;j<4;++j)pointer[j]=std::byte(address>>(j*8));
  m.mount(pointers[i],pointer,SourceRuntimeMemory::Access::ReadOnly,"original melee basis pointer binding");
  m.write(address,o.basis[i]);
 }
 std::vector<std::byte> clock_pointer(4);for(unsigned i=0;i<4;++i)clock_pointer[i]=std::byte(0x60000240u>>(i*8));
 m.mount(0x518444,clock_pointer,SourceRuntimeMemory::Access::ReadOnly,"native30Hz melee clock binding");
 put(m,0x60000240,30u<<16);put(m,0x60000244,std::bit_cast<std::uint32_t>(1.f/30.f));
 m.write(0x60000000,s.state);
}
}
void start_native_melee_lunge(NativeMeleeLunge &s,std::uint32_t target,bool sword,const NativeMeleePhysicsOperands &o){
 s={};SourceRuntimeMemory m;mount(m,s,o);SourceUnitAnimationRuntimeAbi a{};a.gp[0]=0x60000000;a.gp[4]=0x70008000;a.eflags=2;
 source_run_melee_physics_1_1(m,0x1ef030,a,{0x27f,0x1f80},{});
 std::copy_n(m.view(0x60000000,0x64).begin(),0x64,s.state.begin());s.state[0xc]=std::byte(sword);s.target=target;s.active=true;
}
Vec3 tick_native_melee_lunge(NativeMeleeLunge &s,float distance,Vec3 tv,Vec3 tp,Vec3 aim,Vec3 velocity,Vec3 position,const NativeMeleePhysicsOperands &o){
 SourceRuntimeMemory m;mount(m,s,o);vec(m,0x60000300,tv);vec(m,0x60000310,tp);vec(m,0x60000320,aim);vec(m,0x60000330,velocity);vec(m,0x60000340,position);
 SourceUnitAnimationRuntimeAbi a{};a.gp[0]=1;a.gp[1]=0x60000300;a.gp[2]=0x60000310;a.gp[6]=0x60000000;a.gp[4]=0x70008000;a.eflags=2;
 // Actual1EF290 ABI: AL target-valid,ECX target velocity,EDX target point,
 // ESI state,stack output,distance,aim,velocity,position; callee removes20.
 for(unsigned i=0;i<5;++i)put(m,a.gp[4]+4+i*4,std::array<std::uint32_t,5>{0x60000400,std::bit_cast<std::uint32_t>(distance),0x60000320,0x60000330,0x60000340}[i]);
 source_run_melee_physics_1_1(m,0x1ef290,a,{0x27f,0x1f80},{});
 std::copy_n(m.view(0x60000000,0x64).begin(),0x64,s.state.begin());const Bytes output(m.view(0x60000400,0x70));s.active=!(output.u32(0)&4);
 return {output.f32(12),output.f32(16),output.f32(20)};
}
}
