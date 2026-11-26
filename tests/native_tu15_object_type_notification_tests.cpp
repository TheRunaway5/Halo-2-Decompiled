// The 10DFA0 source copies notify an object's type chain through the same
// callback ABI. Exercise signed indexing, null callbacks, child changes
// to the live walk and stack, and memory or callback faults.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_object_type_notification.hpp"

using namespace frame_misc_harness;
namespace owner=halo2::native_object_type_notification_detail;

namespace {
constexpr std::uint32_t headers=0x10001000u,rows=0x10003000u,object=0x10005000u;
constexpr std::uint32_t primary=0x10029000u,alternate=0x10059000u;
constexpr std::uint32_t active_type=0x100b1000u,quiet_type=0x100b3000u,redirect_type=0x100b5000u;
constexpr std::uint32_t negative_type=0x100b7000u,shift_type=0x100b9000u,fault_type=0x100bb000u;
constexpr std::uint32_t writable=0x11008000u,stack=0x20002080u;
constexpr std::uint32_t active_callback=0x300010u,redirect_callback=0x300020u,negative_callback=0x300030u;
constexpr std::uint32_t shift_callback=0x300040u,fault_callback=0x300050u;

void notify(WorldState &state,std::uint32_t target,std::uint32_t site,bool tail) {
    if(site!=0x10dfebu||tail)throw std::runtime_error("Changed object type notification dispatch route");
    auto signature=entry_hash(state,target);
    for(unsigned slot=0;slot<3;++slot)
        signature=mix(signature,r(state.memory,state.gp[4]+4u*slot,4));
    log_call(signature);
    std::mt19937_64 random(signature);
    w(state.memory,state.observe,target+1u,writable,4,std::uint32_t(random()));
    state.gp[0]=std::uint32_t(random());
    state.gp[1]=std::uint32_t(random());
    state.gp[2]=std::uint32_t(random());
    state.flags.load(std::uint32_t(random())&0x8d5u);
    state.xmm[random()%8]=_mm_castsi128_ps(_mm_set1_epi32(int(random())));
    _mm_setcsr(_mm_getcsr()|std::uint32_t(random()%64u));
    state.gp[4]+=12u;
    switch(target) {
    case active_callback:break;
    case redirect_callback:
        state.gp[3]^=0xa55a0000u;
        state.gp[5]^=0x5aa50000u;
        state.gp[6]=0u;
        state.gp[7]=alternate;
        break;
    case negative_callback:
        state.gp[6]=0x7fffu;
        break;
    case shift_callback:
        state.gp[4]+=4u;
        break;
    case fault_callback:
        throw std::runtime_error("Object type callback fault");
    default:
        throw std::runtime_error("Object type callback target was not authored");
    }
}
}

namespace original {
using namespace halo2::physics_world_runtime_detail;
using frame_misc_harness::sign_extend;
std::uint32_t selected(std::uint32_t address,SourcePhysicsContactImage){return address;}
void dispatch(WorldState &state,std::uint32_t target,std::uint32_t site,bool tail){notify(state,target,site,tail);}
#include "body.inc"
#include "reference.inc"
}

namespace {
Case author(std::mt19937 &random,unsigned index) {
    Case test;
    auto &memory=test.memory;
    random_abi(random,test,stack);
    const auto shape=index%16u;
    const auto row=std::uint32_t(random()%24u);
    test.abi.gp[3]=(std::uint32_t(random())<<16u)|row;
    memory.put(0x4ecbd0u,headers);
    memory.put(headers+0x44u,rows);
    memory.put(rows+12u*row+8u,object);
    const int type=shape%4u==0?-128:shape%4u==1?-1:int(random()%10u);
    memory.put(object+0xaau,std::uint32_t(type),1);
    memory.put(0x474b90u+4u*std::uint32_t(type),primary);
    for(std::uint32_t offset=0u;offset<0x60u;offset+=4u)
        memory.put(stack-0x30u+offset,std::uint32_t(random()));
    memory.put(stack,0x10df00u+std::uint32_t(random()%32u));
    memory.put(writable,std::uint32_t(random()));
    memory.put(active_type+0x58u,active_callback);
    memory.put(quiet_type+0x58u,0u);
    memory.put(redirect_type+0x58u,redirect_callback);
    memory.put(negative_type+0x58u,negative_callback);
    memory.put(shift_type+0x58u,shift_callback);
    memory.put(fault_type+0x58u,fault_callback);

    const std::array chain{
        0u,quiet_type,active_type,redirect_type,negative_type,shift_type,fault_type,active_type,
        active_type,active_type,active_type,active_type,active_type,active_type,quiet_type,active_type,
    };
    memory.put(primary+0x84u,chain[shape]);
    memory.put(primary+0x88u,shape==7u||shape==14u?quiet_type:0u);
    memory.put(primary+0x8cu,shape==7u?active_type:0u);
    memory.put(primary+0x90u,0u);
    memory.put(alternate+0x88u,active_type);
    memory.put(alternate+0x8cu,0u);
    memory.put(primary-0x20000u+0x84u,active_type);
    memory.put(primary-0x20000u+0x88u,0u);

    if(shape==8u)memory.drop(0x4ecbd0u);
    if(shape==9u)memory.drop(headers);
    if(shape==10u)memory.drop(rows);
    if(shape==11u)memory.drop(object);
    if(shape==12u)memory.drop(primary);
    if(shape==13u)memory.drop(active_type);
    return test;
}

bool native(WorldState &state) {
    return owner::bridge(state,[](auto &value){original::native_frame_misc_reference_10dfa0(value);},
        [](auto &value,std::uint32_t target,std::uint32_t site){original::dispatch(value,target,site,false);});
}
void reference(WorldState &state){original::run_10dfa0(state);}
}

int main(int argc,char **argv)try {
    const auto count=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_TYPE_NOTIFICATION");
    std::mt19937 random(0x10dfa0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals totals;
    for(unsigned index=0;index<count;++index)
        if(!compare(index,author(random,index),native,reference,totals))return 1;
    if(!census_mode)check_refusals(author(random,0u),native);
    report(count,totals,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &error){std::fprintf(stderr,"%s\n",error.what());return 1;}
