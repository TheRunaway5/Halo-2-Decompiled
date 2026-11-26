#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/animation/native_tu15_animation_request.hpp"
#include <set>

namespace {
using namespace frame_misc_harness;
using namespace halo2::native_animation_request;

constexpr std::uint32_t unit=0x10001000u,channel=unit+0x400u,pose=unit+0x700u;
constexpr std::uint32_t unit_system=0x10002000u,unit_slots=0x10002100u,definition=0x10002800u;
constexpr std::uint32_t definition_slots=0x10002c00u,request=0x10003000u,information=0x10003400u;
constexpr std::uint32_t marker=0x10003800u,offset=0x10003c00u;
constexpr std::uint32_t datum=0x10240001u;

unsigned cleanup(std::uint32_t target) {
    switch(target) {
        case 0x1115d0u:case 0xbac00u:case 0xbc190u:case 0xbc380u:case 0xd9110u:
        case 0xe7d90u:case 0x11ddf0u:case 0x145070u:return 4;
        case 0xe8f70u:case 0x140f00u:case 0x1cf650u:return 8;
        case 0xe9050u:case 0x112a80u:case 0x1ceb70u:case 0x1d6430u:return 12;
        case 0x1d39c0u:return 20;
        case 0x111b10u:return 28;
        default:return 0;
    }
}
void fixture_child(WorldState &s,std::uint32_t target,std::uint32_t site) {
    auto &gp=s.gp;
    if(r(s.memory,gp[esp],4)!=site+5u)throw std::runtime_error("Wrong original CALL return word");
    log_call(mix(entry_hash(s,target),site));
    const unsigned conditions=policy>>5u;
    const auto bit=[&](unsigned index){return (conditions>>index)&1u;};
    switch(target) {
        case 0x115ff0u:s.set(eax,bit(0),1);break;
        case 0x1115d0u:gp[eax]=0x07000039u;break;
        case 0xb8780u:gp[eax]=information;break;
        case 0xc1cc0u:s.set(eax,bit(2),1);break;
        case 0x111580u:s.set(eax,bit(1),1);break;
        case 0x111550u:case 0x140f00u:gp[eax]=2u;break;
        case 0xbac00u:gp[eax]=offset;break;
        case 0x111cd0u:gp[eax]=bit(3)?0x0d000042u:0x07000101u;break;
        case 0xb1870u:s.set(eax,bit(4),1);break;
        case 0x1136f0u:s.set(eax,bit(5),1);break;
        case 0xc9550u:s.set(eax,bit(6),1);break;
        case 0x112a80u:s.set(eax,bit(7),1);break;
        case 0x111b10u:s.set(eax,bit(8),1);break;
        case 0x111500u:s.set(eax,bit(9),1);break;
        case 0x1136a0u:s.set(eax,bit(10),1);break;
        case 0x110350u:case 0x1111d0u:s.set(eax,bit(11),1);break;
        case 0xe9050u:
            if(site==0x113facu&&bit(12)) {
                w(s.memory,s.observe,target,r(s.memory,gp[esp]+8,4),4,0x03001234u);
                w(s.memory,s.observe,target,r(s.memory,gp[esp]+12,4),4,0x0400000cu);
            }
            break;
        default:break;
    }
    (void)s.pop();
    gp[esp]+=cleanup(target);
}

#include "animation_request_reference.inc"

bool native(WorldState &s){return bridge(s,fixture_child);}
void reference(WorldState &s){run_113d30(s);}

Case fixture(std::mt19937 &random,unsigned index) {
    Case test;
    random_abi(random,test,0x20010400u+(index&3u));
    const unsigned scenario=index%24u;
    const unsigned mods=index<24u ? 0u : random()&0x1fffffu;
    test.policy=(mods<<5u)|scenario;
    const auto bit=[&](unsigned bit){return (mods>>bit)&1u;};
    auto &m=test.memory;
    const auto sp=test.abi.gp[esp];
    for(auto address=sp-0x180u;address<=sp+0x20u;address+=4u)m.put(address,random());
    m.put(sp,0x63000000u);
    m.put(sp+4u,datum);
    m.put(sp+8u,request);
    m.put(unit_table,unit_system);
    m.put(unit_system+0x44u,unit_slots);
    m.put(unit_slots+0x14u,unit);
    m.put(unit_slots+0x20u,unit);
    m.put(definition_table,definition_slots);
    m.put(definition_slots+8u,definition);
    m.put(unit,0x00010000u);
    m.put(unit+0x14u,(scenario==18u||scenario==19u)?none:0x10240002u);
    m.put(unit+0x12au,0x400u,2);
    m.put(unit+0x33eu,0x700u,2);
    m.put(unit+0x346u,0x400u,2);
    m.put(unit+0x10au,scenario==18u?4u:0u,1);
    m.put(unit+0x1f4u,bit(0),1);
    m.put(unit+0x112u,0x500u,2);
    m.put(unit+0x10eu,0x530u,2);
    m.put(unit+0x110u,32u,2);
    m.put(unit+0xaau,scenario==20u?1u:0u,1);
    m.put(unit+0xc1u,bit(14),1);
    m.put(unit+0x1fcu,0,2);
    for(unsigned n=0;n<4;++n)m.put(unit+0x218u+4u*n,n&1u?none:datum);
    m.put(channel,scenario==15u?none:2u);
    m.put(channel+6u,scenario==16u?none:3u,2);
    m.put(channel+0x11u,scenario==21u?1u:2u,1);
    m.put(channel+0x12u,scenario==17u?(bit(15)?0x4003u:0x4001u):1u,2);
    m.put(channel+0x14u,bit(13)?3u:0u,2);
    m.put(channel+0x16u,7u,2);
    m.put(channel+0x68u,bit(16)?none:6u);
    m.put(channel+0x6cu,scenario==22u?1u:0u,1);
    m.put(channel+0x6eu,bit(17),1);
    m.put(channel+current_activity,scenario==19u&&bit(15)?0x020001b3u:default_activity);
    m.put(channel+0x74u,1u);
    m.put(channel+0x78u,2u);
    constexpr std::array<std::uint32_t,15> activities{
        0x0a00001du,0x0800001cu,0x0700005cu,0x0700005du,0x090006b2u,
        0x09000020u,0x0900001fu,0x0c000043u,0x0d000042u,0x0e0000c2u,
        0x0c000099u,0x07000101u,0x09000022u,0x0900001eu,0x01000004u
    };
    m.put(channel+current_overlay,activities[scenario%activities.size()]);
    for(unsigned slot:{0x20u,0x40u,0x9cu,0xbcu,0xdcu}) {
        const auto address=(slot<0x90u?channel:pose)+slot;
        m.put(address,scenario==23u?none:slot);
        m.put(address+6,1u,2);
        m.put(address+8,1u);
        m.put(address+0x11u,slot>=0x90u?(bit(19)?2u:0u):0u,1);
        m.put(address+0x12u,slot>=0x90u?(bit(20)?(bit(15)?0x4003u:0x4001u):1u):1u,2);
        m.put(address+0x14u,bit(13)?7u:0u,2);
        m.put(address+0x16u,2u,2);
    }
    m.put(channel+0x36u,bit(18),2);
    m.put(pose,bit(19)?0x80u:0u,1);
    m.put(pose+1u,bit(20),1);
    m.put(pose+0x2cu,bit(14)?0x07000400u:none);
    const bool unchanged_special=scenario<2u&&bit(18);
    m.put(request,scenario==19u||unchanged_special?0u:(bit(11)?none:0x0300002au));
    m.put(request+4u,unchanged_special?activities[scenario]:(bit(10)?0x0400000cu:default_activity));
    m.put(request+8u,1u);
    m.put(definition+0x254u,bit(17)?none:2u,2);
    m.put(definition+0xbcu,2u);
    m.put(definition+0x1ccu,marker);
    m.put(marker,bit(12)?1u<<11u:0u);
    m.put(marker+0x3eu,bit(14)?0u:none,2);
    m.put(information+0x14u,bit(9)?0u:4u,2);
    m.put(0x474d04u,offset);
    m.put(offset,0x3f800000u);
    m.put(offset+4u,0x40000000u);
    m.put(offset+8u,0x40400000u);
    m.put(offset+0x28u,0x40a00000u);
    m.put(offset+0x2cu,0x40c00000u);
    m.put(offset+0x30u,0x40e00000u);
    m.put(0x56e1dcu,bit(8)?datum-1u:datum);
    return test;
}
}

