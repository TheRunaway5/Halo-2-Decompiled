#include "halo2/source_combat_action_tu15.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &m,std::uint32_t p,unsigned n=4) {
  std::array<std::byte,4> b{};m.read(p,std::span(b).first(n));std::uint32_t v{};
  for(unsigned k=0;k<n;++k)v|=std::to_integer<unsigned>(b[k])<<(k*8);return v;
}
void write(SourcePhysicsContactMemory &m,std::uint32_t p,std::uint32_t v) {
  std::array<std::byte,4> b;for(unsigned k=0;k<4;++k)b[k]=std::byte(v>>(k*8));m.write(p,b);
}
}
void source_native_cache_bitmap_view_tu15(SourcePhysicsContactMemory &m,
    SourcePhysicsWorldRuntimeAbi &a,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices &services,const SourcePhysicsContactRecordObserver &observe) {
  const auto sp=a.gp[4],texture=a.gp[7],output=a.gp[6],level=a.gp[3];
  if(read(m,sp)!=0x1d2efu || output!=sp+0x24u || level!=0 ||
      (read(m,texture)&0x70000u)!=0x40000u)
    throw std::runtime_error("TU1.5 native cache bitmap view differs from its authenticated texture caller");
  // Mirror the original stack layout so the complete metadata helper's
  // temporary/output ownership and the source caller's observed ABI agree.
  const auto base=sp-0x14u,format=read(m,texture+0xdu,1);
  write(m,output,format);a.gp[0]=format;a.gp[1]=texture;
  a.gp[4]=base-4u;write(m,a.gp[4],0x407c10u);
  source_run_combat_action_tu15(m,0x405770u,a,fp,services,observe);
  write(m,output+4u,a.gp[0]);write(m,output+8u,0);
  const auto flags=read(m,0x40db30u+format,1);
  if(flags&0x80u)write(m,output+8u,1u);
  else if(flags&0x40u)write(m,output+8u,2u);
  // output+10 is GPU multisample state. It is not an input to any instruction
  // of the complete1D2D0 caller and is intentionally not manufactured here.
  const std::array<std::uint32_t,5> outputs{base,base+4u,base+0x10u,base+0xcu,base+8u};
  a.gp[4]=base-24u;write(m,a.gp[4],0x407c87u);
  for(unsigned n=0;n<outputs.size();++n)write(m,a.gp[4]+4u+n*4u,outputs[n]);
  a.gp[0]=level;a.gp[1]=texture;
  source_run_combat_action_tu15(m,0x407aa0u,a,fp,services,observe);
  a.gp[2]=read(m,base);a.gp[0]=read(m,base+4u);a.gp[1]=read(m,base+8u);
  write(m,output+0x14u,a.gp[2]);write(m,output+0x18u,a.gp[0]);write(m,output+0xcu,a.gp[1]);
  physics_contact_runtime_detail::Flags arithmetic;arithmetic.load(a.eflags);
  a.gp[4]=arithmetic.plus(base,0x14u,4)+4u;
  a.eflags=(a.eflags&~0xcd5u)|arithmetic.bits();
}
void source_native_cache_vertex_view_tu15(SourcePhysicsContactMemory &m,SourcePhysicsWorldRuntimeAbi &a) {
  const auto sp=a.gp[4],handle=a.gp[0];
  if(read(m,sp)!=0x183e1cu || handle==0xffffffffu)
    throw std::runtime_error("TU1.5 native vertex view differs from its actual decal caller");
  const auto cache=read(m,0x515f28u),resource=read(m,0x515f24u);
  const auto pool=read(m,cache+0x64u),index=handle&0xffffu;
  if(read(m,pool+0x24u)!=0x18u || index>=read(m,pool+0x38u) ||
      !(read(m,read(m,pool+0x48u)+(index>>5)*4u)&(1u<<(index&31u))))
    throw std::runtime_error("TU1.5 vertex view lost its retained cache allocation");
  const auto row=read(m,pool+0x44u)+index*0x18u,shift=read(m,cache+0x34u);
  if(read(m,row,2)!=(handle>>16) || shift>=32u || !resource)
    throw std::runtime_error("TU1.5 vertex view cache generation/shift/resource differs");
  const auto offset=read(m,row+8u)<<(shift&31u);
  const auto base=read(m,resource+4u)|0x80000000u;
  physics_contact_runtime_detail::Flags arithmetic;arithmetic.load(a.eflags);
  const auto address=arithmetic.plus(base,offset,4);
  // The resource is already allocated by the original cache constructor.
  // Require its real physical bank instead of allocating a parallel buffer.
  std::array<std::byte,1> backing;m.read(address,backing);
  write(m,row+0x14u,read(m,cache+0x38u));
  a.gp[0]=address;a.gp[1]=shift;a.gp[2]=read(m,cache+0x38u);
  a.gp[4]=sp+4u;a.eflags=(a.eflags&~0xcd5u)|arithmetic.bits();
}
} // namespace halo2
