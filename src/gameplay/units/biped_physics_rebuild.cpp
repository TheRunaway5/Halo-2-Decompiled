#include "halo2/biped_physics_rebuild.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
using M=SourcePhysicsContactMemory;
using O=SourcePhysicsContactRecordObserver;
std::uint32_t r(const M&m,std::uint32_t p,unsigned n=4){
  std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v=0;
  for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8);return v;
}
void w(M&m,const O&o,std::uint32_t site,std::uint32_t p,unsigned n,std::uint32_t v){
  SourcePhysicsContactRecordStore store{site,p,{},{}};store.before.resize(n);store.written.resize(n);
  m.read(p,store.before);for(unsigned i=0;i<n;++i)store.written[i]=std::byte(v>>(i*8));
  m.write(p,store.written);if(o)o(store);
}
std::uint32_t object(const M&m,std::uint32_t d){return r(m,r(m,r(m,0x4e78d0)+0x44)+(d&0xffffu)*12u+8);}
std::uint32_t tag(const M&m,std::uint32_t d){return r(m,r(m,0x4eb114)+(d&0xffffu)*16u+8);}
template<class F,class...A>decltype(auto)required(const F&function,const char*message,A&&...args){
  if(!function)throw std::runtime_error(message);return function(std::forward<A>(args)...);
}
}
bool source_biped_physical_constraint_1_1(const M&m,std::uint32_t datum){
  const auto unit=object(m,datum);
  return r(m,unit+0x3dc,1)==6&&r(m,unit+0x3f8,1)!=0&&(r(m,unit+0x348,2)&0x2000u);
}
void source_rebuild_biped_physics_1_1(M&m,std::uint32_t component,
    const SourceBipedPhysicsRebuildServices&s,const O&o){
  const auto datum=r(m,component+8),unit=object(m,datum),definition=tag(m,r(m,unit));
  const auto model=tag(m,r(m,definition+0x38));
  const bool constrained=(r(m,unit+0x134)&0x800000u)||source_biped_physical_constraint_1_1(m,datum);
  const bool dead=r(m,unit+0x3dc,1)==3;
  std::uint8_t controller_class;
  if(dead&&r(m,model+0x24)!=0xffffffffu&&
      (r(m,unit+0x34b,1)==1||(r(m,definition+0x1f0)&0x800u))){
    const auto result=required(s.group,"Required original biped rigid-body grouping owner unavailable",component);
    w(m,o,0x1da622,component+0x1c,1,result);
    if(r(m,component+0x74)){
      w(m,o,0x1da64a,component+4,4,r(m,component+4)|0x40000u);return;
    }
    required(s.dispose,"Required original biped controller disposal owner unavailable",component);
    controller_class=10;
  }else controller_class=r(m,unit+0x34b,1)==2?11:10;
  required(s.controller,"Required original biped physical controller construction owner unavailable",
    SourceBipedControllerRequest{component,definition+0x264,unit+0x3dc,9,controller_class,constrained,dead});
}
void source_rebuild_object_physics_1_1(M&m,std::uint32_t datum,std::uint32_t flags,
    const SourceObjectPhysicsRebuildServices&s,const O&o){
  const auto component_datum=r(m,object(m,datum)+0xb4);if(component_datum==0xffffffffu)return;
  const auto component=r(m,r(m,0x526248)+0x44)+(component_datum&0xffffu)*160u;
  const bool member=r(m,component+4)&0x20u;
  if(member)required(s.remove,"Required original component world-removal owner unavailable",component);
  required(s.dispose,"Required original component disposal owner unavailable",component);
  w(m,o,0x1c7129,component+4,4,flags);
  required(s.rebuild,"Required original type-specific component rebuild owner unavailable",r(m,object(m,datum)+0xb4));
  if(member)required(s.insert,"Required original component world-insertion owner unavailable",component);
}
} // namespace halo2
