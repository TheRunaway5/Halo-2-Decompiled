#include "halo2/native_campaign_ai_route_traversal_tu15.hpp"
#include "native_campaign_ai_goal_port_tu15.hpp"
#include <algorithm>
namespace halo2 {
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
namespace {
using namespace goal_tu15;
constexpr u32 none=0xffffffffu;
struct Traversal {
    Port p;SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;Simd s{fp};
    const NativeCampaignAiPerformServicesTu15 &services;
    void push(u32 value){abi.gp[4]-=4;std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(8*i));p.memory.write(abi.gp[4],b);}
    void call(u32 site,u32 entry){push(site+5);dispatch(entry);}
    void result(u32 sp,u32 cleanup,bool yes,const std::array<u32,8>&saved){
        abi.gp[0]=(abi.gp[0]&0xffffff00u)|u32(yes);abi.gp[4]=sp+4+cleanup;
        for(unsigned i:{3u,5u,6u,7u})abi.gp[i]=saved[i];
    }
    // COMISS below includes unordered; UCOMISS equality excludes unordered.
    bool below(u32 a,u32 b){return !s.above_equal(a,b);}
    bool equal(u32 a,u32 b){
        u32 host{},csr=fp.mxcsr;unsigned char eq{},unordered{};
        float x=std::bit_cast<float>(a),y=std::bit_cast<float>(b);
        asm volatile("stmxcsr %0\n\tldmxcsr %1\n\tucomiss %5,%4\n\tsete %2\n\tsetp %3\n\tstmxcsr %1\n\tldmxcsr %0"
            :"+m"(host),"+m"(csr),"=qm"(eq),"=qm"(unordered):"x"(x),"x"(y):"memory","cc");fp.mxcsr=csr;return eq&&!unordered;
    }
    void endpoint(u32 direction,u32 start,u32 parameter,u32 output,std::array<u32,3>sites){
        for(unsigned i=0;i<3;++i){auto v=p.r32(direction+4*i);v=s.mul(v,parameter);v=s.add(v,p.r32(start+4*i));p.w32(sites[i],output+4+4*i,v);}
    }
    bool blocked(u32 sector,u32 navigation){
        u32 index=u32(sx16(p.r16(sector+2)));if(index==none)return false;
        const auto links=p.r32(navigation+0x3c);u32 row;
        for(;;){row=links+index*20;if(p.r16(row)==7)break;index=u32(sx16(p.r16(row+2)));if(index==none)return false;}
        const auto devices=p.r32(p.r32(p.r32(0x4ecc18)+0x18)+0x2c),device=p.r32(row+4);
        if(!(p.r8(devices+device*8+4)&8))return false;
        const auto argument=p.r32(row+8),bit=p.r8(devices+device*8+5);
        const auto group=u32(sx16(p.r16(0x474c24))),base=p.r32(0x4f9b50);
        const auto field=argument==none?(group<<5)+base+1:(group<<10)+base+argument+0x201;
        return !(p.r32(field+(bit>>5)*4)&(1u<<(bit&31)));
    }
    void iterator(){
        const auto sp=abi.gp[4],c=abi.gp[1];const auto saved=abi.gp;
        auto link=p.r32(c+0x14);push(saved[3]);push(saved[6]);push(saved[7]);u32 edge=0;
        bool from_link=false;
        if(link!=none){const auto nav=p.r32(c+0x1c);for(;;){
            link=p.r32(c+0x14);p.w32(0x2029f5,c+0x10,link);p.w16(0x2029f8,c,0xffff);
            const auto row=p.r32(nav+0x3c)+link*20;link=u32(sx16(p.r16(row+2)));p.w32(0x202a0a,c+0x14,link);
            if(p.r16(row)==0){const auto index=p.r16(row+4);p.w16(0x202a73,c,index);
                const auto edges=p.r32(nav+0xc),sector=p.r32(c+8);edge=edges+index*16;
                const bool reverse=p.r16(edge+0xe)==sector;p.w8(0x202a8b,c+0x18,reverse);from_link=true;break;}
            if(link==none)break;
        }}
        if(!from_link){const auto index=p.r16(c+4);if(index!=0xffff){
            p.w16(0x202a27,c,index);const auto nav=p.r32(c+0x1c);p.w32(0x202a2d,c+0x10,none);
            const auto edges=p.r32(nav+0xc),sector=p.r32(c+8);edge=edges+index*16;
            const bool reverse=p.r16(edge+0xe)==sector;p.w8(0x202a45,c+0x18,reverse);
            const auto next=p.r16(edge+8+u32(reverse)*2),first=p.r16(c+2);p.w16(0x202a54,c+4,next);
            if(next==first)p.w16(0x202a5a,c+4,0xffff);
        }}
        if(edge){const auto reverse=p.r8(c+0x18);const auto vertex=p.r16(edge+(reverse?0:2));p.w32(0x202a94,c+0xc,vertex);}
        p.w16(0x202a97,c+0x20,p.r16(c+0x20)+1);if(sx16(p.r16(c+0x20))>2000)edge=0;
        abi.gp=saved;abi.gp[0]=edge;abi.gp[4]=sp+4;
    }
    void contact(){
        const auto sp=abi.gp[4];const auto saved=abi.gp;auto second_side=p.r32(sp+0x18);
        push(saved[3]);const auto endpoint_b=p.r32(sp+0x20);push(saved[5]);const auto direction=p.r32(sp+0xc);
        push(saved[6]);const auto out=p.r32(sp+0x24);const auto f=abi.gp[4];
        if(below(second_side,0)||below(0,p.r32(sp+0x14))){result(sp,0x24,false,saved);return;}
        const auto edge_index=p.r32(sp+8);const bool reverse=(saved[1]&255)!=0;
        const auto edges=p.r32(saved[2]+0xc);u32 association=p.r16(edges+edge_index*16+0xc+u32(!reverse)*2);
        p.w32(0x2760d7,sp+0x24,association);
        if(association!=none&&association!=0xffff){const auto sector=p.r32(saved[2]+4)+association*8;const auto flags=p.r16(sector);
            const bool allow=!(flags&1)||((flags&2)&&blocked(sector,saved[2]));
            association=p.r32(sp+0x24);p.w8(allow?0x276112:0x276105,out,allow);
        }else p.w8(0x276116,out,1);
        const auto edge=p.r32(sp+8),opaque=p.r32(sp+4);p.w32(0x276121,out+0x14,edge);
        const auto endpoint_a=p.r32(sp+0x1c);p.w32(0x276128,out+0x10,opaque);
        auto dx=p.r32(endpoint_a),dy=p.r32(endpoint_a+4);dx=s.sub(dx,p.r32(endpoint_b));dy=s.sub(dy,p.r32(endpoint_b+4));
        auto vx=p.r32(direction),vy=p.r32(direction+4),px=p.r32(saved[7]),py=p.r32(saved[7]+4);
        px=s.sub(px,p.r32(endpoint_b));py=s.sub(py,p.r32(endpoint_b+4));
        auto denominator=s.sub(s.mul(vx,dy),s.mul(vy,dx));
        if(!equal(denominator,0)){
            px=s.mul(px,dy);py=s.mul(py,dx);auto t=s.div(s.sub(py,px),denominator);p.w32(0x27617e,out+0x18,t);
            auto x=p.r32(direction);x=s.mul(x,t);x=s.add(x,p.r32(saved[7]));p.w32(0x276190,out+4,x);
            auto y=p.r32(direction+4);y=s.mul(y,t);y=s.add(y,p.r32(saved[7]+4));p.w32(0x2761a3,out+8,y);
            const bool negative=s.above(0,p.r32(out+0x18));const auto z=p.r32(saved[7]+8);p.w32(0x2761af,out+0xc,z);
            auto clamped=negative?0:p.r32(out+0x18);p.w32(0x2761b9,out+0x18,clamped);
        }else{p.w32(0x2761c0,out+0x18,0);for(unsigned i=0;i<3;++i)p.w32(std::array<u32,3>{0x2761ca,0x2761cf,0x2761d5}[i],out+4+i*4,p.r32(saved[7]+i*4));}
        const auto association_out=p.r32(sp+0x10);if(association_out)p.w32(0x2761e0,association_out,association);
        (void)f;result(sp,0x24,true,saved);
    }
    bool excluded(u32 list,u32 edge){
        const auto count=sx16(p.r16(list+2));for(i32 i=0;i<count;++i){
            const auto row=list+u32(i)*8;
            if(p.r16(row+4)==0xffff&&p.r32(row+8)==edge&&(u32(sx16(p.r16(list)))&(1u<<(u32(i)&31))))return true;
        }return false;
    }
    u32 difference(u32 a,u32 b){detail::NativeTu15ArithmeticEnvironment env(fp);u32 v;
        const auto first=p.r32(a);asm volatile("flds %0"::"m"(first):"memory","st");
        const auto second=p.r32(b);asm volatile("fsubs %1\n\tfstps %0":"=m"(v):"m"(second):"memory","st");return v;}
    bool cross_plane(u32 direction,u32 axis,u32 f){
        detail::NativeTu15ArithmeticEnvironment env(fp);
        auto load=[&](u32 at){auto v=p.r32(at);asm volatile("flds %0"::"m"(v):"memory","st");};
        auto mul=[&](u32 at){auto v=p.r32(at);asm volatile("fmuls %0"::"m"(v):"memory","st");};
        u32 x,y;
        load(direction+8);mul(axis+4);load(axis+8);mul(direction+4);
        asm volatile("fsubrp %%st,%%st(1)\n\tfstps %0":"=m"(x)::"memory","st","st(1)");p.w32(0x27632f,f+0x40,x);
        load(direction);mul(axis+8);load(direction+8);mul(axis);
        asm volatile("fsubrp %%st,%%st(1)\n\tfstps %0":"=m"(y)::"memory","st","st(1)");p.w32(0x27633f,f+0x44,y);
        load(direction+4);mul(axis);load(direction);mul(axis+4);
        asm volatile("fsubrp %%st,%%st(1)\n\tfld %%st(0)\n\tfmulp %%st,%%st(1)":::"memory","st","st(1)");
        load(f+0x44);mul(f+0x44);asm volatile("faddp %%st,%%st(1)":::"memory","st","st(1)");
        load(f+0x40);mul(f+0x40);asm volatile("faddp %%st,%%st(1)\n\tfsqrt":::"memory","st","st(1)");
        const auto threshold=p.r64(0x46a6a0);unsigned char small;
        asm volatile("fldl %1\n\tfxch %%st(1)\n\tfcomip %%st(1),%%st\n\tsetbe %0\n\tfstp %%st(0)":"=qm"(small):"m"(threshold):"memory","cc","st","st(1)");return small;
    }
    // Compare a native extended difference against an authored DOUBLE.
    bool threshold(u32 at,u32 sub_at,u32 constant,bool reversed,u32 publish=0,u32 site=0){
        detail::NativeTu15ArithmeticEnvironment env(fp);auto value=p.r32(at);
        asm volatile("flds %0"::"m"(value):"memory","st");
        u32 held{};if(publish)held=p.r32(at);
        const auto rhs=p.r32(sub_at);asm volatile("fsubs %0"::"m"(rhs):"memory","st");
        if(publish)p.w32(site,publish,held);
        const auto c=p.r64(constant);unsigned char above;
        if(reversed)asm volatile("fldl %1\n\tfxch %%st(1)\n\tfcomip %%st(1),%%st\n\tseta %0\n\tfstp %%st(0)":"=qm"(above):"m"(c):"memory","cc","st","st(1)");
        else asm volatile("fldl %1\n\tfcomip %%st(1),%%st\n\tseta %0\n\tfstp %%st(0)":"=qm"(above):"m"(c):"memory","cc","st","st(1)");
        return above;
    }
    void traverse();void segment();void clearance();void obstacle();void short_segment();
    void dispatch(u32 entry){switch(entry){
        case 0x2029e0:iterator();return;case 0x276090:contact();return;case 0x2762a0:traverse();return;
        case 0x2761f0:segment();return;case 0x2769c0:clearance();return;case 0x2cbaf0:obstacle();return;case 0x2768f0:short_segment();return;
        default:throw std::invalid_argument("Native route traversal received an unknown entry");}}
};
void Traversal::traverse(){
    const auto entry=abi.gp[4];const auto saved=abi.gp;abi.gp[4]-=0x64;
    auto initial_extra=p.r32(0x45695c),maximum=p.r32(entry+0x14);
    push(saved[3]);push(saved[5]);const auto navigation=p.r32(entry+4),out=saved[6];
    p.w32(0x2762be,out+0x20,initial_extra);auto previous=p.r32(0x4502c4);push(saved[7]);const auto f=abi.gp[4];
    p.w8(0x2762cc,f+0xe,0);p.w32(0x2762d1,f+0x10,saved[0]);p.w32(0x2762d5,f+0x18,0);
    p.w8(0x2762d9,out,0);p.w32(0x2762db,out+0x10,none);p.w32(0x2762de,out+0x14,0xffff);
    p.w32(0x2762e5,out+0x18,maximum);p.w8(0x2762ea,out+0x1c,0);p.w32(0x2762ed,f+0x2c,previous);
    auto fallback=[&](u32 direction){const auto region=p.r32(f+0x10),start=p.r32(entry+8);
        p.w8(0x27682c,out,0);p.w32(0x27682f,out+0x14,0xffff);p.w32(0x276836,out+0x10,region);p.w32(0x276839,out+0x18,maximum);
        endpoint(direction,start,maximum,out,{0x27684a,0x27685d,0x276876});result(entry,0x18,p.r8(f+0xe)!=0,saved);};
    auto exhausted=[&](u32 region){const auto direction=p.r32(entry+0x10),start=p.r32(entry+8);
        p.w32(0x276891,out+0x10,region);p.w8(0x276894,out,1);p.w32(0x276897,out+0x14,0xffff);p.w32(0x27689e,out+0x18,maximum);
        endpoint(direction,start,maximum,out,{0x2768af,0x2768c2,0x2768d9});result(entry,0x18,true,saved);};
    if(!navigation||saved[0]==p.r32(entry+0xc)||i32(saved[0])<0||i32(saved[0])>=i32(p.r32(navigation))){fallback(p.r32(entry+0x10));return;}
    auto direction=p.r32(entry+0x10);const auto axis=p.r32(0x474d10);
    if(cross_plane(direction,axis,f)){fallback(direction);return;}
    p.w32(0x276381,f+0x34,p.r32(f+0x40));auto plane_y=p.r32(f+0x44),plane_x=p.r32(f+0x34);p.w32(0x27638f,f+0x38,plane_y);
    const auto start=p.r32(entry+8);auto y=p.r32(start+4),x=p.r32(start);plane_y=p.r32(f+0x38);
    auto dot=s.add(s.mul(y,plane_y),s.mul(x,plane_x));p.w32(0x2763b2,f+0x3c,dot);
    u32 next_region=saved[0],previous_edge=0;
    for(;;){
        const auto previous_region=p.r32(f+0x10);p.w32(0x2763e4,f+0x30,previous_region);
        const auto sectors=p.r32(navigation+4);const auto flags=p.r8(sectors+next_region*8);
        p.w8(0x2763f1,f+0xf,1);p.w32(0x2763f6,f+0x10,next_region);p.w32(0x2763fa,f+0x28,none);
        if(!(flags&0x10)&&!p.r8(out+0x1c)&&(!previous_edge||(p.r8(previous_edge+5)&2))){
            const auto param=p.r32(f+0x2c);p.w8(0x276416,out+0x1c,1);p.w32(0x27641a,out+0x20,param);}
        if(next_region==p.r32(entry+0xc)){fallback(direction);return;}
        const auto region=p.r32(f+0x10),sector=p.r32(navigation+4)+region*8;const auto first=p.r32(sector+4);
        p.w16(0x276437,f+0x50,first);p.w16(0x27643c,f+0x4e,first);const auto links=p.r32(navigation+0x38);
        p.w32(0x276446,f+0x54,region);p.w16(0x27644a,f+0x4c,0xffff);p.w32(0x276451,f+0x5c,none);p.w32(0x276455,f+0x58,none);p.w32(0x276459,f+0x68,navigation);
        const auto link=i32(links)>0?u32(sx16(p.r16(sector+2))):none;p.w32(0x276467,f+0x60,link);p.w16(0x27646b,f+0x6c,0);
        abi.gp[1]=f+0x4c;call(0x276472,0x2029e0);auto edge=abi.gp[0];p.w32(0x27647b,f+0x14,edge);
        if(!edge){exhausted(region);return;}
        bool accepted=false;u32 child_start{};
        for(;;){
            const auto current_link=p.r32(f+0x5c),reverse=p.r8(f+0x64);u32 first_point,next_point,side_a,weighted_x,next_y;
            if(current_link!=none){const auto vertex=p.r32(f+0x58),vertices=p.r32(navigation+0x2c);next_point=vertices+vertex*12;
                p.w32(0x2764ba,f+0x1c,next_point);first_point=vertices+p.r16(edge+(reverse?2:0))*12;
                auto ay=p.r32(first_point+4),ax=p.r32(first_point);ay=s.mul(ay,plane_y);ax=s.mul(ax,plane_x);
                weighted_x=s.mul(plane_x,p.r32(next_point));side_a=s.sub(s.add(ay,ax),dot);p.w32(0x2764ee,f+0x24,side_a);next_y=p.r32(next_point+4);
            }else{
                if(p.r8(f+0xf)){const auto vertex=p.r16(edge+(reverse?2:0));first_point=p.r32(navigation+0x2c)+vertex*12;
                    auto ay=p.r32(first_point+4),ax=p.r32(first_point);side_a=s.sub(s.add(s.mul(ay,plane_y),s.mul(ax,plane_x)),dot);p.w8(0x27653b,f+0xf,0);
                }else{side_a=p.r32(f+0x20);first_point=p.r32(f+0x1c);}
                const auto vertex=p.r32(f+0x58),vertices=p.r32(navigation+0x2c);next_point=vertices+vertex*12;
                weighted_x=s.mul(plane_x,p.r32(next_point));p.w32(0x27655e,f+0x24,side_a);next_y=p.r32(next_point+4);p.w32(0x276569,f+0x1c,next_point);
            }
            side_a=p.r32(f+0x24);push(out);push(next_point);push(first_point);
            const auto side_b=s.sub(s.add(s.mul(next_y,plane_y),weighted_x),dot);p.w32(0x276580,f+0x20,side_b);
            push(p.r32(f+0x20));const auto child_direction=p.r32(entry+0x10);push(side_a);const auto edge_index=p.r16(f+0x4c);
            push(f+0x28);const auto current=p.r32(f+0x10);push(child_direction);push(edge_index);child_start=p.r32(entry+8);push(current);
            abi.gp[1]=reverse;abi.gp[7]=child_start;abi.gp[2]=navigation;call(0x2765b1,0x276090);
            accepted=(abi.gp[0]&255)!=0;next_region=p.r32(f+0x28);p.w8(0x2765bc,entry+4,accepted);
            if(accepted&&!p.r8(out)&&next_region==p.r32(f+0x30)){p.w8(0x2765cd,entry+4,0);accepted=false;}
            if(accepted){
                if(threshold(out+0x18,f+0x2c,0x46a698,false)&&sx16(p.r16(f+0x18))>0){maximum=p.r32(entry+0x14);p.w8(0x276818,f+0xe,0);fallback(p.r32(entry+0x10));return;}
                if(threshold(out+0x18,entry+0x14,0x46a430,true,f+0x2c,0x27661e)){
                    const auto target=p.r32(entry+0xc);
                    if(target==none||target==p.r32(out+0x10)){
                        maximum=p.r32(entry+0x14);const auto d=p.r32(entry+0x10);p.w8(0x2767a6,out,0);p.w32(0x2767a9,out+0x14,0xffff);p.w32(0x2767b0,out+0x18,maximum);
                        endpoint(d,child_start,maximum,out,{0x2767c1,0x2767d4,0x2767eb});result(entry,0x18,false,saved);return;}
                    if(next_region==target&&threshold(out+0x18,entry+0x14,0x46a2c0,false)){
                        maximum=p.r32(entry+0x14);const auto d=p.r32(entry+0x10);p.w32(0x2766de,out+0x10,next_region);p.w8(0x2766e1,out,0);p.w32(0x2766e4,out+0x14,0xffff);p.w32(0x2766eb,out+0x18,maximum);
                        endpoint(d,child_start,maximum,out,{0x2766fc,0x27670f,0x276726});result(entry,0x18,false,saved);return;}
                    const auto d=p.r32(entry+0x10);p.w32(0x27673c,out+0x18,0);maximum=p.r32(entry+0x14);p.w8(0x27674a,out,1);p.w32(0x27674d,out+0x14,0xffff);
                    endpoint(d,child_start,maximum,out,{0x276760,0x276773,0x27678a});result(entry,0x18,true,saved);return;
                }
                if(p.r8(out)){result(entry,0x18,true,saved);return;}
                const auto list=p.r32(entry+0x18);if(list){const auto current=p.r32(f+0x14);if(p.r8(current+4)&0x80){
                    if(excluded(list,p.r16(f+0x4c))){p.w8(0x276806,out,1);result(entry,0x18,true,saved);return;}
                    accepted=p.r8(entry+4)!=0;}}
                if(sx16(p.r16(f+0x18))>200){maximum=p.r32(entry+0x14);p.w8(0x276818,f+0xe,0);fallback(p.r32(entry+0x10));return;}
                p.w32(0x276674,f+0x18,p.r32(f+0x18)+1);
                break;
            }
            abi.gp[1]=f+0x4c;call(0x2765d6,0x2029e0);edge=abi.gp[0];p.w32(0x2765dd,f+0x14,edge);
            if(!edge){accepted=p.r8(entry+4)!=0;break;}
            dot=p.r32(f+0x3c);plane_y=p.r32(f+0x38);plane_x=p.r32(f+0x34);edge=p.r32(f+0x14);
        }
        maximum=p.r32(entry+0x14);if(!accepted){exhausted(p.r32(f+0x10));return;}
        direction=p.r32(entry+0x10);if(next_region==none){fallback(direction);return;}
        dot=p.r32(f+0x3c);plane_y=p.r32(f+0x38);plane_x=p.r32(f+0x34);previous_edge=p.r32(f+0x14);
    }
}
void Traversal::segment(){
    const auto entry=abi.gp[4];const auto saved=abi.gp;abi.gp[4]-=0x10;const auto f=abi.gp[4];
    // The zero Z publication lies between the native FLD/FSUB and FSTP.
    {
        detail::NativeTu15ArithmeticEnvironment env(fp);const auto end_x=p.r32(saved[1]);
        asm volatile("flds %0"::"m"(end_x):"memory","st");const auto start_x=p.r32(saved[0]);
        asm volatile("fsubs %0"::"m"(start_x):"memory","st");p.w32(0x2761fa,f+0xc,0);
        u32 delta;asm volatile("fstps %0":"=m"(delta)::"memory","st");p.w32(0x276200,f+4,delta);
    }
    p.w32(0x27620a,f+8,difference(saved[1]+4,saved[0]+4));
    bool small;
    {
        detail::NativeTu15ArithmeticEnvironment env(fp);auto y=p.r32(f+8);
        asm volatile("flds %0"::"m"(y):"memory","st");y=p.r32(f+8);asm volatile("fmuls %0"::"m"(y):"memory","st");
        auto x=p.r32(f+4);asm volatile("flds %0"::"m"(x):"memory","st");x=p.r32(f+4);
        u32 length;asm volatile("fmuls %1\n\tfaddp %%st,%%st(1)\n\tfsqrt\n\tfsts %0":"=m"(length):"m"(x):"memory","st","st(1)");p.w32(0x276222,f,length);
        asm volatile("fabs":::"memory","st");const auto eps=p.r32(0x46a0ec);unsigned char above;
        asm volatile("flds %1\n\tfcomip %%st(1),%%st\n\tseta %0\n\tfstp %%st(0)":"=qm"(above):"m"(eps):"memory","cc","st","st(1)");small=above;
    }
    if(small)p.w32(0x27626b,f,0);
    else{const auto inv=s.div(p.r32(0x46a0d0),p.r32(f));p.w32(0x276249,f+4,s.mul(inv,p.r32(f+4)));
        const auto y=s.mul(p.r32(f+8),inv),z=s.mul(inv,0);p.w32(0x27625d,f+8,y);p.w32(0x276263,f+0xc,z);}
    const auto exclusions=p.r32(entry+0x10),maximum=p.r32(f);push(exclusions);push(maximum);
    const auto target=p.r32(entry+0xc);push(f+4);push(target);push(saved[0]);const auto navigation=p.r32(entry+4);push(navigation);
    abi.gp[0]=p.r32(entry+8);call(0x27628d,0x2762a0);const bool yes=(abi.gp[0]&255)!=0;result(entry,0x10,yes,saved);
}
void Traversal::short_segment(){
    const auto entry=abi.gp[4];const auto saved=abi.gp;abi.gp[4]-=0x34;const auto f=abi.gp[4];
    p.w32(0x2768f7,f+4,difference(saved[1],saved[0]));p.w32(0x276901,f+8,difference(saved[1]+4,saved[0]+4));
    bool small;
    {
        detail::NativeTu15ArithmeticEnvironment env(fp);auto end=p.r32(saved[1]+8);asm volatile("flds %0"::"m"(end):"memory","st");
        auto start=p.r32(saved[0]+8);u32 z;asm volatile("fsubs %1\n\tfsts %0":"=m"(z):"m"(start):"memory","st");p.w32(0x27690b,f+0xc,z);
        z=p.r32(f+0xc);asm volatile("fmuls %0"::"m"(z):"memory","st");auto y=p.r32(f+8);asm volatile("flds %0"::"m"(y):"memory","st");y=p.r32(f+8);
        asm volatile("fmuls %0\n\tfaddp %%st,%%st(1)"::"m"(y):"memory","st","st(1)");auto x=p.r32(f+4);asm volatile("flds %0"::"m"(x):"memory","st");x=p.r32(f+4);
        u32 length;asm volatile("fmuls %1\n\tfaddp %%st,%%st(1)\n\tfsqrt\n\tfsts %0":"=m"(length):"m"(x):"memory","st","st(1)");p.w32(0x276929,f,length);
        asm volatile("fabs":::"memory","st");const auto eps=p.r32(0x46a0ec);unsigned char above;
        asm volatile("flds %1\n\tfcomip %%st(1),%%st\n\tseta %0\n\tfstp %%st(0)":"=qm"(above):"m"(eps):"memory","cc","st","st(1)");small=above;
    }
    if(!small){const auto length=p.r32(f);const bool long_enough=s.above(length,p.r32(0x46a0e8));const auto inverse=s.div(p.r32(0x46a0d0),length);
        p.w32(0x27695b,f+4,s.mul(inverse,p.r32(f+4)));p.w32(0x27696b,f+8,s.mul(p.r32(f+8),inverse));p.w32(0x27697b,f+0xc,s.mul(p.r32(f+0xc),inverse));
        if(long_enough){const auto exclusions=p.r32(entry+0x10),maximum=p.r32(f);push(saved[6]);push(exclusions);push(maximum);
            const auto target=p.r32(entry+0xc);push(f+4);push(target);push(saved[0]);const auto nav=p.r32(entry+4);push(nav);abi.gp[0]=p.r32(entry+8);abi.gp[6]=f+0x10;
            call(0x2769a5,0x2762a0);result(entry,0x10,(abi.gp[0]&255)!=0,saved);return;}}
    result(entry,0x10,false,saved);
}
void Traversal::obstacle(){
    const auto entry=abi.gp[4];const auto saved=abi.gp;auto maximum=p.r32(entry+0x10);push(saved[5]);const auto start=p.r32(entry+8);push(saved[6]);
    const auto f=abi.gp[4],out=saved[1],obstacles=saved[7],direction=saved[3];
    p.w32(0x2cbb01,out,maximum);p.w16(0x2cbb05,out+4,0xffff);p.w16(0x2cbb09,out+6,0xffff);
    if(sx16(p.r16(obstacles+2))>0){const auto radius=p.r32(entry+0xc);u32 index=0;
        do{if(u16(index)!=p.r16(entry+4)){
            const auto row=obstacles+u32(sx16(index))*20+0xc;
            if(!(saved[2]&255)||!(p.r8(row)&1)){
                auto x=p.r32(row+8),y=p.r32(row+0xc);x=s.sub(x,p.r32(start));y=s.sub(y,p.r32(start+4));
                auto dy=p.r32(direction+4),dx=p.r32(direction),r=p.r32(row+0x10);auto projected=s.add(s.mul(dy,y),s.mul(dx,x));
                const bool forward=s.above(projected,0);r=s.add(r,radius);p.w32(0x2cbb77,entry+0x10,projected);
                if(forward){auto yy=s.mul(y,y);x=s.mul(x,x);auto rr=s.mul(r,r);const auto separation=s.sub(s.add(x,yy),rr);u32 hit=0;bool intersects=true;
                    if(below(0,separation)){const auto disc=s.sub(s.mul(projected,projected),separation);intersects=!below(disc,0);p.w32(0x2cbbae,entry+8,disc);
                        if(intersects){detail::NativeTu15ArithmeticEnvironment env(fp);const auto d=p.r32(entry+8);asm volatile("flds %0\n\tfsqrt"::"m"(d):"memory","st");
                            const auto proj=p.r32(entry+0x10);asm volatile("fsubrs %1\n\tfstps %0":"=m"(hit):"m"(proj):"memory","st");p.w32(0x2cbbc0,entry+0xc,hit);hit=p.r32(entry+0xc);}}
                    if(intersects&&s.above(p.r32(out),hit)){p.w32(0x2cbbd3,out,hit);p.w16(0x2cbbd7,out+4,index);p.w16(0x2cbbdf,out+6,p.r16(row+2));}
                }
            }
        }++index;}while(sx16(index)<sx16(p.r16(obstacles+2)));
    }
    (void)f;result(entry,0x10,p.r16(out+4)!=0xffff,saved);
}
void Traversal::clearance(){
    const auto entry=abi.gp[4];const auto saved=abi.gp;abi.gp[4]-=0xa4;const auto sector=p.r32(entry+0x14),maximum=p.r32(entry+0x24);push(saved[5]);
    const auto out=p.r32(entry+0x38);p.w8(0x2769e3,entry-0xa1,0);
    p.w32(0x2769e8,out,maximum);p.w32(0x2769ed,out+4,none);p.w32(0x2769f0,out+8,0xffff);p.w16(0x2769f7,out+0xc,0xffff);p.w16(0x2769fb,out+0xe,0xffff);
    if(sector==none||sector==0xffff){result(entry,0x38,true,saved);return;}
    const auto adjust=p.r8(entry+0x2c);const auto threshold_value=adjust?s.sub(maximum,p.r32(entry+0x20)):maximum;
    p.w32(adjust?0x276a27:0x276a2f,entry-0xa0,threshold_value);
    const auto start=p.r32(entry+0x10);const auto x=p.r32(start),y=p.r32(start+4),direction=p.r32(entry+0x1c),dx=p.r32(direction),dy=p.r32(direction+4);
    const auto skip_nav=p.r8(entry+0x28);push(saved[3]);push(saved[6]);push(saved[7]);const auto f=abi.gp[4],navigation=p.r32(entry+4);
    p.w32(0x276a6b,f+0x24,x);p.w32(0x276a71,f+0x28,y);p.w32(0x276a77,f+0x2c,0);
    p.w32(0x276a7d,f+0x30,dx);p.w32(0x276a83,f+0x34,dy);p.w32(0x276a89,f+0x38,0);
    auto traverse_at=[&](u32 site,u32 from,u32 origin,u32 target,u32 vector,u32 extent,u32 result_pointer,u32 exclusions){
        push(exclusions);push(extent);push(vector);push(target);push(origin);push(navigation);abi.gp[0]=from;abi.gp[6]=result_pointer;call(site,0x2762a0);return (abi.gp[0]&255)!=0;};
    const auto exclusions=p.r32(entry+0x34);
    if(!skip_nav){const auto sectors=p.r32(navigation+4);if(!(p.r8(sectors+sector*8)&0x10)){result(entry,0x38,true,saved);return;}
        const auto extent=p.r32(entry+0x24),target=p.r32(entry+0x18),from=p.r32(entry+0x14);
        auto ex=s.add(s.mul(dx,maximum),x),ey=s.add(s.mul(dy,maximum),y),ez=s.mul(maximum,0);
        p.w32(0x276af2,f+0x3c,ex);p.w32(0x276af8,f+0x40,ey);p.w32(0x276afe,f+0x44,ez);
        if(traverse_at(0x276b04,from,f+0x24,target,f+0x30,extent,f+0x6c,exclusions)){
            const auto distance=p.r32(f+0x84);bool hit=s.above(p.r32(f+0x14),distance);
            if(!hit){const auto desired=p.r32(entry+0x18);hit=desired!=none&&p.r32(f+0x7c)!=desired;}
            if(hit){const auto edge=p.r32(f+0x80);p.w8(0x276b3a,f+0x13,1);p.w32(0x276b3f,out,distance);p.w32(0x276b44,out+8,edge);}
        }
        for(unsigned side=0;side<2;++side){
            const auto original_direction=p.r32(entry+0x1c),radius=p.r32(entry+0x20);
            if(side==0){const auto perpendicular=s.sub(0,p.r32(original_direction+4));p.w32(0x276b68,f+0x18,perpendicular);
                const auto py=p.r32(original_direction);p.w32(0x276b88,f+0x1c,py);p.w32(0x276b8e,f+0x20,0);
            }else{const auto minus=p.r32(0x46a0dc);p.w32(0x276c3d,f+0x18,s.mul(p.r32(f+0x18),minus));
                p.w32(0x276c5b,f+0x1c,s.mul(p.r32(f+0x1c),minus));p.w32(0x276c71,f+0x20,p.r32(0x46a838));}
            (void)traverse_at(side?0x276c77:0x276b94,p.r32(entry+0x14),f+0x24,none,f+0x18,radius,f+0x90,0);
            const auto max_now=p.r32(out);const auto from=p.r32(f+0xa0);
            if(traverse_at(side?0x276c9c:0x276bb9,from,f+0x94,none,f+0x30,max_now,f+0x48,exclusions)){
                const auto distance=p.r32(f+0x60);bool hit=s.above(p.r32(out),distance)&&s.above(p.r32(f+0x14),distance);
                if(hit&&!p.r8(f+0x6c)){
                    const auto target=p.r32(entry+0x18),edge_from=p.r32(f+0x58);push(exclusions);push(target);push(edge_from);push(navigation);
                    abi.gp[1]=f+0x3c;abi.gp[0]=f+0x4c;call(side?0x276cdf:0x276bfc,0x2768f0);hit=(abi.gp[0]&255)!=0;
                }
                if(hit){const auto edge=p.r32(f+0x5c);p.w8(side?0x276cf2:0x276c0f,f+0x13,1);p.w32(side?0x276cf7:0x276c14,out,p.r32(f+0x60));p.w32(side?0x276cfc:0x276c19,out+8,edge);}
            }
        }
    }
    const auto obstacles=p.r32(entry+8);
    if(obstacles){const auto extent=p.r32(entry+0x24),radius=p.r32(entry+0x20),direction_now=p.r32(entry+0x1c),start_now=p.r32(entry+0x10),ignore=p.r32(entry+0xc);
        push(extent);push(radius);push(start_now);push(ignore);abi.gp[2]=p.r8(entry+0x30);abi.gp[1]=f+0x18;abi.gp[3]=direction_now;abi.gp[7]=obstacles;
        call(0x276d4b,0x2cbaf0);
        if((abi.gp[0]&255)&&s.above(p.r32(out),p.r32(f+0x18))&&s.above(p.r32(f+0x14),p.r32(f+0x18))){
            const auto index=p.r16(f+0x1c),kind=p.r16(f+0x1e);p.w8(0x276d79,f+0x13,1);p.w32(0x276d7e,out,p.r32(f+0x18));
            p.w32(0x276d83,out+8,0xffff);p.w16(0x276d8a,out+0xc,index);p.w16(0x276d8e,out+0xe,kind);}
    }
    (void)traverse_at(0x276dc6,p.r32(entry+0x14),f+0x24,p.r32(entry+0x18),f+0x30,p.r32(out),f+0x6c,p.r32(entry+0x34));
    p.w32(0x276dd1,out+4,p.r32(f+0x7c));result(entry,0x38,p.r8(f+0x13)!=0,saved);
}
}
#endif
void native_campaign_ai_route_traversal_call_tu15(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,const NativeCampaignAiPerformServicesTu15 &services,const SourcePhysicsContactRecordObserver &observe){
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
    if(!abi.floating_point||abi.x87_depth)throw std::invalid_argument("Native traversal requires FP controls and empty x87");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    struct Publish{SourcePhysicsWorldRuntimeAbi&a;NativeTu15PhysicsFrameFp&f;~Publish(){a.floating_point=SourcePhysicsFactoryFloatingPoint{f.control_word,f.mxcsr};a.x87_status=f.status;}}publish{abi,fp};
    Traversal{{memory,observe},abi,fp,Simd(fp),services}.dispatch(entry);
#else
    (void)memory;(void)entry;(void)abi;(void)services;(void)observe;
    throw std::runtime_error("Native traversal requires its qualified host arithmetic");
#endif
}
}
