#include "halo2/native_campaign_ai_route_obstacles_tu15.hpp"
#include "halo2/native_campaign_ai_route_obstacle_geometry_tu15.hpp"
#include "halo2/native_campaign_script_builtins_tu15.hpp"
#include "halo2/native_campaign_ai_perception_lookup_tu15.hpp"
#include "halo2/native_campaign_ai_perception_group_initialize_tu15.hpp"
#include "halo2/native_campaign_ai_vector_math_tu15.hpp"
#include "halo2/native_campaign_ai_point_geometry_tu15.hpp"
#include "halo2/native_campaign_object_markers_tu15.hpp"
#include "halo2/native_campaign_object_world_point_tu15.hpp"
#include "halo2/native_object_factory_deviceanim_tu15.hpp"
#include "halo2/native_object_factory_nodes_tu15.hpp"
#include "halo2/native_tu15_object_deletion.hpp"
#include "halo2/native_tu15_data_array_iteration.hpp"
#include "native_campaign_ai_goal_port_tu15.hpp"

namespace halo2 {
namespace {
using namespace goal_tu15;
constexpr u32 none=0xffffffffu;
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
struct Obstacles {
    Port p;SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;Simd s{fp};
    u32 frame{},entry{},actor{},policy{},mask{},primary{},secondary{},start{},extent{},direction{},ignore0{},ignore1{};
    u32 actor_record{},actor_body{};bool speed_low=true,speed_high=true,attached{};
    i16 step{},jump{},climb_min{},climb_max{};
    void push(u32 value){abi.gp[4]-=4;std::array<std::byte,4>raw{};for(unsigned n=0;n<4;++n)raw[n]=std::byte(value>>(8*n));p.memory.write(abi.gp[4],raw);}
    u32 body(u32 object){const auto pool=p.r32(0x4ecbd0u),rows=p.r32(pool+0x44u);return p.r32(rows+(object&65535u)*12u+8u);}
    u32 actor_row(u32 datum){return p.r32(p.r32(0x5020c8u)+0x44u)+(datum&65535u)*0x888u;}
    u32 tag(u32 datum,u32 tags){return p.r32(tags+(datum&65535u)*16u+8u);}
    bool below(u32 a,u32 b){return !s.above_equal(a,b);}
    bool equal(u32 a,u32 b) {
        auto x=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<i32>(a))),y=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<i32>(b)));
        u32 host{},csr=fp.mxcsr;unsigned char eq{},ordered{};
        asm volatile("stmxcsr %2;ldmxcsr %3;ucomiss %5,%4;sete %0;setnp %1;stmxcsr %3;ldmxcsr %2"
            :"=q"(eq),"=q"(ordered),"+m"(host),"+m"(csr):"x"(x),"x"(y):"memory","cc");fp.mxcsr=csr;return eq&&ordered;
    }
    bool relation(u32 first,u32 second){return native_campaign_ai_perception_team_relation_tu15(p.memory,i16(first),i16(second));}
    void semantic_store(u32 address,std::span<const std::byte> bytes) {
        // The shared script spatial owner exposes publication widths/order,
        // but no instruction PC. Keep that provenance explicitly anonymous.
        SourcePhysicsContactRecordStore event{0,address,{},std::vector<std::byte>(bytes.begin(),bytes.end())};
        if(p.observe){event.before.resize(bytes.size());p.memory.read(address,event.before);}
        p.memory.write(address,bytes);if(p.observe)p.observe(event);
    }
    std::vector<u32> sphere();
    void world_matrix(u32 object,u32 destination) {
        const auto pool=p.r32(0x4ecbd0u),rows=p.r32(pool+0x44u),object_body=p.r32(rows+(object&65535u)*12u+8u);
        native_object_factory_tu15::device_animation::basis_matrix(p.memory,destination,object_body+0x64u,object_body+0x70u,object_body+0x7cu,
            {fp.control_word,fp.mxcsr});
        const auto parent=p.r32(object_body+0x14u);
        if(parent!=none) {
            const auto parent_rows=p.r32(pool+0x44u),parent_body=p.r32(parent_rows+(parent&65535u)*12u+8u);
            const auto node=i32(std::int8_t(p.r8(object_body+0x18u))),offset=sx16(p.r16(parent_body+0x116u));
            native_campaign_object_transform_product_tu15(p.memory,parent_body+u32(node*52)+u32(offset),destination,destination,fp);
        }
    }
    u32 dot(u32 a,u32 b) {
        auto z=p.r32(a+8u);z=s.mul(z,p.r32(b+8u));auto y=p.r32(a+4u);y=s.mul(y,p.r32(b+4u));z=s.add(z,y);
        auto x=p.r32(a);x=s.mul(x,p.r32(b));return s.add(z,x);
    }
    bool facing_away(u32 target_actor);
    bool marker_alignment(u32 record,bool reverse_last);
    void geometry(u32 owner) {
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
        const auto reload=[&]{fp.control_word=abi.floating_point->x87_control_word;fp.mxcsr=abi.floating_point->mxcsr;fp.status=abi.x87_status;};
        try{native_campaign_ai_route_obstacle_geometry_call_tu15(p.memory,owner,abi,p.observe);reload();}
        catch(...){reload();throw;}
    }
    u32 markers(u32 site,u32 owner,u32 object,u32 output) {
        const auto name=p.r16(policy+4u);abi.gp[4]=frame;
        push(name);push(output);push(object);abi.gp[0]=32;push(site+5u);
        geometry(owner);
        return u32(sx16(abi.gp[0]));
    }
    bool marker_valid(u32 site,u32 record) {
        abi.gp[1]=record;abi.gp[4]=frame;push(site+5u);
        geometry(0x279600u);return (abi.gp[0]&255u)!=0;
    }
    void classify_markers(u32 object,u32 &flags,bool &special);
    void append(u32 object,u32 radius,u32 flags,u32 output) {
        abi.gp[4]=frame;push(radius);push(object);abi.gp[2]=output;abi.gp[3]=flags;abi.gp[6]=frame+0x34u;push(0x2cba30u);
        geometry(0x2cab80u);
    }
    void collect_object(u32 object);
    void run();
};