int main(int argc,char **argv) {
    if(argc!=2)return 2;
    const auto cases=std::stoul(argv[1]);
    std::mt19937 random{0x113d30u};
    Totals totals;
    std::set<std::uint32_t> reached;
    for(unsigned index=0;index<cases;++index) {
        auto test=fixture(random,index);
        test.full_observation=true;
        auto original_memory=test.memory.clone();
        const auto baseline=execute(original_memory,test,false,native,reference);
        if(!baseline.fault.empty()||baseline.stores.empty()||baseline.cursor!=halo2::native_animation_request::exit) {
            std::fprintf(stderr,"invalid reference case %u PC %x: %s\n",index,baseline.cursor,baseline.fault.c_str());
            return 1;
        }
        reached.insert(baseline.instructions.begin(),baseline.instructions.end());
        const auto &store=baseline.stores[index%baseline.stores.size()];
        switch((index/24u)%6u) {
            case 0:break;
            case 1:test.full_observation=false;break;
            case 2:test.full_observation=false;test.activate_trace_at_store=store.instruction;break;
            case 3:test.reject_after_store_pc=store.instruction;break;
            case 4:test.memory.denied_write=store.address;break;
            case 5:test.reject_instruction=baseline.instructions[index%baseline.instructions.size()];break;
        }
        if(!compare(index,test,native,reference,totals,false,{},true)) {
            auto native_memory=test.memory.clone();
            auto reference_memory=test.memory.clone();
            const auto native_trace=execute(native_memory,test,true,native,reference).instructions;
            const auto reference_trace=execute(reference_memory,test,false,native,reference).instructions;
            unsigned i=0;
            while(i<native_trace.size()&&i<reference_trace.size()&&native_trace[i]==reference_trace[i])++i;
            std::fprintf(stderr,"divergence at %u %x / %x\n",i,
                i<native_trace.size()?native_trace[i]:0u,i<reference_trace.size()?reference_trace[i]:0u);
            return 1;
        }
    }
    report(unsigned(cases),totals,reached.size(),1,"quiet-and-observed");
    for(auto pc:reached)std::fprintf(stderr,"%x ",pc);
    return !totals.faults||!totals.children;
}
