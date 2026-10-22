#include "halo2/physics_node_delivery.hpp"
#include "halo2/transform.hpp"
#include <bit>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace halo2 {
namespace {
using M=SourcePhysicsContactMemory;using O=SourcePhysicsContactRecordObserver;
std::uint32_t r(const M&m,std::uint32_t p,unsigned n=4){
  std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v=0;
  for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;
}
std::int32_t si(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
std::int32_t short_at(const M&m,std::uint32_t p){return std::bit_cast<std::int16_t>(std::uint16_t(r(m,p,2)));}
std::int32_t byte_at(const M&m,std::uint32_t p){return std::bit_cast<std::int8_t>(std::uint8_t(r(m,p,1)));}
float f(const M&m,std::uint32_t p){return std::bit_cast<float>(r(m,p));}
float add(float a,float b){volatile float v=a+b;return v;}
float sub(float a,float b){volatile float v=a-b;return v;}
float mul(float a,float b){volatile float v=a*b;return v;}
float div(float a,float b){volatile float v=a/b;return v;}
void w(M&m,const O&o,std::uint32_t site,std::uint32_t p,std::uint32_t v){
  SourcePhysicsContactRecordStore s{site,p,{},{}};s.before.resize(4);s.written.resize(4);m.read(p,s.before);
  for(unsigned i=0;i<4;++i)s.written[i]=std::byte(v>>(8*i));m.write(p,s.written);if(o)o(s);
}
void scalar(M&m,const O&o,std::uint32_t site,std::uint32_t p,float v){w(m,o,site,p,std::bit_cast<std::uint32_t>(v));}
std::uint32_t object(const M&m,std::uint32_t d){return r(m,r(m,r(m,0x4e78d0)+0x44)+(d&65535u)*12u+8);}
std::uint32_t tag(const M&m,std::uint32_t d){return r(m,r(m,0x4eb114)+(d&65535u)*16u+8);}
Transform matrix(const M&m,std::uint32_t p){Transform t;m.read(p,std::as_writable_bytes(std::span{&t,1}));return t;}
void matrix(M&m,const O&o,std::uint32_t site,std::uint32_t p,const Transform&t){
  const auto a=std::bit_cast<std::array<std::uint32_t,13>>(t);
  for(unsigned i=0;i<a.size();++i)w(m,o,site,p+i*4,a[i]);
}
bool marked(const M&m,std::uint32_t p,std::uint32_t node){return r(m,p+(node>>5)*4)&(1u<<(node&31));}
void mark(M&m,const O&o,std::uint32_t site,std::uint32_t p,std::uint32_t node){
  const auto at=p+(node>>5)*4;w(m,o,site,at,r(m,at)|(1u<<(node&31)));
}
std::array<Vec3,3> quaternion_basis(const M&m,std::uint32_t p){
  //143580 is a distinct SSE owner from142260's RCP-refined animation locals.
  const auto x=f(m,p),y=f(m,p+4),z=f(m,p+8),q=f(m,p+12);
  const auto norm=add(add(add(mul(x,x),mul(y,y)),mul(z,z)),mul(q,q));
  const auto scale=norm>f(m,0x46504c)?div(f(m,0x46507c),norm):0.f;
  const auto sx=mul(x,scale),sy=mul(y,scale),sz=mul(z,scale);
  const auto zw=mul(sz,q),xw=mul(sx,q),xx=mul(x,sx),xy=mul(x,sy),xz=mul(x,sz);
  const auto yy=mul(y,sy),yw=mul(sy,q),yz=mul(y,sz),zz=mul(sz,z),one=f(m,0x465030);
  return {{{sub(one,add(zz,yy)),add(xy,zw),sub(xz,yw)},
      {sub(xy,zw),sub(one,add(zz,xx)),add(yz,xw)},
      {add(xz,yw),sub(yz,xw),sub(one,add(yy,xx))}}};
}
std::array<float,4> matrix_quaternion(const M&m,const Transform&t){
  //1436D0 uses x87 for sqrt/half and SSE for stored differences/products.
  const auto a=std::bit_cast<std::array<float,9>>(t.basis);
  const auto trace=add(add(a[4],a[0]),a[8]);const auto one=f(m,0x465030),half=f(m,0x46502c),quarter=f(m,0x465090);
  std::array<float,4>q{};
  if(trace>0.f){
    const auto s=float(std::sqrt(double(trace)+double(one))*double(half));const auto inv=div(quarter,s);
    q={mul(sub(a[5],a[7]),inv),mul(sub(a[6],a[2]),inv),mul(sub(a[1],a[3]),inv),s};
  }else{
    unsigned i=a[4]>a[0]?1:0;if(a[8]>a[i*4])i=2;const auto j=(i+1)%3,k=(i+2)%3;
    const auto s=float(std::sqrt(((double(a[i*4])-double(a[j*4]))-double(a[k*4]))+double(one))*double(half));
    const auto inv=div(quarter,s);q[i]=s;q[3]=mul(sub(a[j*3+k],a[k*3+j]),inv);
    q[j]=mul(add(a[i*3+j],a[j*3+i]),inv);q[k]=mul(add(a[i*3+k],a[k*3+i]),inv);
  }
  if(q[3]<0.f)for(auto&v:q)v=sub(0.f,v);return q;
}
void hierarchy(M&m,std::uint32_t component,std::uint32_t node,std::uint32_t mask,
    std::uint32_t ancestor,std::uint32_t locals,const O&o,unsigned depth){
  if(depth>=256)throw std::runtime_error("Invalid original physical-node hierarchy cycle");
  const auto ob=object(m,r(m,component+8)),model=tag(m,r(m,tag(m,r(m,ob))+0x38));
  const auto phmo=tag(m,r(m,model+0x24)),render=tag(m,r(m,model+4));
  const auto row=r(m,phmo+0xcc)+node*12u;const auto parent=short_at(m,row+6);
  const auto nodes=ob+short_at(m,ob+0x116),out=nodes+node*0x34u;
  if(parent!=-1){
    if(marked(m,mask,node)){
      if(locals){const auto relative=compose_transform(inverse_transform(matrix(m,nodes+std::uint32_t(parent)*0x34u)),matrix(m,out));
        const auto q=matrix_quaternion(m,relative);const auto local=locals+node*32u;
        for(unsigned i=0;i<4;++i)scalar(m,o,0x1436d0,local+i*4,q[i]);
        scalar(m,o,0x143ae0,local+16,relative.position.x);scalar(m,o,0x143ae6,local+20,relative.position.y);
        scalar(m,o,0x143aec,local+24,relative.position.z);scalar(m,o,0x143af2,local+28,relative.scale);
      }
    }else if(marked(m,ancestor,node)){
      const auto bind=r(m,render+0x4c)+node*0x60u;
      Transform local{f(m,0x465030),quaternion_basis(m,bind+0x18),{f(m,bind+0xc),f(m,bind+0x10),f(m,bind+0x14)}};
      matrix(m,o,0x1441d0,out,compose_transform(matrix(m,nodes+std::uint32_t(parent)*0x34u),local));
      mark(m,o,0x1d78fb,mask,node);
      if(locals){const auto at=locals+node*32u;
        for(unsigned i=0;i<4;++i)w(m,o,0x1d791e,at+i*4,r(m,bind+0x18+i*4));
        for(unsigned i=0;i<3;++i)w(m,o,0x1d7937,at+16+i*4,r(m,bind+0xc+i*4));
        w(m,o,0x1d7945,at+28,r(m,0x465030));}
    }
  }
  auto child=short_at(m,row+10);unsigned visited=0;
  while(child!=-1){if(++visited>256)throw std::runtime_error("Invalid original physical-node sibling cycle");
    hierarchy(m,component,std::uint32_t(child),mask,ancestor,locals,o,depth+1);
    child=short_at(m,r(m,phmo+0xcc)+std::uint32_t(child)*12u+8);}
}
}
void source_physics_deliver_nodes_1_1(M&m,std::uint32_t component,std::uint32_t mask,float height,
    const PhysicsNodeDeliveryScratch&s,const PhysicsNodeDeliveryServices&services,const O&o){
  if((r(m,component+4)&0x200u)||si(r(m,component+0x74))<=0)return;
  const auto ob=object(m,r(m,component+8)),model=tag(m,r(m,tag(m,r(m,ob))+0x38));
  const auto phmo=tag(m,r(m,model+0x24)),render=tag(m,r(m,model+4));
  const auto nodes=ob+short_at(m,ob+0x116);const auto local_offset=short_at(m,ob+0x112);
  const auto locals=local_offset==-1?0u:ob+std::uint32_t(local_offset);
  for(unsigned i=0;i<8;++i)w(m,o,0x1d73a1+i*4,s.ancestor_mask32+i*4,0);
  bool delivered=false;
  for(std::uint32_t slot_index=0;si(slot_index)<si(r(m,component+0x74));++slot_index){
    const auto slot=r(m,component+0x70)+slot_index*0x60u;
    const auto indices=r(m,slot+0x48),rows=r(m,phmo+0x3c);
    const auto first=short_at(m,rows+std::uint32_t(byte_at(m,indices))*0x90u);
    for(std::uint32_t i=0;si(i)<si(r(m,slot+0x4c));++i){
      const auto row=rows+std::uint32_t(byte_at(m,indices+i))*0x90u;const auto node=short_at(m,row);
      if(node==-1)continue;
      const auto motion=r(m,r(m,slot+0x40)+0x3c),method=r(m,r(m,motion)+0x18);
      //9D900 is the complete literal MOV EAX,4;RET leaf in both source traces.
      std::uint32_t type;
      if(method==0x9d900)type=4;
      else{if(!services.motion_type)throw std::runtime_error("Required original motion virtual type owner unavailable");
        type=services.motion_type(motion,method);}
      if(type==6||marked(m,mask,std::uint32_t(node)))continue;
      auto chain=node;unsigned depth=0;bool skip=false;
      while(chain!=-1){if(++depth>256)throw std::runtime_error("Invalid original physical-node parent cycle");
        if(chain==first){skip=node!=first;break;}chain=short_at(m,r(m,phmo+0xcc)+std::uint32_t(chain)*12u+6);}
      if(skip)continue;
      const auto out=nodes+std::uint32_t(node)*0x34u;
      source_physics_body_matrix_1_1(m,component,slot_index,out,s.body_transform40,services.correction,o);
      scalar(m,o,0x1d7532,out+0x30,add(f(m,out+0x30),height));
      if(node!=first){if(!services.child_transform)throw std::runtime_error("Required original physical shape child frame owner unavailable");
        const auto frame=services.child_transform(r(m,r(m,slot+0x40)+0xc),i);
        matrix(m,o,0x1441d0,out,compose_transform(matrix(m,out),std::bit_cast<Transform>(frame)));}
      mark(m,o,0x1d7634,mask,std::uint32_t(node));mark(m,o,0x1d7655,s.ancestor_mask32,std::uint32_t(node));
      chain=short_at(m,r(m,phmo+0xcc)+std::uint32_t(node)*12u+6);depth=0;
      while(chain!=-1){if(++depth>256)throw std::runtime_error("Invalid original physical-node ancestor cycle");
        mark(m,o,0x1d7684,s.ancestor_mask32,std::uint32_t(chain));chain=short_at(m,r(m,phmo+0xcc)+std::uint32_t(chain)*12u+6);}
      delivered=true;
    }
  }
  if(delivered&&si(r(m,render+0x48))>0)hierarchy(m,component,0,mask,s.ancestor_mask32,locals,o,0);
}
} // namespace halo2