std::vector<u32> Obstacles::sphere() {
    NativeCampaignScriptBank bank(p.memory,[&](u32 address,std::span<const std::byte> bytes){semantic_store(address,bytes);},
        {fp.control_word,fp.mxcsr});
    native_campaign_script_tu15::NativeCampaignLocation location;
    if(ignore0!=none) {
        const auto object_body=body(ignore0),a=p.r32(object_body+0x28u),b=p.r32(object_body+0x2cu);
        location={a,u16(b),u16(b>>16u)};
    } else {
        const auto up=p.r32(0x474d10u),offset=p.r32(0x46a12cu);
        std::array<u32,3> point{};
        for(unsigned n=0;n<3;++n){const auto component=p.r32(up+n*4u);point[n]=s.add(s.mul(component,offset),p.r32(start+n*4u));}
        location=native_campaign_script_tu15::scenario_location(bank,point);
    }
    const std::array<u32,3> center{p.r32(start),p.r32(start+4u),p.r32(start+8u)};
    return native_campaign_script_tu15::objects_in_sphere(bank,1,0x8c3u,location,center,extent,128,frame+0xbcu);
}

bool Obstacles::facing_away(u32 other) {
    const auto point=frame+0x70u,delta=frame+0x64u;
    native_campaign_object_world_point_tu15(p.memory,p.r32(frame+0x1cu),point,fp,p.observe);
    for(unsigned n=0;n<3;++n)p.w32(std::array<u32,3>{0x2caf76u,0x2caf87u,0x2caf9cu}[n],delta+n*4u,s.sub(p.r32(point+n*4u),p.r32(start+n*4u)));
    const auto length=native_campaign_ai_normalize_vector_tu15(p.memory,delta,fp,p.observe);
    if(!s.above(length,p.r32(0x46a0e8u)))return false;
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    const auto load=[&](u32 address){const auto v=p.r32(address);asm volatile("flds %0"::"m"(v):"memory","st");};
    const auto mul=[&](u32 address){const auto v=p.r32(address);asm volatile("fmuls %0"::"m"(v):"memory","st");};
    const auto aim=other+0x5ecu;
    load(delta+8u);mul(aim+8u);load(delta+4u);mul(aim+4u);asm volatile("faddp":::"memory","st","st(1)");
    load(delta);mul(aim);u32 alignment{};asm volatile("faddp;fstps %0":"=m"(alignment)::"memory","st","st(1)");
    p.w32(0x2cafceu,frame+0x30u,alignment);
    // Actual24570 extended vector magnitude, with the source load/stack order.
    load(aim+8u);load(aim+4u);load(aim);
    asm volatile("fld %%st(0);fmul %%st(1),%%st;fld %%st(2);fmul %%st(3),%%st;faddp;fld %%st(3);fmul %%st(4),%%st;faddp;fsqrt;fstp %%st(3);fstp %%st(0);fstp %%st(0)":::"memory","st","st(1)","st(2)","st(3)","st(4)");
    mul(0x46a794u);load(frame+0x30u);unsigned char above{};
    asm volatile("fcomip %%st(1),%%st;fstp %%st(0);seta %0":"=q"(above)::"memory","cc","st","st(1)");return above!=0;
}

