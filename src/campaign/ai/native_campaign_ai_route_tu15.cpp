#include "halo2/native_campaign_ai_route_tu15.hpp"
#include "halo2/native_campaign_ai_route_segments_tu15.hpp"
#include "halo2/native_campaign_ai_route_smoothing_tu15.hpp"
#include "halo2/native_campaign_ai_route_special_link_tu15.hpp"
#include "halo2/native_campaign_ai_path_candidates_tu15.hpp"
#include "halo2/native_campaign_ai_point_geometry_tu15.hpp"
#include "halo2/native_campaign_ai_vector_math_tu15.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <vector>

namespace halo2 {
NativeCampaignAiRoutePendingIntegerErrorTu15::NativeCampaignAiRoutePendingIntegerErrorTu15(
    NativeCampaignAiRoutePendingIntegerTu15 pending,std::exception_ptr cause):
    std::runtime_error("Native route integer store rejected with pending extended quantity"),pending_(pending),cause_(std::move(cause)){}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace {
using u8=std::uint8_t;using u16=std::uint16_t;using u32=std::uint32_t;
using i8=std::int8_t;using i16=std::int16_t;using i32=std::int32_t;
i32 signed_word(u32 value){return std::bit_cast<i16>(u16(value));}
u32 word_offset(u32 value,u32 stride){return u32(signed_word(value))*stride;}
constexpr u32 none=0xffffffffu;

// A single native scalar operation owns its MXCSR. Memory operations occur
// between operations in source order, including before a failing read.
struct Scalar {
    NativeTu15PhysicsFrameFp &fp;
    template<class F> u32 calculate(u32 left,u32 right,F operation)const {
        u32 host{},csr=fp.mxcsr;asm volatile("stmxcsr %0\n\tldmxcsr %1":"=m"(host):"m"(csr):"memory");
        auto a=std::bit_cast<float>(left);const auto b=std::bit_cast<float>(right);operation(a,b);
        asm volatile("stmxcsr %0\n\tldmxcsr %1":"=m"(fp.mxcsr):"m"(host):"memory");return std::bit_cast<u32>(a);
    }
    u32 add(u32 a,u32 b)const{return calculate(a,b,[](float &x,float y){asm volatile("addss %1,%0":"+x"(x):"x"(y):"memory");});}
    u32 sub(u32 a,u32 b)const{return calculate(a,b,[](float &x,float y){asm volatile("subss %1,%0":"+x"(x):"x"(y):"memory");});}
    u32 mul(u32 a,u32 b)const{return calculate(a,b,[](float &x,float y){asm volatile("mulss %1,%0":"+x"(x):"x"(y):"memory");});}
    bool above(u32 left,u32 right)const {
        u32 host{},csr=fp.mxcsr;unsigned char result{};const auto a=std::bit_cast<float>(left),b=std::bit_cast<float>(right);
        asm volatile("stmxcsr %[host]\n\tldmxcsr %[csr]\n\tcomiss %[b],%[a]\n\tseta %[result]\n\tstmxcsr %[csr]\n\tldmxcsr %[host]"
            :[host]"+m"(host),[csr]"+m"(csr),[result]"=&q"(result):[a]"x"(a),[b]"x"(b):"memory","cc");
        fp.mxcsr=csr;return result;
    }
    u32 integer(i32 value)const {
        u32 host{},csr=fp.mxcsr;float result{};
        asm volatile("stmxcsr %0\n\tldmxcsr %1\n\tcvtsi2ss %3,%2\n\tstmxcsr %1\n\tldmxcsr %0"
            :"+m"(host),"+m"(csr),"=x"(result):"r"(value):"memory");fp.mxcsr=csr;return std::bit_cast<u32>(result);
    }
};

class Route {
public:
    SourcePhysicsContactMemory &memory;SourcePhysicsWorldRuntimeAbi &abi;
    const NativeCampaignAiPerformServicesTu15 &services;NativeTu15PhysicsFrameFp &fp;
    const SourcePhysicsContactRecordObserver &observe;Scalar scalar{fp};
    u32 r(u32 address,unsigned width=4)const {
        std::array<std::byte,4> raw{};memory.read(address,std::span(raw).first(width));u32 value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<u32>(raw[n])<<(n*8u);return value;
    }
    void w(u32 site,u32 address,u32 value,unsigned width=4)const {
        SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(width)};
        if(observe){event.before.resize(width);memory.read(address,event.before);}
        for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));
        memory.write(address,event.written);if(observe)observe(event);
    }
    void push(u32 &sp,u32 value)const {
        sp-=4u;std::array<std::byte,4> raw{};
        for(unsigned n=0;n<4;++n)raw[n]=std::byte(value>>(n*8u));memory.write(sp,raw);
    }
    void copy(u32 source,u32 destination,std::initializer_list<u32> sites)const {
        for(auto site:sites){const auto value=r(source);w(site,destination,value);source+=4u;destination+=4u;}
    }
    void repeat_copy(u32 source,u32 destination,u32 bytes,u32 dword_site,u32 byte_site)const {
        for(u32 n=0;n<(bytes>>2u);++n){const auto value=r(source);w(dword_site,destination,value);source+=4u;destination+=4u;}
        for(u32 n=0;n<(bytes&3u);++n){const auto value=r(source,1);w(byte_site,destination,value,1);++source;++destination;}
    }
    void unported(u32 site,u32 target,std::array<u32,8> gp,u32 cleanup) {
        const auto caller_sp=gp[4];push(gp[4],site+5u);abi.gp=gp;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
        if(!services.callee)throw NativeCampaignAiPerformRequiredOwnerTu15(site,target);
        services.callee(site,target,abi);
        if(!abi.floating_point || abi.gp[4]!=caller_sp+cleanup)
            throw std::runtime_error("Native route reference callee returned another ABI");
        fp.control_word=abi.floating_point->x87_control_word;fp.mxcsr=abi.floating_point->mxcsr;fp.status=abi.x87_status;
    }
    void special_link(std::array<u32,8> gp) {
        const auto caller_sp=gp[4];push(gp[4],0x27ad13u);abi.gp=gp;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
        const auto import=[&] {
            fp.control_word=abi.floating_point->x87_control_word;fp.mxcsr=abi.floating_point->mxcsr;fp.status=abi.x87_status;
        };
        try {native_campaign_ai_route_special_link_call_tu15(memory,0x279100u,abi,services,observe);}
        catch(...){import();throw;}
        import();
        if(abi.gp[4]!=caller_sp+0x1cu)throw std::runtime_error("Native special link returned another source frame");
    }
    u16 reconstruct(u32 workspace,u32 entry,const std::array<u32,8> &caller);
    u8 materialize(u32 workspace,u32 entry,const std::array<u32,8> &caller);
    u8 after_segments(u32 workspace,u32 output,u32 frame);
    u8 after_smoothing(u32 workspace,u32 output,u32 frame,u32 actor,bool result);
    u8 publish(u32 workspace,u32 output,u32 frame);
    u8 distance(u32 first,u32 second,u32 destination,u32 store_site,u32 stack,bool read_result_before=false);
    void integer_store(u32 value,u32 destination,u32 site);
};

