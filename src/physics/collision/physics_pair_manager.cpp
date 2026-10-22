#include "halo2/physics_pair_manager.hpp"
#include "halo2/physics_body_disposal.hpp"
#include "halo2/physics_constraint_membership.hpp"
#include <bit>
#include <stdexcept>
#include <string>

namespace halo2 {
namespace {
using M=SourcePhysicsContactMemory;using O=SourcePhysicsContactRecordObserver;
using S=SourcePhysicsPairManagerServices;
std::uint32_t r(const M&m,std::uint32_t p,unsigned n=4) {
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v=0;
    for(unsigned k=0;k<n;++k)v|=std::to_integer<std::uint32_t>(b[k])<<(k*8);
    return v;
}
void w(M&m,const O&o,std::uint32_t site,std::uint32_t p,unsigned n,std::uint64_t v) {
    SourcePhysicsContactRecordStore x{site,p,{},{}};x.before.resize(n);x.written.resize(n);m.read(p,x.before);
    for(unsigned k=0;k<n;++k)x.written[k]=std::byte((v>>(k*8))&255u);
    m.write(p,x.written);if(o)o(x);
}
std::int32_t sign(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
std::uint32_t sbyte(std::uint32_t v){return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(v))));}
[[noreturn]]void missing(const char*s){throw std::runtime_error(s);}
std::uint32_t required(M&m,const S&s,std::uint32_t entry,std::uint32_t receiver,
    std::vector<std::uint32_t>args,bool cdecl,const O&o,std::uint32_t entry_esp=0) {
    if(!s.required_owner)throw std::runtime_error("Required original pair manager lower owner unavailable at "+std::to_string(entry));
    return s.required_owner(m,{entry,receiver,std::move(args),cdecl,entry_esp},o);
}
std::uint32_t allocate(M&m,std::uint32_t bytes,const O&o) {
    const auto adapter=r(m,0x487638),backend=r(m,0x480da0);
    if(r(m,r(m,adapter)+0x10)!=0x148a10u||r(m,0x480d98)!=2u||r(m,r(m,backend)+0x10)!=0x22e950u)
        missing("Required original pair allocator owner unavailable");
    const auto p=source_allocate_physics_body_small_block_1_1(m,backend,bytes,o);
    if(!p)missing("Required original pair alternate allocator owner unavailable");
    return p;
}
std::uint32_t type(M&m,const S&s,std::uint32_t p,unsigned slot,const O&o) {
    const auto target=r(m,r(m,p)+slot);
    if(slot==0x14&&target==0xa0120)return 23;
    if(slot==0x14&&target==0xa47b0)return 20;
    if(slot==0x14&&target==0x2e4d20)return 21;
    if(slot==0x18&&target==0x9d900)return 4;
    if(slot==0x18&&target==0x9b560)return 5;
    return required(m,s,target,p,{},false,o);
}
std::uint32_t component(const M&m,std::uint32_t body) {
    const auto n=r(m,body+0x34),bank=r(m,body+0x30);
    for(std::uint32_t k=0;sign(k)<sign(n);++k)if(r(m,bank+k*8u)==0x2001)return r(m,bank+k*8u+4);
    return 0;
}
void append_listener(M&m,std::uint32_t body,std::uint32_t constraint,const O&o) {
    const auto d=body+0x88;
    if(r(m,d+4)==(r(m,d+8)&0x7fffffffu))source_double_physics_body_bank_1_1(m,d,4,o);
    w(m,o,0x31234d,r(m,d)+r(m,d+4)*4u,4,constraint);w(m,o,0x312350,d+4,4,r(m,d+4)+1u);
}
void remove_listener(M&m,std::uint32_t body,std::uint32_t constraint,const S&s,const O&o) {
    const auto n=r(m,body+0x8c),bank=r(m,body+0x88);auto k=0u;
    while(sign(k)<sign(n)&&r(m,bank+k*4u)!=constraint)++k;
    if(sign(k)>=sign(n))k=0xffffffffu;
    // Preserve source's literal missing-index write; backing remains required.
    w(m,o,0x31224e,bank+k*4u,4,0);
    if(const auto world=r(m,body+8))required(m,s,0x313240,world,{body},false,o);
}
void callback(M&m,std::uint32_t target,std::uint32_t receiver,std::uint32_t pair,
    std::uint32_t entry_esp,const S&s,const O&o) {
    if(target==0x31c430||target==0x31c4b0)source_notify_physics_collidable_pair_1_1(m,pair,target==0x31c4b0,s,o);
    else if(target==0x3140f0)source_remove_physics_static_pair_1_1(m,receiver,pair,o);
    else if(target==0x314020)source_append_physics_static_pair_1_1(m,receiver,pair,o);
    else if(target==0x31d360)source_append_physics_island_pair_1_1(m,receiver,pair,{entry_esp},s,o);
    else required(m,s,target,receiver,{pair},false,o,entry_esp);
}
void add(M&m,std::uint32_t manager,std::uint32_t pair,std::uint32_t filter,
    std::uint32_t scratch,std::uint32_t callback_esp,const S&s,const O&o) {
    const auto ah=r(m,pair),bh=r(m,pair+4),a=ah?ah-0x10u:0u,b=bh?bh-0x10u:0u;
    const auto target=r(m,r(m,filter));std::uint32_t output;
    if(target==0x2e5f50)output=source_filter_physics_pair_1_1(m,filter,scratch,a,b,o);
    else output=required(m,s,target,filter,{scratch,a,b},false,o);
    if(r(m,output,1)) {
        const auto ix=r(m,ah+8)*8u+r(m,bh+8),receiver=r(m,manager+ix*4u);
        callback(m,r(m,r(m,receiver)+4),receiver,pair,callback_esp,s,o);
    }
}
void remove(M&m,std::uint32_t manager,std::uint32_t pair,std::uint32_t esp,const S&s,const O&o) {
    const auto ix=r(m,r(m,pair)+8)*8u+r(m,r(m,pair+4)+8),receiver=r(m,manager+ix*4u);
    callback(m,r(m,r(m,receiver)+8),receiver,pair,esp,s,o);
}
void quick_order(M&m,std::uint32_t pairs,std::uint32_t low,
    std::uint32_t high,const O&o) {
    for(;;) {
        auto left=low,right=high;
        const auto middle=std::uint32_t(sign(low+high)>>1),
                   pivot_a=r(m,pairs+middle*8u),pivot_b=r(m,pairs+middle*8u+4u);
        for(;;) {
            const auto key=r(m,pivot_a+4);
            while(r(m,r(m,pairs+left*8u)+4)<key ||
                (r(m,r(m,pairs+left*8u)+4)==key &&
                 r(m,r(m,pairs+left*8u+4u)+4)<r(m,pivot_b+4)))++left;
            while(key<r(m,r(m,pairs+right*8u)+4) ||
                (key==r(m,r(m,pairs+right*8u)+4) &&
                 r(m,pivot_b+4)<r(m,r(m,pairs+right*8u+4u)+4)))--right;
            if(sign(right)<sign(left))break;
            if(right!=left) {
                const auto a=r(m,pairs+right*8u),b=r(m,pairs+right*8u+4u),
                           c=r(m,pairs+left*8u);
                w(m,o,0x3790fb,pairs+right*8u,4,c);
                w(m,o,0x379102,pairs+right*8u+4u,4,r(m,pairs+left*8u+4u));
                w(m,o,0x379106,pairs+left*8u,4,a);
                w(m,o,0x379109,pairs+left*8u+4u,4,b);
            }
            --right;++left;
            if(sign(left)>sign(right))break;
        }
        if(sign(low)<sign(right))quick_order(m,pairs,low,right,o);
        if(sign(high)<=sign(left))return;
        low=left;
    }
}
}
std::uint32_t source_filter_physics_pair_1_1(M&m,std::uint32_t filter,std::uint32_t output,
    std::uint32_t a,std::uint32_t b,const O&o) {
    const auto right=r(m,b+0x1c),left=r(m,a+0x1c);
    if(((left^right)&0xffff0000u)==0&&(left&0xffff0000u)!=0)w(m,o,0x2e5f78,output,1,0);
    else w(m,o,0x2e5f99,output,1,(r(m,filter+(left&31u)*4u+0x14)&(1u<<(right&31u)))!=0);
    return output;
}
void source_append_physics_static_pair_1_1(M&m,std::uint32_t list,std::uint32_t pair,const O&o) {
    const auto owner=r(m,list+4),mine=owner?owner+0x10u:0u;
    auto handle=r(m,pair);if(handle==mine)handle=r(m,pair+4);
    const auto collidable=handle?handle-0x10u:0u,body=r(m,collidable+0x20);
    if(r(m,body+0x40,1))return;
    const auto island=r(m,body+0x44),pairs=island+0x80;
    if(r(m,pairs+4)==(r(m,pairs+8)&0x7fffffffu))source_double_physics_body_bank_1_1(m,pairs,8,o);
    const auto dst=r(m,pairs)+r(m,pairs+4)*8u;
    w(m,o,0x31408a,dst,4,r(m,pair));w(m,o,0x314090,dst+4,4,r(m,pair+4));w(m,o,0x314094,pairs+4,4,r(m,pairs+4)+1u);
    const auto n=r(m,list+0xc);for(std::uint32_t k=0;sign(k)<sign(n);++k)if(r(m,r(m,list+8)+k*4u)==island)return;
    const auto d=list+8;if(r(m,d+4)==(r(m,d+8)&0x7fffffffu))source_double_physics_body_bank_1_1(m,d,4,o);
    w(m,o,0x3140d8,r(m,d)+r(m,d+4)*4u,4,island);w(m,o,0x3140db,d+4,4,r(m,d+4)+1u);
}
void source_remove_physics_static_pair_1_1(M&m,std::uint32_t list,std::uint32_t pair,const O&o) {
    const auto owner=r(m,list+4),mine=owner?owner+16u:0u;
    auto handle=r(m,pair);if(handle==mine)handle=r(m,pair+4);
    const auto collidable=handle?handle-16u:0u,body=r(m,collidable+0x20);
    if(r(m,body+0x40,1))return;
    const auto island=r(m,body+0x44),removed=island+0x8c;
    if(r(m,removed+4)==(r(m,removed+8)&0x7fffffffu))source_double_physics_body_bank_1_1(m,removed,8,o);
    const auto dst=r(m,removed)+r(m,removed+4)*8u;
    w(m,o,0x3141b0,dst,4,r(m,pair));w(m,o,0x3141b6,dst+4,4,r(m,pair+4));w(m,o,0x3141bf,removed+4,4,r(m,removed+4)+1u);
    const auto d=list+8,n=r(m,d+4);
    for(std::uint32_t k=0;sign(k)<sign(n);++k)if(r(m,r(m,d)+k*4u)==island)return;
    if(r(m,d+4)==(r(m,d+8)&0x7fffffffu))source_double_physics_body_bank_1_1(m,d,4,o);
    w(m,o,0x314172,r(m,d)+r(m,d+4)*4u,4,island);w(m,o,0x314175,d+4,4,r(m,d+4)+1u);
}
void source_deliver_physics_pairs_1_1(M&m,std::uint32_t manager,
    const std::array<std::uint32_t,5>&args,const SourcePhysicsPairManagerScratch&scratch,
    const S&s,const O&o) {
    auto a=args[0],b=args[2];const auto ae=a+args[1]*8u,be=b+args[3]*8u;
    while(a!=ae&&b!=be) {
        const auto af=r(m,r(m,a)+4),bf=r(m,r(m,b)+4);
        if(af==bf&&r(m,r(m,b+4)+4)==r(m,r(m,a+4)+4)){a+=8;b+=8;continue;}
        const bool older=bf<af||(bf==af&&r(m,r(m,b+4)+4)<r(m,r(m,a+4)+4));
        if(older){remove(m,manager,b,scratch.entry_esp-28u,s,o);b+=8;}
        else{add(m,manager,a,args[4],scratch.entry_esp+4u,scratch.entry_esp-28u,s,o);a+=8;}
    }
    while(a<ae){add(m,manager,a,args[4],scratch.entry_esp+20u,scratch.entry_esp-28u,s,o);a+=8;}
    while(b<be){remove(m,manager,b,scratch.entry_esp-28u,s,o);b+=8;}
}
void source_notify_physics_collidable_pair_1_1(M&m,std::uint32_t pair,bool removed,const S&s,const O&o) {
    // The virtual body owns the collidable bank; handle kind2 is the source
    // branch, not a native decision that collision/solver contact succeeded.
    for(unsigned side=0;side<2;++side) {
        const auto handle=r(m,pair+side*4u);
        if(r(m,handle+8)!=2)continue;
        const auto other=r(m,pair+(1u-side)*4u),collidable=handle?handle-16u:0u;
        const auto b=other?other-16u:0u,entity=r(m,collidable+0x20);
        const auto target=r(m,r(m,entity)+(removed?0x20u:0x1cu));
        if(removed&&target==0x311f80)source_remove_physics_entity_collidable_1_1(m,entity,b,s,o);
        else if(!removed&&target==0x312010)source_add_physics_entity_collidable_1_1(m,entity,b,entity&0xffffff00u,s,o);
        else required(m,s,target,entity,removed?std::vector{b}:std::vector{b,entity&0xffffff00u},false,o);
    }
}
void source_add_physics_entity_collidable_1_1(M&m,std::uint32_t entity,std::uint32_t collidable,
    std::uint32_t check,const S&s,const O&o) {
    if(r(m,collidable)==0)return;
    bool found=false;
    if(std::uint8_t(check))for(std::uint32_t k=0;sign(k)<sign(r(m,entity+0xc4));++k)
        if(r(m,r(m,entity+0xc0)+k*4u)==collidable){found=true;break;}
    const auto admission=sign(r(m,entity+0x40))>0?required(m,s,0x311490,entity,{collidable,(collidable&0xffffff00u)|(found?1u:0u)},false,o):(found?1u:0u);
    if(admission||found)return;
    const auto d=entity+0xc0;
    if(r(m,d+4)==(r(m,d+8)&0x7fffffffu))source_double_physics_body_bank_1_1(m,d,4,o);
    const auto count=r(m,d+4),dst=r(m,d)+count*4u;
    w(m,o,0x31209b,d+4,4,count+1u);w(m,o,0x31209e,dst,4,collidable);
}
void source_remove_physics_entity_collidable_1_1(M&m,std::uint32_t entity,std::uint32_t collidable,
    const S&s,const O&o) {
    if(r(m,collidable)==0)return;
    auto k=r(m,entity+0xc4)-1u;bool found=false;
    while(sign(k)>=0){if(r(m,r(m,entity+0xc0)+k*4u)==collidable){found=true;break;}--k;}
    if(!found) {
        if(sign(r(m,entity+0x40))>0)required(m,s,0x311f80,entity,{collidable},false,o);
        return;
    }
    if(sign(r(m,entity+0x40))>0)required(m,s,0x3114f0,entity,{collidable,(collidable&0xffffff00u)|1u},false,o);
    const auto count=r(m,entity+0xc4)-1u;
    w(m,o,0x311ff5,entity+0xc4,4,count);
    w(m,o,0x312004,r(m,entity+0xc0)+k*4u,4,r(m,r(m,entity+0xc0)+count*4u));
}
std::uint32_t source_construct_physics_pair_constraint_1_1(M&m,std::uint32_t p,const O&o) {
    w(m,o,0x322592,p+6,2,1);w(m,o,0x322598,p+8,4,0x41b980);
    w(m,o,0x32259f,p,4,0x41ba08);w(m,o,0x3225a5,p+8,4,0x41b9f8);
    for(const auto [site,offset,value]:std::array<std::array<std::uint32_t,3>,28>{{
        {0x3225ae,0x14,0},{0x3225b1,0x10,0},{0x3225b4,0xc,0},{0x3225b7,0x18,0},
        {0x3225ba,0x24,0},{0x3225bd,0x28,0x80000008},{0x3225c7,0x20,p+0x2c},
        {0x3225ca,0x34,0},{0x3225cd,0x38,0},{0x3225d0,0x3c,0x80000000},
        {0x3225d7,0x50,0},{0x3225da,0x54,0},{0x3225dd,0x58,0},{0x3225e0,0x5c,0},
        {0x3225e3,0x60,0},{0x3225e6,0x64,0},{0x3225e9,0x68,0},{0x3225ec,0x6c,0},
        {0x3225ef,0x70,0x3e88d677},{0x3225f6,0x74,0x3f08d677},
        {0x322602,0x78,0x3f4d41b3},{0x322605,0x7c,0x3f4d41b3},
        {0x322608,0x94,0},{0x32260e,0x98,0x80000004},{0x32261e,0x90,p+0x9c},
        {0x322624,0x9c,0},{0x322626,0xa0,0},{0x32262c,0xb8,0}}})w(m,o,site,p+offset,4,value);
    w(m,o,0x32262e,p+0xbc,4,0);w(m,o,0x322634,p+0xd4,4,0);w(m,o,0x322636,p+0xd8,4,0);
    w(m,o,0x32263c,p+0xf0,4,0);w(m,o,0x32263e,p+0xf4,4,0);return p;
}
void source_bind_physics_pair_constraint_body_1_1(M&m,std::uint32_t p,std::uint32_t body,
    bool second,const S&s,const O&o) {
    const auto offset=second?0x10u:0xcu;
    if(body){w(m,o,second?0x3220dc:0x32206c,body+6,2,r(m,body+6,2)+1u);append_listener(m,body,p,o);}
    if(const auto old=r(m,p+offset)) {
        remove_listener(m,old,p,s,o);w(m,o,second?0x3220f9:0x322089,old+6,2,r(m,old+6,2)-1u);
        if(r(m,old+6,2)==0) {
            const auto resident=source_physics_body_reference_is_resident_1_1(m,old);
            if(std::uint8_t(resident)!=0) {
                w(m,o,second?0x322111:0x3220a1,old+6,2,1);
                w(m,o,second?0x322118:0x3220a8,p+offset,4,body);return;
            }
            required(m,s,r(m,r(m,old)),old,{1},false,o);
        }
    }
    w(m,o,second?0x322129:0x3220b9,p+offset,4,body);
}
std::uint32_t source_construct_physics_pair_contact_manager_1_1(M&m,std::uint32_t p,
    const std::array<std::uint32_t,3>&a,const S&s,const O&o) {
    w(m,o,0x31f6eb,p,4,0x41b6b4);source_construct_physics_pair_constraint_1_1(m,p+0x10,o);
    w(m,o,0x31f706,p+8,4,a[0]);source_bind_physics_pair_constraint_body_1_1(m,p+0x10,a[1],false,s,o);
    source_bind_physics_pair_constraint_body_1_1(m,p+0x10,a[2],true,s,o);
    w(m,o,0x31f71e,p+0xc,4,1);return p;
}
std::uint32_t source_allocate_physics_pair_contact_manager_1_1(M&m,std::uint32_t factory,
    const std::array<std::uint32_t,3>&a,const S&s,const O&o) {
    const auto b=r(m,a[1]+0x20),body=r(m,a[0]+0x20),p=allocate(m,0x120,o);
    w(m,o,0x31f7aa,p+4,2,0x120);return source_construct_physics_pair_contact_manager_1_1(m,p,{r(m,factory+8),body,b},s,o);
}
std::uint32_t source_construct_physics_pair_agent_base_1_1(M&m,std::uint32_t p,
    std::uint32_t manager,const O&o) {
    w(m,o,0x2efb6e,p+8,4,manager);w(m,o,0x2efb71,p,4,0x4190d8);
    w(m,o,0x2efb87,p+0xc,4,0);w(m,o,0x2efb8a,p+0x10,4,0);
    w(m,o,0x2efb8d,p+0x14,4,0x80000000);
    for(unsigned n=0;n<2;++n)w(m,o,0x2efb98,p+0x30+n*8,8,0x7f7fffff7f7fffffull);
    for(unsigned n=0;n<2;++n)w(m,o,0x2efb9c,p+0x20+n*8,8,0x7f7fffff7f7fffffull);
    return p;
}
std::uint32_t source_allocate_physics_pair_agent_1_1(M&m,
    const std::array<std::uint32_t,4>&a,const O&o) {
    const auto p=allocate(m,0x40,o);w(m,o,0x300198,p+4,2,0x40);
    source_construct_physics_pair_agent_base_1_1(m,p,a[3],o);w(m,o,0x3001ba,p,4,0x41959c);return p;
}
std::uint32_t source_validate_physics_pair_1_1(M&m,std::uint32_t a,std::uint32_t b,const S&s,const O&o) {
    const auto aa=r(m,a+0x18)==1?r(m,a+0x20):0u,bb=r(m,b+0x18)==1?r(m,b+0x20):0u;
    if(!aa||!bb)return 0;
    if(r(m,aa+0x40,1)||r(m,bb+0x40,1))return 1;
    if(type(m,s,r(m,aa+0x3c),0x18,o)==6||type(m,s,r(m,bb+0x3c),0x18,o)==6)return 1;
    const auto ca=component(m,aa);if(ca==0xffffffffu)return 0;
    const auto cb=component(m,bb);if(cb==0xffffffffu)return 0;
    const auto physical=r(m,r(m,0x526248)+0x44),objects=r(m,r(m,0x4e78d0)+0x44);
    const auto datum=r(m,physical+(ca&0xffffu)*0xa0u+8),header=objects+(datum&0xffffu)*12u;
    // Literal27E52A..27E57F retains its repeated first-header reads. No
    // inferred second-object comparison is inserted into this source owner.
    const auto first=r(m,header+3,1);
    auto flag=first==1?0u:1u;
    if(first==1||r(m,header+3,1)==1)if(first!=r(m,header+3,1))return 1;
    return flag?1u:0u;
}
std::uint32_t source_write_physics_pair_validation_1_1(M&m,std::uint32_t validator,
    std::uint32_t output,std::uint32_t a,std::uint32_t b,const S&s,const O&o) {
    const auto filter=r(m,validator+8),target=r(m,r(m,filter)+0xc);
    const auto result=target==0x27e4a0?source_validate_physics_pair_1_1(m,a,b,s,o):
        required(m,s,target,filter,{a,b},false,o);
    w(m,o,0x27daa6,output,1,result);return output;
}
std::uint32_t source_construct_physics_island_pair_bank_1_1(M&m,std::uint32_t d,
    std::uint32_t count,const S&s,const O&o) {
    w(m,o,0x313f10,d,4,0);w(m,o,0x313f12,d+4,4,0);w(m,o,0x313f15,d+8,4,0x80000000);
    const auto adapter=r(m,0x487638),bytes=(count*16u+15u)&~15u,available=r(m,adapter+0xc);
    std::uint32_t p;
    if(sign(bytes)>sign(available)) {
        if(!s.scratch.scratch_allocate)missing("Required original island scratch allocator virtual+24 owner unavailable");
        p=s.scratch.scratch_allocate(m,adapter,r(m,r(m,adapter)+0x24),bytes,o);
    }else {
        p=r(m,adapter+8);w(m,o,0x313f51,adapter+8,4,p+bytes);
        w(m,o,0x313f54,adapter+0xc,4,available-bytes);
    }
    w(m,o,0x313f5a,d,4,p);w(m,o,0x313f62,d+0xc,4,p);
    w(m,o,0x313f65,d+8,4,count|0x80000000u);w(m,o,0x313f6c,d+0x10,4,count);return d;
}
void source_destroy_physics_island_pair_bank_1_1(M&m,std::uint32_t d,const S&s,const O&o) {
    const auto adapter=r(m,0x487638),p=r(m,d+0xc),bytes=(r(m,d+0x10)*16u+15u)&~15u;
    const auto alternate=r(m,adapter+0x14);
    if(bytes!=0&&(p==alternate||alternate==0)) {
        if(!s.scratch.scratch_free)missing("Required original island scratch allocator virtual+28 owner unavailable");
        s.scratch.scratch_free(m,adapter,r(m,r(m,adapter)+0x28),p,bytes,o);
    }else {
        w(m,o,0x313fdb,adapter+8,4,r(m,adapter+8)-bytes);
        w(m,o,0x313fde,adapter+0xc,4,r(m,adapter+0xc)+bytes);
    }
    const auto capacity=r(m,d+8);
    if(sign(capacity)>=0) {
        const auto backend=r(m,0x480da0),block=r(m,d),n=(capacity&0x7fffffffu)*16u;
        if(r(m,r(m,adapter)+0x14)!=0x148a70u||
           (block&&(sign(block)<sign(backend+0x4340u)||sign(block)>sign(backend+0xf0340u)||
            r(m,r(m,backend)+0x14)!=0x22eb20u)))
            missing("Required original island grown-bank free owner unavailable");
        if(block)source_free_physics_body_small_block_1_1(m,backend,block,n,o);
    }
}
void source_order_physics_pairs_1_1(M&m,std::uint32_t pairs,std::uint32_t count,const S&s,const O&o) {
    (void)s;
    if(count==0)return;
    for(std::uint32_t k=0;sign(k)<sign(count);++k) {
        const auto p=pairs+k*8u,a=r(m,p),b=r(m,p+4);
        if(r(m,a+4)>r(m,b+4)){w(m,o,0x379164,p,4,b);w(m,o,0x379167,p+4,4,a);}
    }
    if(sign(count)>1)quick_order(m,pairs,0,count-1u,o);
}
void source_update_physics_island_pairs_1_1(M&m,std::uint32_t island,
    const std::array<std::uint32_t,4>&args,std::uint32_t frame,const S&s,const O&o) {
    if(args[1]==0&&args[3]==0)return;
    source_order_physics_pairs_1_1(m,args[0],args[1],s,o);
    source_order_physics_pairs_1_1(m,args[2],args[3],s,o);
    const auto descriptor=frame+12u;
    source_construct_physics_island_pair_bank_1_1(m,descriptor,r(m,island+0x6c)+args[1],s,o);
    w(m,o,0x31d9e1,island+0x9c,4,r(m,island+0x68));
    w(m,o,0x31d9f2,island+0xa0,4,r(m,island+0x68)+(r(m,island+0x6c)<<4));
    w(m,o,0x31d9f8,island+0xa4,4,r(m,descriptor));
    auto world=r(m,island+0x20);w(m,o,0x31da04,island+0x98,4,r(m,world+0xcc));
    auto manager=r(m,world+0x138);w(m,o,0x31da13,manager+0x24,4,island+0x1c);
    auto filter=r(m,world+0xd0);if(filter)filter+=8;
    source_deliver_physics_pairs_1_1(m,manager,{args[0],args[1],args[2],args[3],filter},{frame-28u},s,o);
    world=r(m,island+0x20);manager=r(m,world+0x138);
    w(m,o,0x31da57,manager+0x24,4,manager+0x100);
    while(r(m,island+0x9c)<r(m,island+0xa0)) {
        const auto old=r(m,island+0x9c),dst=r(m,island+0xa4);
        for(unsigned k=0;k<4;++k)w(m,o,std::array{0x31da7eu,0x31da83u,0x31da89u,0x31da8fu}[k],dst+k*4u,4,r(m,old+k*4u));
        w(m,o,0x31daa2,island+0xa4,4,dst+16u);w(m,o,0x31daa8,island+0x9c,4,old+16u);
    }
    const auto count=std::uint32_t(sign(r(m,island+0xa4)-r(m,descriptor))>>4),capacity=r(m,island+0x70);
    if(sign(capacity&0x7fffffffu)<sign(count)) {
        if(sign(capacity)>=0) {
            const auto adapter=r(m,0x487638),backend=r(m,0x480da0),block=r(m,island+0x68);
            if(r(m,r(m,adapter)+0x14)!=0x148a70u||
               (block&&(sign(block)<sign(backend+0x4340u)||sign(block)>sign(backend+0xf0340u)||
                r(m,r(m,backend)+0x14)!=0x22eb20u)))
                missing("Required original island retained-bank free owner unavailable");
            if(block)source_free_physics_body_small_block_1_1(m,backend,block,(capacity&0x7fffffffu)*16u,o);
        }
        const auto block=allocate(m,count*16u,o);w(m,o,0x31db06,island+0x68,4,block);w(m,o,0x31db0d,island+0x70,4,count);
    }
    w(m,o,0x31db15,island+0x6c,4,count);
    auto dst=r(m,island+0x68),src=r(m,descriptor);
    for(std::uint32_t k=0;sign(k)<sign(count);++k)for(unsigned j=0;j<4;++j)
        w(m,o,std::array{0x31db26u,0x31db2bu,0x31db31u,0x31db3cu}[j],dst+k*16u+j*4u,4,r(m,src+k*16u+j*4u));
    source_destroy_physics_island_pair_bank_1_1(m,descriptor,s,o);
}
void source_append_physics_island_pair_1_1(M&m,std::uint32_t list,std::uint32_t pair,const SourcePhysicsPairManagerScratch&scratch,const S&s,const O&o) {
    while(r(m,list+0x80)<r(m,list+0x84)) {
        const auto old=r(m,list+0x80),a=r(m,r(m,old)+4),b=r(m,r(m,pair)+4);
        const bool before=a<b||(a==b&&r(m,r(m,old+4)+4)<r(m,r(m,pair+4)+4));
        if(!before)break;
        const auto dst=r(m,list+0x88);
        w(m,o,0x31d3b5,dst,4,r(m,old));w(m,o,0x31d3ba,dst+4,4,r(m,old+4));
        w(m,o,0x31d3c0,dst+8,4,r(m,old+8));w(m,o,0x31d3c6,dst+12,4,r(m,old+12));
        w(m,o,0x31d3de,list+0x88,4,r(m,list+0x88)+16u);w(m,o,0x31d3e4,list+0x80,4,r(m,list+0x80)+16u);
    }
    if(r(m,list+0x50)&&r(m,list+0x80)!=r(m,list+0x84)) {
        const auto old=r(m,list+0x80);
        if(r(m,r(m,pair)+4)==r(m,r(m,old)+4)&&r(m,r(m,old+4)+4)==r(m,r(m,pair+4)+4))return;
    }
    const auto fresh=r(m,list+0x88),ha=r(m,pair),hb=r(m,pair+4),a=ha?ha-16u:0u,b=hb?hb-16u:0u;
    w(m,o,0x31d45e,fresh,4,a?a+16u:0u);w(m,o,0x31d469,fresh+4,4,b?b+16u:0u);
    const auto ba=r(m,a+0x20),bb=r(m,b+0x20),ia=r(m,ba+0x44),ib=r(m,bb+0x44);
    if(ia!=ib) {
        const auto other=ia==list-0x1cu?ib:ia;
        if(r(m,other+0x24)!=0xffffffffu&&!r(m,other+0x29,1))required(m,s,0x3131c0,r(m,list+4),{other},false,o);
        if(!r(m,other+0x30,1)){required(m,s,0x31d210,other,{pair},false,o);return;}
        if(!r(m,ba+0x40,1)&&!r(m,bb+0x40,1))source_queue_physics_island_merge_1_1(m,r(m,list+4),ba,bb,o);
    }
    const auto context=r(m,list+0x7c),table=r(m,context),idx=sbyte(r(m,ba+0x48,1))*8u+sbyte(r(m,bb+0x48,1));
    const auto factory=r(m,table+idx*4u+0xc),factory_method=r(m,r(m,factory)+4);
    const auto manager=factory_method==0x31f770?
        source_allocate_physics_pair_contact_manager_1_1(m,factory,{a,b,context},s,o):
        required(m,s,factory_method,factory,{a,b,context},false,o);
    const auto current_context=r(m,list+0x7c),current_table=r(m,current_context);
    const auto first_shape_type=type(m,s,r(m,a),0x14,o);
    const auto second_shape_type=type(m,s,r(m,b),0x14,o);
    const auto ty=first_shape_type*32u+second_shape_type,agent_method=r(m,current_table+ty*4u+0x18c);
    std::uint32_t agent;
    if(agent_method==0x300170)agent=source_allocate_physics_pair_agent_1_1(m,{a,b,current_context,manager},o);
    else if(agent_method==0x2f3730) {
        if(!s.floating_point)missing("Required original transformed-agent FP context unavailable");
        agent=source_allocate_physics_transformed_pair_agent_1_1(m,{a,b,current_context,manager},scratch.entry_esp-48u,*s.floating_point,s,o);
    }else agent=required(m,s,agent_method,0,{a,b,current_context,manager},true,o);
    w(m,o,0x31d54e,fresh+8,4,agent);
    if(agent&&r(m,agent+8)) {
        const auto current_a=r(m,a+0x20),current_b=r(m,b+0x20);
        const auto first_motion=type(m,s,r(m,current_a+0x3c),0x18,o);
        bool unconditional=false;
        if(first_motion==6||r(m,current_a+0x40,1))unconditional=type(m,s,r(m,current_b+0x3c),0x18,o)==6||r(m,current_b+0x40,1);
        if(unconditional)w(m,o,0x31d5bc,fresh+12,4,0xffffffffu);
        else {
            const auto validator=r(m,r(m,list+4)+0x250),target=r(m,r(m,validator)+4);
            // The actual27DA90 owns an output byte. Its caller ABI scratch
            // is not retained as an entity or contact identity.
            const auto out=target==0x27da90?
                source_write_physics_pair_validation_1_1(m,validator,scratch.entry_esp+4u,a,b,s,o):
                required(m,s,target,validator,{scratch.entry_esp+4u,a,b},false,o);
            w(m,o,0x31d5b3,fresh+12,4,r(m,out,1)?0u:0xffffffffu);
        }
    }else {
        w(m,o,0x31d5c5,fresh+8,4,0);required(m,s,r(m,r(m,manager)+0x10),manager,{},false,o);
    }
    w(m,o,0x31d5d3,list+0x88,4,r(m,list+0x88)+16u);
}
} // namespace halo2