bool Obstacles::marker_alignment(u32 record,bool reverse_last) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    const auto load=[&](u32 address){const auto v=p.r32(address);asm volatile("flds %0"::"m"(v):"memory","st");};
    const auto mul=[&](u32 address){const auto v=p.r32(address);asm volatile("fmuls %0"::"m"(v):"memory","st");};
    load(record+0x50u);mul(direction+8u);load(record+0x4cu);mul(direction+4u);asm volatile("faddp":::"memory","st","st(1)");
    load(reverse_last?direction:record+0x48u);mul(reverse_last?record+0x48u:direction);
    asm volatile("faddp;fabs":::"memory","st","st(1)");load(0x46a194u);unsigned char above_equal{};
    asm volatile("fxch %%st(1);fcomip %%st(1),%%st;fstp %%st(0);setae %0":"=q"(above_equal)::"memory","cc","st","st(1)");return above_equal!=0;
}

void Obstacles::classify_markers(u32 object,u32 &flags,bool &special) {
    if((mask&8u)&&speed_low) {
        const auto buffer=frame+0x10bcu;const auto count=sx16(markers(0x2cb38bu,0x279640u,object,buffer));
        for(i32 n=0;n<count;++n){const auto record=buffer+u32(n)*0x70u;if(!marker_valid(0x2cb3bdu,record))continue;
            auto z=s.sub(p.r32(start+8u),p.r32(record+0x68u));auto a=p.r32(record+0x5cu);
            auto y=p.r32(start+4u),x=p.r32(start);y=s.sub(y,p.r32(record+0x64u));x=s.sub(x,p.r32(record+0x60u));
            x=s.mul(x,p.r32(record+0x54u));a=s.mul(a,z);z=s.mul(p.r32(record+0x58u),y);a=s.add(s.add(a,z),x);
            if(below(a,p.r32(0x46a3f4u)))continue;
            const auto limit=s.add(p.r32(record+0x6cu),p.r32(0x46a16cu));if(!s.above(limit,a))continue;
            if(marker_alignment(record,false)){flags|=0x40u;special=true;break;}
        }
    }
    if((mask&16u)&&speed_high) {
        const auto buffer=frame+0x2bcu;const auto count=sx16(markers(0x2cb493u,0x279900u,object,buffer));
        for(i32 n=0;n<count;++n){const auto record=buffer+u32(n)*0x70u;if(!marker_valid(0x2cb4c0u,record))continue;
            auto z=s.sub(p.r32(start+8u),p.r32(record+0x68u));auto a=p.r32(record+0x5cu);
            auto y=s.sub(p.r32(start+4u),p.r32(record+0x64u));auto x=s.sub(p.r32(start),p.r32(record+0x60u));
            a=s.mul(a,z);z=s.mul(p.r32(record+0x58u),y);y=s.mul(p.r32(record+0x54u),x);const auto margin=p.r32(0x46a178u);
            a=s.add(s.add(a,z),y);if(below(a,margin))continue;
            const auto limit=s.sub(p.r32(record+0x6cu),margin);if(!s.above(limit,a))continue;
            if(marker_alignment(record,true)){flags|=0x100u;special=true;break;}
        }
    }
    if((mask&32u)&&speed_high) {
        const auto buffer=frame+0x1ebcu;const auto count=sx16(markers(0x2cb599u,0x279b30u,object,buffer));
        for(i32 n=0;n<count;++n)if(marker_valid(0x2cb5bdu,buffer+u32(n)*0x70u)){flags|=0x80u;special=true;break;}
    }
}