u16 Route::reconstruct(u32 workspace,u32 entry,const std::array<u32,8> &caller) {
    // 27AAA0: the parent links are signed WORD indices into 0x44-byte rows.
    u32 sp=entry-0xe4u;
    const auto index=r(entry+0xcu,2),capacity=r(entry+0x10u,2);
    push(sp,caller[5]);u32 rows=r(entry+0x14u);
    const auto depth=signed_word(r(workspace+0xb0u+word_offset(index,0x44u)+0x3eu,2));
    push(sp,caller[6]);push(sp,caller[7]);const auto f=sp;
    const auto initialized=std::min(depth+1,signed_word(capacity));
    // 27ABD2 stores a DWORD although its count consumers use the low WORD.
    // Its upper bits come from the preceding metadata-selection operand;
    // retain that native value instead of zero-extending the depth.
    u32 depth_count_operand=u32(initialized);
    u32 previous_node=0;
    w(0x27aad9u,f+0x18u,none);w(0x27aae1u,f+0x10u,0);w(0x27aae5u,f+0xeu,1,1);w(0x27aaeau,f+0x1cu,0);
    if(signed_word(u32(initialized))>0) {
        depth_count_operand=none;
        for(u32 n=0;n<u16(initialized);++n)w(0x27aaffu,f+0x50u+n*4u,none);
        for(u32 n=0;n<u16(initialized);++n){const auto at=rows+n*0x1cu;w(0x27ab04u,at,0xffffu,2);w(0x27ab08u,at+8u,0xffffu,2);w(0x27ab0cu,at+2u,0,2);}
        previous_node=r(f+0x10u);
    }
    const auto first_index=r(entry+0xcu,2);w(0x27ab26u,f+0x14u,0);
    if(first_index!=0xffffu)for(;;) {
        const auto current=r(entry+0xcu,2);const auto pair_count=r(f+0x1cu);
        const auto pair_capacity=r(entry+0x24u,2);const auto node=workspace+0xb0u+word_offset(current,0x44u);
        if(signed_word(pair_count)<signed_word(pair_capacity)) {
            const auto kind=r(node+0x10u,2);const auto navigation=r(workspace+0x8cu);
            bool retain=kind!=0xffffu;
            if(!retain) {
                const auto edge=r(node+0x14u);
                depth_count_operand=edge;
                if(std::bit_cast<i32>(edge)>=0 && std::bit_cast<i32>(edge)<std::bit_cast<i32>(r(navigation+8u))) {
                    const auto edges=r(navigation+0xcu),edge_flags=r(edges+edge*16u+4u,1);
                    depth_count_operand=(u32(i32(std::bit_cast<i8>(u8(edge_flags))))>>7u)&0xffffff01u;
                    const auto flag=depth_count_operand&0xffu;
                    w(0x27ab89u,f+0xfu,flag,1);retain=flag!=0;
                }
            }
            if(retain) {
                const auto pairs=r(entry+0x20u),at=pairs+word_offset(pair_count,8u);
                depth_count_operand=pairs;
                const auto kind_word=r(node+0x10u);w(0x27ab9cu,at,kind_word);
                const auto link=r(node+0x14u);w(0x27aba2u,at+4u,link);
                const auto count=r(f+0x1cu);w(0x27aba6u,f+0x1cu,count+1u);
            }
        }
        const auto node_depth=r(node+0x3eu,2),limit=r(entry+0x10u,2);
        depth_count_operand=(depth_count_operand&0xffff0000u)|node_depth;
        if(signed_word(node_depth)>=signed_word(limit))w(0x27abb8u,f+0xeu,0,1);
        else {
            const auto count=r(f+0x14u,2);w(0x27abc8u,f+0xfu,0,1);
            if(count==0)w(0x27abd2u,f+0x14u,depth_count_operand+1u);
            const auto sector=r(node+8u),last=r(f+0x18u,2),at=rows+word_offset(node_depth,0x1cu);
            w(0x27abe5u,at+4u,sector);
            if(last==0xffffu) {
                const auto d=r(node+0x3eu,2),goal=r(entry+8u);
                copy(goal+4u,rows+word_offset(d,0x1cu)+0xcu,{0x27ac02u,0x27ac07u,0x27ac0du,0x27ac13u});
            } else {
                const auto d=r(node+0x3eu,2),previous=previous_node;
                copy(previous+0x18u,rows+word_offset(d,0x1cu)+0xcu,{0x27ac28u,0x27ac2du,0x27ac33u,0x27ac39u});
                if(r(previous+0x10u,2)==0xffffu){const auto again=r(node+0x3eu,2),edge=r(previous+0x14u);w(0x27ac4au,f+0x50u+word_offset(again,4u),edge);}
            }
            const auto kind=r(node+0x10u,2);u32 special{};
            if(kind==2u) {
                const auto nav=r(workspace+0x8cu),link=r(node+0x14u),links=r(nav+0x3cu);
                special=r(links+link*20u+0xau,2)==0 ? 1u:r(f+0xfu,1);
            } else special=r(f+0xfu,1);
            if(kind==1u || kind==6u || kind==5u || special) {
                const auto d=r(node+0x3eu,2),parent=r(node+2u,2);
                w(0x27acbeu,rows+word_offset(d,0x1cu)-0x1cu,kind,2);
                const auto d2=r(node+0x3eu,2),link_word=r(node+0x14u,2);
                w(0x27acd1u,rows+word_offset(d2,0x1cu)-0x14u,link_word,2);
                const auto d3=r(node+0x3eu,2),destination=rows+word_offset(d3,0x1cu);
                const auto parent_node=workspace+0xb0u+word_offset(parent,0x44u);
                u32 call_sp=f;push(call_sp,destination-0x18u);push(call_sp,destination-0x10u);
                const auto link=r(node+0x14u);push(call_sp,parent_node+8u);const auto parent_sector=r(parent_node+8u);
                push(call_sp,parent_sector);push(call_sp,parent_node+0x18u);
                const auto actor=r(entry+4u);push(call_sp,workspace);push(call_sp,actor);const auto navigation=r(workspace+0x8cu);
                special_link({navigation,link,parent_sector,workspace,call_sp,rows,node,node+0x18u});
            } else if(kind==2u) {
                const auto d=r(node+0x3eu,2);w(0x27ac9cu,rows+word_offset(d,0x1cu)-0x1cu,kind,2);
                const auto d2=r(node+0x3eu,2),link=r(node+0x14u,2);w(0x27acacu,rows+word_offset(d2,0x1cu)-0x14u,link,2);
            }
        }
        const auto prior=r(entry+0xcu),parent=r(node+2u,2);
        w(0x27ad24u,f+0x18u,prior);w(0x27ad28u,f+0x10u,node);w(0x27ad2cu,entry+0xcu,parent);
        depth_count_operand=parent;
        if(parent==0xffffu)break;
        previous_node=r(f+0x10u);
    }
    u32 count=first_index==0xffffu?0u:r(f+0x14u);
    const auto width=r(workspace),factor=r(0x46a1f8u);const auto margin=scalar.mul(width,factor);
    w(0x27ad4eu,f+0x18u,margin);w(0x27ad54u,f+0x10u,0);
    if(signed_word(count)>0)for(u32 n=0;;) {
        const auto edge=r(f+0x50u+word_offset(n,4u));
        if(std::bit_cast<i32>(edge)>=0) {
            const auto navigation=r(workspace+0x8cu),edges_count=r(navigation+8u);
            if(std::bit_cast<i32>(edge)<std::bit_cast<i32>(edges_count) && edge!=0xffffu) {
                const auto previous=signed_word(n)>0 ? rows+word_offset(n,0x1cu)-0x10u:workspace+0x14u;
                const auto edges=r(navigation+0xcu),edge_record=edges+edge*16u;
                const auto vertex_a=r(edge_record,2),vertex_b=r(edge_record+2u,2);
                const auto point=rows+word_offset(n,0x1cu)+0xcu;
                // MOVZX followed by CMP -1: even FFFF is not the dword NONE.
                if(vertex_a!=none && vertex_b!=none) {
                    const auto nav_again=r(workspace+0x8cu),vertices=r(nav_again+0x2cu);
                    const auto a=vertices+vertex_a*12u,b=vertices+vertex_b*12u;
                    const auto bx=r(b),ax=r(a);w(0x27ade0u,f+0x38u,scalar.sub(bx,ax));
                    const auto by=r(b+4u),ay=r(a+4u);w(0x27adf4u,f+0x3cu,scalar.sub(by,ay));
                    const auto bz=r(b+8u),az=r(a+8u);w(0x27ae04u,f+0x40u,scalar.sub(bz,az));
                    const auto px=r(previous),ax2=r(a);w(0x27ae12u,f+0x44u,scalar.sub(px,ax2));
                    const auto py=r(previous+4u),ay2=r(a+4u);w(0x27ae22u,f+0x48u,scalar.sub(py,ay2));
                    const auto pz=r(previous+8u),az2=r(a+8u);w(0x27ae36u,f+0x4cu,scalar.sub(pz,az2));
                    {u32 call_sp=f;push(call_sp,0x27ae41u);}
                    auto magnitude=native_campaign_ai_normalize_vector_tu15(memory,f+0x38u,fp,observe);
                    const auto padding=r(f+0x18u),twice=r(0x46a11cu);
                    if(scalar.above(magnitude,scalar.mul(padding,twice))) {
                        auto z=r(f+0x40u),dz=r(f+0x4cu),y=r(f+0x3cu),dy=r(f+0x48u),dx=r(f+0x44u);
                        dz=scalar.mul(dz,z);dy=scalar.mul(dy,y);auto projection=scalar.add(dz,dy);
                        auto x=r(f+0x38u);dx=scalar.mul(dx,x);projection=scalar.add(projection,dx);
                        const auto bound=r(f+0x18u);
                        if(scalar.above(bound,projection))projection=bound;
                        else {magnitude=scalar.sub(magnitude,bound);if(scalar.above(projection,magnitude))projection=magnitude;}
                        x=scalar.mul(x,projection);const auto origin_x=r(a);w(0x27aeb4u,point,scalar.add(x,origin_x));
                        y=scalar.mul(y,projection);const auto origin_y=r(a+4u);w(0x27aec2u,point+4u,scalar.add(y,origin_y));
                        z=scalar.mul(z,projection);const auto origin_z=r(a+8u);w(0x27aed0u,point+8u,scalar.add(z,origin_z));
                    }
                }
                const auto start_frame=r(workspace+0x20u,2),point_frame=r(point+0xcu,2);
                u32 x,y,z;
                if(start_frame==point_frame) {
                    x=r(point);y=r(point+4u);z=r(point+8u);
                    const auto px=r(workspace+0x14u);x=scalar.sub(x,px);
                    const auto py=r(workspace+0x18u);y=scalar.sub(y,py);
                    const auto pz=r(workspace+0x1cu);z=scalar.sub(z,pz);
                } else {
                    bool resolved=false;
                    if(start_frame!=0xffffu){u32 call_sp=f;push(call_sp,workspace+0x14u);push(call_sp,0x27af11u);resolved=native_campaign_ai_resolve_frame_tu15(memory,u16(start_frame),workspace+0x14u,f+0x20u,fp,observe);}
                    if(!resolved){const auto a=r(workspace+0x14u),b=r(workspace+0x18u),c=r(workspace+0x1cu);w(0x27af1du,f+0x20u,a);w(0x27af21u,f+0x24u,b);w(0x27af25u,f+0x28u,c);}
                    const auto destination_frame=r(point+0xcu,2);resolved=false;
                    if(destination_frame!=0xffffu){u32 call_sp=f;push(call_sp,point);push(call_sp,0x27af3fu);resolved=native_campaign_ai_resolve_frame_tu15(memory,u16(destination_frame),point,f+0x2cu,fp,observe);}
                    if(!resolved){const auto a=r(point),b=r(point+4u),c=r(point+8u);w(0x27af4cu,f+0x2cu,a);w(0x27af50u,f+0x30u,b);w(0x27af54u,f+0x34u,c);}
                    x=r(f+0x2cu);y=r(f+0x30u);z=r(f+0x34u);
                    const auto a=r(f+0x20u);x=scalar.sub(x,a);const auto b=r(f+0x24u);y=scalar.sub(y,b);const auto c=r(f+0x28u);z=scalar.sub(z,c);
                }
                const auto yy=scalar.mul(y,y),zz=scalar.mul(z,z);auto squared=scalar.add(zz,yy);const auto xx=scalar.mul(x,x);squared=scalar.add(squared,xx);
                if(scalar.above(squared,r(0x46a8c8u))) {
                    const auto limit=r(f+0x14u,2),current=r(f+0x10u,2);
                    if(signed_word(current)<signed_word(limit)-1){const auto at=r(f+0x10u);w(0x27afb3u,f+0x14u,at+1u);w(0x27afb7u,f+0xeu,0,1);}
                }
                n=r(f+0x10u);rows=r(entry+0x14u);
            }
        }
        count=r(f+0x14u);++n;w(0x27afcfu,f+0x10u,n);if(signed_word(n)>=signed_word(count))break;
    }
    const auto complete=r(entry+0x18u);
    if(complete){const auto value=r(f+0xeu,1);w(0x27afebu,complete,value,1);}
    const auto pair_output=r(entry+0x1cu);
    if(pair_output){const auto value=r(f+0x1cu,2);w(0x27affdu,pair_output,value,2);}
    return u16(count);
}

