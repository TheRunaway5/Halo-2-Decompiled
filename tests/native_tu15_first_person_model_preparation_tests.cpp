#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_first_person_model_preparation.hpp"

using namespace frame_misc_harness;
namespace model=halo2::native_first_person_model_preparation;
namespace {
constexpr std::uint32_t stack=0x20010400u, view_record=0x10000000u, game_session=0x10100000u,
    representation_header=0x10200000u, representation=0x10300000u, object_header=0x10400000u,
    datums=0x10500000u, object_values=0x10600000u, tag_instances=0x10700000u,
    tag_data=0x10800000u, model_choice=0x10900000u, output=0x10a00000u, matrices=0x10b00000u;
constexpr std::uint32_t player_handle=0xa0030001u, first_weapon=0xb0020002u, second_weapon=0xc0010003u;

void child(WorldState &s,std::uint32_t target) {
    const auto sp=s.gp[4];
    auto hash=entry_hash(s,target);
    std::uint32_t args{};
    switch(target) {
    case 0xbac30u:case 0x145560u:args=1;break;
    case 0x16b4e8u:args=2;break;
    case 0x168d76u:args=5;break;
    default:break;
    }
    for(unsigned i=0;i<=args;++i)hash=mix(hash,r(s.memory,sp+4u*i));
    log_call(hash);
    switch(target) {
    case 0xbac30u:
        w(s.memory,s.observe,target,r(s.memory,sp+4u),4,matrices);
        s.gp[0]=6u;
        break;
    case 0x144bf0u:case 0x144090u:case 0x145560u: {
        const auto dest=target==0x144bf0u?s.gp[2]:(target==0x144090u?s.gp[0]:r(s.memory,sp+4u));
        std::mt19937_64 values(hash);
        for(unsigned i=0;i<13u;++i)w(s.memory,s.observe,target,dest+4u*i,4,std::uint32_t(values()));
        break;
    }
    case 0x11e060u:s.gp[0]=(policy&4u)?0u:model_choice;break;
    case 0x168d76u: {
        const auto dest=r(s.memory,sp+20u);
        w(s.memory,s.observe,target,dest,4,s.gp[0]);
        w(s.memory,s.observe,target,dest+4u,4,r(s.memory,sp+4u));
        w(s.memory,s.observe,target,dest+8u,4,r(s.memory,sp+8u));
        break;
    }
    case 0x16b4e8u:
        w(s.memory,s.observe,target,r(s.memory,sp+8u)+12u,4,std::uint32_t(hash));
        break;
    default:throw std::logic_error("Unexpected model-preparation child");
    }
    s.flags.load(std::uint32_t(hash)&0x8d5u);
    s.gp[4]+=4u+args*4u;
}
}

namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_bac30(WorldState &s){child(s,0xbac30u);}
static void run_168d76(WorldState &s){child(s,0x168d76u);}
static void run_16b4e8(WorldState &s){child(s,0x16b4e8u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail) {
    if(tail||r(s.memory,s.gp[4])!=site+5u)throw std::logic_error("Incorrect child dispatch");
    child(s,target);
}
#include "body.inc"
#include "reference.inc"
}

namespace {
Case author(std::mt19937 &rng,unsigned i) {
    Case c;
    auto &m=c.memory;
    const auto shape=i%16u;
    for(std::uint32_t address=stack-0x300u;address<=stack+0x30u;address+=4u)m.put(address,std::uint32_t(rng()));
    for(std::uint32_t offset=0;offset<0x20ccu;offset+=4u)m.put(view_record+offset,0u);
    for(std::uint32_t offset=0;offset<4u*0xd0cu;offset+=4u)m.put(output+offset,0u);
    random_abi(rng,c,stack);
    m.put(stack,0x10196u);
    m.put(stack+4u,shape==2u?0xffffffffu:0u);
    m.put(stack+8u,player_handle);
    m.put(stack+12u,shape==8u?2u:4u);
    m.put(stack+16u,output);

    m.put(model::session,game_session);m.put(game_session+8u,shape==1u?1u:2u);
    m.put(model::view_mask,3u);
    m.put(model::first_person_views,view_record);
    m.put(view_record,shape==5u?0u:4u);
    m.put(view_record+4u,shape==3u?0xffffffffu:player_handle);
    m.put(view_record+8u,shape==4u?0xffffffffu:0u);
    m.put(view_record+12u,shape==0u?0u:7u);
    m.put(view_record+16u,first_weapon);
    m.put(view_record+0x1010u+12u,shape==0u?0u:3u);
    m.put(view_record+0x1010u+16u,shape==6u?0xffffffffu:second_weapon);
    m.put(model::representations,representation_header);
    m.put(representation_header+0x138u,1u);
    m.put(representation_header+0x13cu,representation);
    m.put(representation+4u,shape==7u?0xffffffffu:7u);
    m.put(representation+12u,shape==9u?0xffffffffu:6u);
    m.put(model::objects,object_header);
    m.put(object_header+0x44u,datums);
    m.put(model::tags,tag_instances);
    for(std::uint32_t index=1u;index<=7u;++index) {
        m.put(tag_instances+index*16u+8u,tag_data+index*0x100u);
        if(index<=3u) {
            m.put(datums+index*12u+8u,object_values+index*0x100u);
            m.put(object_values+index*0x100u,index);
        }
    }
    m.put(tag_data+0x100u+0x38u,shape==10u?0xffffffffu:4u);
    m.put(tag_data+0x400u+4u,5u);
    m.put(tag_data+0x500u+0x44u,0x12345678u);
    m.put(tag_data+0x600u+0x44u,shape==11u?0u:0x12345678u);
    m.put(tag_data+0x500u+0x48u,6u);
    m.put(tag_data+0x600u+0x48u,6u);
    m.put(tag_data+0x600u+0x70u,shape==12u?0x7f800001u:0x3f000000u);
    m.put(0x46a0e8u,shape==13u?0x3f000000u:0xbf800000u);
    m.put(model::camera_forward+4u,0x3f800000u);
    m.put(0x4c65d4u,shape==13u?0x3f000000u:0u);
    m.put(model_choice+4u,5u);
    if(shape==14u)c.policy|=4u;
    if(shape==15u)m.drop(tag_data);
    if((i/128u)%2u) {
        if(shape==0u) {
            m.denied_write=stack-8u;
        } else if(shape==5u) {
            m.denied_write=stack-12u;
            m.writes_before_denial=1u;
        }
    }
    c.full_observation=!census_mode && (i/16u)%2u!=0;
    if(!c.full_observation && !census_mode && (i/256u)%2u)c.activate_trace_at_store=0x144bf0u;
    if(c.full_observation && (i/32u)%2u && shape>4u) {
        constexpr std::uint32_t faults[]{0x168e54u,0x168f5fu,0x169048u,0x16910fu};
        c.reject_instruction=faults[(i/64u)%std::size(faults)];
    }
    return c;
}
bool native(WorldState &s) {
    return model::bridge(s,original::native_frame_misc_reference_168ded,
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
}

int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_FIRST_PERSON_MODEL_PREPARATION");
    std::mt19937 rng(0x168dedu);
    Totals totals;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,original::run_168ded,totals,false,{},true))return 1;
    report(cases,totals,model::coverage().load(),unsigned(model::branch_count),mode);
}catch(const std::exception &error){std::fprintf(stderr,"%s\n",error.what());return 1;}