void Obstacles::collect_object(u32 object) {
    const auto object_body=body(object);p.w32(0x2cae25u,frame+0x1cu,object);
    if(object==ignore0||object==ignore1)return;
    const auto object_flags=p.r32(object_body+4u);const bool opaque=(object_flags&0x400000u)!=0;
    if(!opaque&&(object_flags&0x800000u))return;
    if(native_tu15_object_or_ancestor_deleting(p.memory,object))return;
    if(actor_body && p.r8(actor_body+0x3dcu)==1 && object==p.r32(actor_body+0x448u))return;
    const auto kind=i32(std::int8_t(p.r8(object_body+0xaau)));bool hostile=false,device_open=false;u32 padding=0;
    if(kind==0) {
        if(p.r8(object_body+0x10au)&4u)return;
        if(actor!=none){const auto prop=native_campaign_ai_perception_lookup_tu15(p.memory,object,actor,false,fp,abi.x87_depth);
            if(prop!=none){const auto rows=p.r32(p.r32(0x50eef0u)+0x44u);if(sx16(p.r16(rows+(prop&65535u)*0x3cu+0x24u))<1)return;}}
        const auto other_actor=p.r32(object_body+0x12cu);
        if(other_actor!=none) {
            const auto other=actor_row(other_actor);bool allied=actor_record&&relation(p.r16(other+0x24u),p.r16(actor_record+0x24u));
            if(p.r8(other+0x5d0u) && (!actor_record||p.r32(actor_record+0x26cu)==none) && facing_away(other))return;
            if(actor_record) {
                if(p.r32(actor_record+0x26cu)!=none && !allied)padding=p.r32(0x46a0d0u);
                else if(allied&&p.r8(actor_record+0x223u))padding=p.r32(0x46a0ccu);
            }
            hostile=!allied;
        } else if(actor_record && p.r32(object_body+0x13cu)!=none) {
            const auto team=p.r16(actor_record+0x24u);const bool allied=relation(team,1);
            if(p.r32(actor_record+0x26cu)!=none && !allied)padding=p.r32(0x46a0d0u);
            else if(allied&&p.r8(actor_record+0x223u))padding=p.r32(0x46a0ccu);
            hostile=!allied;
        }
    } else if(kind==1) {
        if(actor_record&&p.r32(actor_record+0x26cu)==none){const auto team=u32(sx16(p.r16(object_body+0x138u))),own=p.r16(actor_record+0x24u);hostile=team==own||!relation(team,own);}
    } else if(kind==7) {
        const auto definition=p.r32(object_body),tags=p.r32(0x4f0414u),object_tag=tag(definition,tags),flags=p.r16(object_tag+0x11eu);
        if(!(flags&1u))device_open=true;
        else if(flags&2u)device_open=equal(p.r32(object_body+0x140u),p.r32(0x46a0d0u))||!(p.r8(object_body+0x1ccu)&1u);
    } else if(kind==11) {
        const auto physics=p.r32(object_body+0xb4u);if(physics!=none){const auto rows=p.r32(p.r32(0x52be5cu)+0x44u);if(i32(std::int8_t(p.r8(rows+(physics&65535u)*160u+0x1cu)))<=1)return;}
    }
    auto z=s.sub(p.r32(object_body+0x38u),p.r32(start+8u)),y=s.sub(p.r32(object_body+0x34u),p.r32(start+4u)),x=s.sub(p.r32(object_body+0x30u),p.r32(start));
    const auto radius=s.add(p.r32(object_body+0x3cu),extent);const auto distance=s.add(s.add(s.mul(z,z),s.mul(y,y)),s.mul(x,x));
    if(below(s.mul(radius,radius),distance))return;
    const auto definition=p.r32(object_body),tags=p.r32(0x4f0414u),object_tag=tag(definition,tags);if(p.r8(object_tag+2u)&8u)return;
    u32 flags=0,extra=0,shape=0;bool special=false;
    if(p.r32(policy+0x18u)!=object && !hostile) {
        const auto shape_count=p.r32(object_tag+0x5cu);i32 obstacle_class=0;bool usable=true;
        if(i32(shape_count)>0){shape=p.r32(object_tag+0x60u);if((p.r8(shape)&2u)&&!below(0,p.r32(object_body+0xecu)))usable=false;
            else obstacle_class=sx16(p.r16(shape+12u));}
        if(!obstacle_class){const auto physics=p.r32(object_body+0xb4u);if(physics!=none){const auto rows=p.r32(p.r32(0x52be5cu)+0x44u);obstacle_class=i32(std::int8_t(p.r8(rows+(physics&65535u)*160u+0x1cu)));}}
        if(!usable)return;
        if(obstacle_class>0){if(step>=obstacle_class)return;
            if(jump>=obstacle_class&&speed_high){flags=0x10u;special=true;}
            else if(climb_min<=obstacle_class&&climb_max>=obstacle_class&&p.r8(object_body+0xaau)!=0){flags=0x20u;special=true;}}
        classify_markers(object,flags,special);
        if(actor_record&&p.r32(actor_record+0x26cu)!=none&&(kind==0||kind==1)) {
            const auto point=frame+0x7cu;native_campaign_object_world_point_tu15(p.memory,object,point,fp,p.observe);
            auto a=s.sub(p.r32(point),p.r32(start)),b=s.sub(p.r32(point+8u),p.r32(start+8u));a=s.mul(a,p.r32(direction));b=s.mul(b,p.r32(direction+8u));
            auto c=s.sub(p.r32(point+4u),p.r32(start+4u));b=s.add(b,a);c=s.mul(p.r32(direction+4u),c);b=s.add(b,c);
            if(s.above(b,p.r32(0x46a0e8u))&&p.r32(object_body+0x248u)!=none) {
                const auto team=p.r16(object_body+0x138u),own=p.r16(actor_record+0x24u);
                if(team==own||!relation(team,own)) {
                    const auto rider_body=body(p.r32(object_body+0x248u)),rider_actor=p.r32(rider_body+0x12cu);bool toward=false;
                    if(rider_actor!=none){const auto rider=actor_row(rider_actor);if(p.r8(rider+0x5d0u))toward=s.above(dot(rider+0x5ecu,direction),p.r32(0x46a0e8u));}
                    if(!toward)toward=s.above(dot(object_body+0x88u,direction),p.r32(0x46a0e8u));
                    if(toward){flags|=0x200u;special=true;padding=p.r32(0x46a120u);}
                }
            }
        }
        if(shape&&(mask&0x40u)&&(p.r8(shape)&2u))extra=8u;
    }
    const auto model_datum=p.r32(object_tag+0x38u);if(model_datum==none)return;
    const auto model_tags=p.r32(0x4f0414u),model=tag(model_datum,model_tags),collision_datum=p.r32(model+12u);if(collision_datum==none)return;
    const auto collision=tag(collision_datum,model_tags);if(i32(p.r32(collision+0x24u))<=0)return;
    world_matrix(object,frame+0x88u);
    for(i32 n=0;n<i32(p.r32(collision+0x24u));n=sx16(u32(n)+1u)) {
        const auto spheres=p.r32(collision+0x28u),sphere=spheres+u32(n)*20u;
        if(device_open&&!(p.r8(sphere+2u)&1u))continue;
        if(opaque&&!(p.r8(sphere+2u)&4u))continue;
        if(!attached&&(p.r8(sphere+2u)&2u))continue;
        const auto node=p.r16(sphere);const auto matrix=node==0xffffu?frame+0x88u:native_object_factory_tu15::object_node_matrix(p.memory,object,node);
        native_campaign_ai_transform_point_tu15(p.memory,matrix,sphere+4u,frame+0x34u,fp,p.observe);
        auto r=s.mul(p.r32(sphere+16u),p.r32(matrix));auto z0=p.r32(frame+0x3cu);const auto one=p.r32(0x46a0d0u),start_z=p.r32(start+8u);
        if(s.above(start_z,s.add(s.add(z0,r),one))&&s.above(p.r32(direction+8u),p.r32(0x46a3f4u)))continue;
        if(s.above(s.sub(s.sub(z0,r),one),p.r32(start+8u))&&s.above(p.r32(0x46a16cu),p.r32(direction+8u)))continue;
        z=s.sub(z0,p.r32(start+8u));y=s.sub(p.r32(frame+0x38u),p.r32(start+4u));x=s.sub(p.r32(frame+0x34u),p.r32(start));
        auto squared=s.mul(s.mul(z,z),p.r32(0x46a120u));squared=s.add(s.add(squared,s.mul(y,y)),s.mul(x,x));auto reach=s.add(r,extent);
        if(below(s.mul(reach,reach),squared))continue;
        u32 moving=0;
        if(p.r8(object_body+0xaau)==0) {
            auto a=s.mul(p.r32(direction+4u),y),b=s.mul(z,p.r32(direction+8u)),c=p.r32(direction);a=s.add(a,b);c=s.mul(c,x);a=s.add(a,c);
            if(s.above(a,p.r32(0x46a0e8u))&&s.above(dot(object_body+0x88u,direction),p.r32(0x46a11cu)))moving=1;
        }
        const auto output=special?secondary:primary;if(!output)continue;
        append(object,s.add(r,padding),special?flags:(moving|flags|extra),output);
    }
}