// 218C40's source association, followed by the caller's actual FSTP store.
u8 Route::distance(u32 first,u32 second,u32 destination,u32 store_site,u32 stack,bool read_result_before) {
    const auto frame_a=r(first+0xcu,2),frame_b=r(second+0xcu,2);u32 squared{};
    if(frame_a==frame_b) {
        const auto bx=r(second),ax=r(first);const auto x=scalar.sub(bx,ax);
        const auto bz=r(second+8u),az=r(first+8u);const auto z=scalar.sub(bz,az);
        const auto by=r(second+4u),ay=r(first+4u);const auto y=scalar.sub(by,ay);
        const auto xx=scalar.mul(x,x),zz=scalar.mul(z,z);auto sum=scalar.add(xx,zz);squared=scalar.add(sum,scalar.mul(y,y));
        w(0x218c89u,stack-0x1cu,squared);
    } else {
        native_campaign_ai_project_point_tu15(memory,first,stack-0xcu,fp,observe);
        native_campaign_ai_project_point_tu15(memory,second,stack-0x18u,fp,observe);
        const auto bz=r(stack-0x10u),az=r(stack-4u);const auto z=scalar.sub(bz,az);
        const auto by=r(stack-0x14u),ay=r(stack-8u);const auto y=scalar.sub(by,ay);
        const auto bx=r(stack-0x18u),ax=r(stack-0xcu);const auto x=scalar.sub(bx,ax);
        auto sum=scalar.mul(z,z);const auto yy=scalar.mul(y,y);sum=scalar.add(sum,yy);squared=scalar.add(sum,scalar.mul(x,x));
        w(0x218cefu,stack-0x1cu,squared);
    }
    const auto value=r(stack-0x1cu);
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    asm volatile("flds %0\n\tfsqrt"::"m"(value):"memory","st");
    alignas(16)std::array<std::byte,108> before{};
    asm volatile("fnsave %0\n\tfrstor %0":"=m"(before)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    NativeCampaignAiPendingDistanceTu15 pending;
    asm volatile("fstpt %0\n\tfrstor %1":"=m"(pending.extended):"m"(before):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    u8 result_byte{};
    try {if(read_result_before)result_byte=u8(r(destination-0x18u,1));}
    catch(...){throw NativeCampaignAiPendingDistanceErrorTu15(pending,std::current_exception());}
    SourcePhysicsContactRecordStore event{store_site,destination,{},std::vector<std::byte>(4)};
    try {if(observe){event.before.resize(4);memory.read(destination,event.before);}}
    catch(...){throw NativeCampaignAiPendingDistanceErrorTu15(pending,std::current_exception());}
    u32 result{};asm volatile("fstps %0":"=m"(result)::"memory","st");
    for(unsigned n=0;n<4;++n)event.written[n]=std::byte(result>>(n*8u));
    try {memory.write(destination,event.written);}
    catch(...) {
        asm volatile("frstor %0"::"m"(before):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        throw NativeCampaignAiPendingDistanceErrorTu15(pending,std::current_exception());
    }
    if(observe)observe(event);
    return result_byte;
}

void Route::integer_store(u32 value,u32 destination,u32 site) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);asm volatile("flds %0"::"m"(value):"memory","st");
    alignas(16)std::array<std::byte,108> before{};
    asm volatile("fnsave %0\n\tfrstor %0":"=m"(before)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    NativeCampaignAiRoutePendingIntegerTu15 pending;
    asm volatile("fstpt %0\n\tfrstor %1":"=m"(pending.extended):"m"(before):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    SourcePhysicsContactRecordStore event{site,destination,{},std::vector<std::byte>(4)};
    try {if(observe){event.before.resize(4);memory.read(destination,event.before);}}
    catch(...){throw NativeCampaignAiRoutePendingIntegerErrorTu15(pending,std::current_exception());}
    u32 result{};asm volatile("fistpl %0":"=m"(result)::"memory","st");
    for(unsigned n=0;n<4;++n)event.written[n]=std::byte(result>>(n*8u));
    try {memory.write(destination,event.written);}
    catch(...) {
        asm volatile("frstor %0"::"m"(before):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        throw NativeCampaignAiRoutePendingIntegerErrorTu15(pending,std::current_exception());
    }
    if(observe)observe(event);
}

u8 Route::materialize(u32 workspace,u32 entry,const std::array<u32,8> &caller) {
    u32 sp=entry-0x5acu;push(sp,caller[3]);push(sp,caller[5]);
    const auto output=r(entry+8u),actor=r(entry+4u);push(sp,caller[6]);push(sp,caller[7]);const auto f=sp;
    w(0x27a65du,f+0x18u,0);
    if(actor!=none){const auto pool=r(0x5020c8u),rows=r(pool+0x44u);w(0x27a67du,f+0x18u,rows+(actor&0xffffu)*0x888u);}
    w(0x27a681u,output,0,1);repeat_copy(workspace+0x70u,output+0xa8u,28u,0x27a693u,0x27a693u);w(0x27a695u,output+0xa6u,0,1);
    if(!r(workspace+0x54u,1))return u8(r(output,1));
    const auto key=r(workspace+0x68u);{u32 call_sp=f;push(call_sp,0x27a6b1u);}
    u16 index=u16(native_campaign_ai_path_find_node_tu15(memory,workspace,key));
    u32 copied_esi{};
    if(index!=0xffffu) {
        copy(workspace+0x58u,output+4u,{0x27a6c2u,0x27a6c7u,0x27a6cdu,0x27a6d3u});
        copied_esi=r(workspace+0x68u);w(0x27a6d9u,output+0x14u,copied_esi);
        const auto tail=r(workspace+0x6cu);w(0x27a6dfu,output+0x18u,tail);w(0x27a6e2u,output+0x18u,0);
    } else {
        const auto permitted=r(workspace+0x6cu),distance=r(workspace+0x94u);
        if(!scalar.above(permitted,distance))return u8(r(output,1));
        index=u16(r(workspace+0x90u,2));copy(workspace+0x9cu,output+4u,{0x27a70du,0x27a712u});
        copied_esi=r(workspace+0xa4u);w(0x27a718u,output+0xcu,copied_esi);
        const auto tail=r(workspace+0xa8u);w(0x27a71eu,output+0x10u,tail);
        const auto sector=r(workspace+0xb8u+word_offset(index,0x44u));w(0x27a72eu,output+0x14u,sector);
        const auto cost=r(workspace+0x94u);w(0x27a737u,output+0x18u,cost);
    }
    if(index==0xffffu)return u8(r(output,1));
    u32 call_sp=f;push(call_sp,10u);w(0x27a748u,f+0x24u,0);w(0x27a74cu,f+0x14u,0);w(0x27a750u,f+0x28u,0);
    w(0x27a756u,f+0x12u,1,1);w(0x27a75au,f+0x13u,1,1);
    push(call_sp,f+0x2cu);push(call_sp,f+0x28u);push(call_sp,f+0x12u);
    const auto actor_again=r(entry+4u);push(call_sp,f+0x15cu);push(call_sp,40u);push(call_sp,index);push(call_sp,output);push(call_sp,actor_again);push(call_sp,0x27a786u);
    // Both children save these live source words, including the EDI left by
    // the initial seven-DWORD REP copy. Capture ESI from its existing read.
    auto nested=caller;nested[3]=workspace;nested[5]=output;nested[6]=copied_esi;nested[7]=output+0xc4u;
    const auto count=reconstruct(workspace,call_sp,nested);
    call_sp=f;push(call_sp,f+0x12u);push(call_sp,f+0xecu);push(call_sp,f+0x24u);push(call_sp,f+0x15cu);push(call_sp,count);push(call_sp,workspace);
    const auto caller_sp=call_sp;
    push(call_sp,0x27a7a7u);
    abi.gp={count,f+0x24u,f+0x15cu,workspace,call_sp,output,copied_esi,output+0xc4u};
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};
    abi.x87_status=fp.status;
    const auto retain_child_fp=[&] {
        if(abi.floating_point){fp.control_word=abi.floating_point->x87_control_word;fp.mxcsr=abi.floating_point->mxcsr;}
        fp.status=abi.x87_status;
    };
    try{native_campaign_ai_route_segments_call_tu15(memory,0x2cbee0u,abi,services,observe);}
    catch(...){retain_child_fp();throw;}
    if(!abi.floating_point || abi.gp[4]!=caller_sp+0x18u)
        throw std::runtime_error("Native route segment owner returned another ABI");
    retain_child_fp();
    return after_segments(workspace,output,f);
}

u8 Route::after_segments(u32 workspace,u32 output,u32 f) {
    const auto actor_row=r(f+0x18u);
    if(actor_row && r(actor_row+0x478u,1)) {
        const auto count=r(f+0x24u);w(0x27a7c1u,f+0x14u,4);
        if(signed_word(count)<=4)w(0x27a7cbu,f+0x14u,count);
        const auto n=r(f+0x14u,2);repeat_copy(f+0xecu,f+0x7cu,word_offset(n,0x1cu),0x27a7e7u,0x27a7eeu);
        return publish(workspace,output,f);
    }
    const auto forced=r(workspace+0xacu,1);w(0x27a800u,f+0x1cu,none);w(0x27a804u,f+0x20u,none);w(0x27a808u,f+0x18u,forced,1);
    if(!forced) {
        const auto attempts=r(output+0xa8u,2);
        if(signed_word(attempts)>0) {
            const auto shift=r(output+0xaau,1)&31u;
            const auto divisor=std::bit_cast<i32>(u32(1u<<shift));
            const auto quotient=signed_word(attempts)/divisor;
            w(0x27a840u,f+0x18u,quotient%2==1,1);
        }
    }
    const auto actor=r(f+0x5c0u);u32 call_sp=f;
    push(call_sp,output+0xa6u);push(call_sp,f+0x20u);push(call_sp,f+0x1cu);push(call_sp,f+0x12u);
    const auto mode=r(f+0x18u);push(call_sp,f+0x7cu);push(call_sp,f+0x14u);
    const auto count=r(f+0x24u);push(call_sp,mode);push(call_sp,f+0xecu);push(call_sp,count);push(call_sp,workspace);push(call_sp,actor);
    const auto caller_sp=call_sp;push(call_sp,0x27a887u);
    abi.gp={mode,f+0xecu,count,workspace,call_sp,output,actor,none};
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
    const auto retain_child_fp=[&] {
        if(abi.floating_point){fp.control_word=abi.floating_point->x87_control_word;fp.mxcsr=abi.floating_point->mxcsr;}
        fp.status=abi.x87_status;
    };
    try{native_campaign_ai_route_smoothing_call_tu15(memory,0x2ce030u,abi,services,observe);}
    catch(...){retain_child_fp();throw;}
    if(!abi.floating_point || abi.gp[4]!=caller_sp+0x2cu)
        throw std::runtime_error("Native route smoothing owner returned another ABI");
    retain_child_fp();
    return after_smoothing(workspace,output,f,actor,(abi.gp[0]&0xffu)!=0);
}

u8 Route::after_smoothing(u32 workspace,u32 output,u32 f,u32 actor,bool result) {
    w(0x27a889u,f+0x13u,result,1);
    if(result) {
        const auto record=r(f+0x1cu),kind=r(f+0x20u);w(0x27a899u,output+0xa0u,record);w(0x27a89fu,output+0xa4u,kind,2);
        if(record!=none && actor!=none && u16(kind)==6u) {
            const auto clock=r(0x51df54u),ticks=r(clock+2u,2);auto value=scalar.integer(signed_word(ticks));
            value=scalar.mul(value,r(0x46a16cu));w(0x27a8c7u,f+0x20u,value);
            const auto input=r(f+0x20u);integer_store(input,f+0x1cu,0x27a8d1u);const auto duration=r(f+0x1cu);u32 call_sp=f;
            for(auto word:{3u,0x2au,1u,3u,duration,9u})push(call_sp,word);
            unported(0x27a8e6u,0x1b0e10u,{clock,actor,duration,workspace,call_sp,output,actor,none},0x18u);
        }
    } else {w(0x27a8edu,output+0xa0u,none);w(0x27a8f3u,output+0xa4u,0xffffu,2);}
    return publish(workspace,output,f);
}

u8 Route::publish(u32 workspace,u32 output,u32 f) {
    if(r(output+0xa8u,2)==0) {
        const auto count=r(f+0x28u);
        if(signed_word(count)>0){const auto a=r(f+0x2cu),b=r(f+0x30u);w(0x27a915u,output+0xacu,a);w(0x27a91bu,output+0xb0u,b);}
        if(signed_word(count)>1){const auto a=r(f+word_offset(count,8u)+0x24u),b=r(f+word_offset(count,8u)+0x28u);w(0x27a932u,output+0xb4u,a);w(0x27a938u,output+0xb8u,b);}
        if(signed_word(count)>2){const auto midpoint=u32(signed_word(count)/2);const auto a=r(f+midpoint*8u+0x2cu),b=r(f+midpoint*8u+0x30u);w(0x27a954u,output+0xbcu,a);w(0x27a95au,output+0xc0u,b);}
        w(0x27a977u,output+0xaau,u32(std::clamp(signed_word(count),0,3)),2);
    }
    if(!r(f+0x13u,1)) {
        const auto count=r(output+0xa8u,2);w(0x27aa72u,output+0xa8u,count+1u,2);
        if(signed_word(r(output+0xa8u,2))>32)w(0x27aa83u,output+0xa8u,0,2);
        return u8(r(output,1));
    }
    const auto complete=r(f+0x12u,1);w(0x27a98eu,output+0x2cu,complete,1);
    const auto count=r(f+0x14u);w(0x27a995u,output+0x2du,count,1);w(0x27a99bu,output,1,1);w(0x27a9a2u,output+0x2eu,0,1);
    copy(workspace+0x14u,output+0x1cu,{0x27a9b0u,0x27a9b5u,0x27a9bbu,0x27a9c1u});
    repeat_copy(f+0x7cu,output+0x30u,word_offset(count,0x1cu),0x27a9d0u,0x27a9d7u);
    if(!r(output+0x2cu,1))return u8(r(output,1));
    const auto route_count=r(output+0x2du,1);
    if(std::bit_cast<i8>(u8(route_count))>0) {
        const auto row=output+u32(i32(std::bit_cast<i8>(u8(route_count))))*0x1cu;
        copy(row+0x20u,output+4u,{0x27a9fdu,0x27aa02u,0x27aa08u,0x27aa0eu});const auto sector=r(row+0x18u);w(0x27aa14u,output+0x14u,sector);
        u32 call_sp=f;push(call_sp,0x27aa22u);return distance(output+4u,workspace+0x58u,output+0x18u,0x27aa26u,call_sp,true);
    } else {
        copy(workspace+0x14u,output+4u,{0x27aa3au,0x27aa3fu,0x27aa45u,0x27aa4bu});const auto sector=r(workspace+0x24u);w(0x27aa57u,output+0x14u,sector);
        u32 call_sp=f;push(call_sp,0x27aa5fu);distance(output+4u,workspace+0x58u,output+0x18u,0x27aa5fu,call_sp);
    }
    return u8(r(output,1));
}

struct FpPublication {
    SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;
    ~FpPublication(){abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;}
};
}

void native_campaign_ai_route_call_tu15(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,const NativeCampaignAiPerformServicesTu15 &services,
    const SourcePhysicsContactRecordObserver &observe) {
    if(!abi.floating_point || abi.active_mmx || abi.x87_depth)
        throw std::invalid_argument("Native route requires retained FP controls and an empty x87 stack");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    if((fp.control_word&0x3fu)!=0x3fu || (fp.mxcsr&0x1f80u)!=0x1f80u)
        throw std::invalid_argument("Native route requires masked original FP exceptions");
    FpPublication publication{abi,fp};const auto caller=abi.gp;Route route{memory,abi,services,fp,observe};
    const auto sp=caller[4];u32 result,frame;
    if(entry==0x27aaa0u) {
        result=route.reconstruct(caller[3],sp,caller);frame=sp-0xf0u;
        abi.gp=caller;abi.gp[0]=(caller[0]&0xffff0000u)|result;
        abi.gp[7]=route.r(frame);abi.gp[6]=route.r(frame+4u);abi.gp[5]=route.r(frame+8u);abi.gp[4]=sp+0x28u;return;
    }
    if(entry==0x27a640u){result=route.materialize(caller[0],sp,caller);frame=sp-0x5bcu;}
    else if(entry==0x27a7a7u){frame=sp;result=route.after_segments(caller[3],caller[5],frame);}
    else if(entry==0x27a887u){frame=sp;result=route.after_smoothing(caller[3],caller[5],frame,caller[6],(caller[0]&0xffu)!=0);}
    else throw std::invalid_argument("Native route has no such entry");
    abi.gp=caller;abi.gp[0]=(caller[0]&0xffffff00u)|result;
    abi.gp[7]=route.r(frame);abi.gp[6]=route.r(frame+4u);abi.gp[5]=route.r(frame+8u);abi.gp[3]=route.r(frame+0xcu);abi.gp[4]=frame+0x5c8u;
}
#else
void native_campaign_ai_route_call_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,const NativeCampaignAiPerformServicesTu15 &,
    const SourcePhysicsContactRecordObserver &) {
    throw std::runtime_error("Exact native TU1.5 route arithmetic backend unavailable");
}
#endif
}