void Obstacles::run() {
    entry=abi.gp[4];const auto saved=abi.gp;actor=p.r32(entry+4u);mask=p.r32(entry+8u);policy=p.r32(entry+12u);
    primary=p.r32(entry+16u);secondary=p.r32(entry+20u);start=p.r32(entry+24u);extent=p.r32(entry+28u);direction=p.r32(entry+32u);ignore0=p.r32(entry+36u);ignore1=p.r32(entry+40u);
    frame=entry-0x2cbcu;abi.gp[4]=frame;
    if(actor!=none) {
        actor_record=actor_row(actor);const auto unit=body(p.r32(actor_record+0x18u));
        if(saved[1]){const auto speed=dot(direction,saved[1]);speed_low=s.above(speed,p.r32(0x46a2bcu));speed_high=s.above(speed,p.r32(0x46a510u));}
        attached=p.r32(actor_record+0x26cu)!=none;if(p.r8(unit+0xaau)==0)actor_body=unit;
        if(policy){if(mask&1u)step=i16(p.r16(policy+14u));if(mask&4u)jump=i16(p.r16(policy+16u));if(mask&2u){climb_min=i16(p.r16(policy+18u));climb_max=i16(p.r16(policy+20u));}}
    }
    const auto candidates=sphere();for(const auto object:candidates)collect_object(object);
    for(auto n:{3u,5u,6u,7u})abi.gp[n]=saved[n];abi.gp[4]=entry+44u;
}
#endif
}
void native_campaign_ai_route_obstacles_call_tu15(SourcePhysicsContactMemory &memory,u32 entry,
    SourcePhysicsWorldRuntimeAbi &abi,const NativeCampaignAiPerformServicesTu15 &,
    const SourcePhysicsContactRecordObserver &observe) {
    if(entry!=0x2cabe0u)throw std::invalid_argument("Unknown native route obstacle owner");
    if(!abi.floating_point)throw std::invalid_argument("Native route obstacles require retained FP controls");
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    const auto finish=[&]{abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;};
    try{Obstacles{Port{memory,observe},abi,fp}.run();finish();}catch(...){finish();throw;}
#else
    throw std::runtime_error("Native route obstacles require x86 arithmetic");
#endif
}
}
