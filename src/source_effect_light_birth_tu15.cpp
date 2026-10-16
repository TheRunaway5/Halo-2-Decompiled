#include "halo2/source_effect_light_birth_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/cache.hpp"
#include <cstring>
#include <set>

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p){return Bytes(m.view(p,4)).u32(0);}
std::uint16_t half(const SourceRuntimeMemory& m,std::uint32_t p){return Bytes(m.view(p,2)).u16(0);}
struct Pool {std::uint32_t address,records,bitmap,capacity,high,stride;};
Pool pool(const SourceRuntimeMemory& m,std::uint32_t global,std::uint32_t capacity,std::uint32_t stride) {
    const auto p=word(m,global);Pool out{p,word(m,p+0x44),word(m,p+0x48),word(m,p+0x20),word(m,p+0x38),word(m,p+0x24)};
    if(out.capacity!=capacity || out.stride!=stride || out.high>capacity || word(m,p+0x3c)>out.high)
        throw FormatError("TU1.5 light birth lost its actual datum-pool contract");
    (void)m.view(out.records,std::size_t(capacity)*stride);(void)m.view(out.bitmap,((capacity+31)/32)*4);return out;
}
std::uint32_t identity(const SourceRuntimeMemory& m,const Pool& p,std::uint32_t address) {
    if(address<p.records || (address-p.records)%p.stride)
        throw FormatError("TU1.5 light birth parent pointer is not a datum record");
    const auto index=(address-p.records)/p.stride;
    if(index>=p.high || !(word(m,p.bitmap+(index>>5)*4)&(1u<<(index&31))) || !half(m,address))
        throw FormatError("TU1.5 light birth requires an occupied full-generation record");
    return (std::uint32_t(half(m,address))<<16)|index;
}
std::uint32_t record(const SourceRuntimeMemory& m,const Pool& p,std::uint32_t id) {
    if(id==0xffffffffu || (id&65535u)>=p.high)throw FormatError("TU1.5 light birth datum escapes its pool");
    const auto address=p.records+(id&65535u)*p.stride;
    if(identity(m,p,address)!=id)throw FormatError("TU1.5 light birth datum generation changed");return address;
}
}
SourceEffectLightBirthTu15 source_effect_light_birth_before_tu15(const SourceRuntimeMemory& m,
    std::uint32_t callsite,const SourcePhysicsWorldRuntimeAbi& a) {
    if(callsite!=0x17db3bu && callsite!=0x17db6du)
        throw FormatError("TU1.5 light birth observer received an unrelated callsite");
    const auto sp=a.gp[4];
    if(word(m,sp)!=(callsite==0x17db3bu?0x17db40u:0x17db72u) || word(m,sp+0x124)!=0x17d6dcu)
        throw FormatError("TU1.5 light birth lost its actual17D210/17D730 caller stack");
    SourceEffectLightBirthTu15 out;out.callsite=callsite;out.caller_stack=sp;
    out.definition=word(m,sp+4);out.owner=word(m,sp+8);out.node=word(m,sp+12);
    out.up_address=word(m,sp+16);out.position_address=a.gp[3];out.forward_address=a.gp[7];out.control_word=a.xmm[0][0];
    (void)m.view(out.position_address,12);(void)m.view(out.forward_address,12);(void)m.view(out.up_address,12);
    out.source_tick=word(m,word(m,0x51df54)+8);
    const auto effects=pool(m,0x4f720c,256,0x190),locations=pool(m,0x4f7208,512,0x3c);
    out.parent_effect_body=word(m,sp+0x128);out.parent_effect=identity(m,effects,out.parent_effect_body);
    out.event_ordinal=half(m,out.parent_effect_body+0x5c);out.part_address=a.gp[5];
    const auto tag=word(m,out.parent_effect_body+4),row=word(m,0x4f0414)+(tag&65535u)*16;
    if(tag==0xffffffffu || word(m,row)!=0x65666665u)
        throw FormatError("TU1.5 light birth parent has no actual effect definition");
    const auto definition=word(m,row+8),events=word(m,definition+0x18);
    if(out.event_ordinal>=word(m,definition+0x14))throw FormatError("TU1.5 light birth parent event escapes authored events");
    const auto event=events+out.event_ordinal*0x38,parts=word(m,event+0x1c),count=word(m,event+0x18);
    if(out.part_address<parts || (out.part_address-parts)%0x38 || (out.part_address-parts)/0x38>=count ||
       word(m,out.part_address+8)!=0x6c696768u || word(m,out.part_address+0x10)!=out.definition)
        throw FormatError("TU1.5 light birth part differs from the current authored event");
    out.part_ordinal=(out.part_address-parts)/0x38;
    //17D730 saves incoming ESI (the17D210 current location) before replacing
    // it. Accept a location only when that pointer owns a real pool datum and
    // occurs in the parent effect's authored location chains.
    const auto candidate=word(m,sp+0x18);
    if(candidate>=locations.records && candidate-locations.records<locations.capacity*locations.stride &&
       (candidate-locations.records)%locations.stride==0) {
        const auto wanted=identity(m,locations,candidate);bool found=false;std::set<std::uint32_t> seen;
        const auto count_locations=word(m,definition+0xc);
        if(count_locations>32)throw FormatError("TU1.5 light birth effect location array exceeds32");
        for(unsigned n=0;n<count_locations && !found;++n) {
            auto id=word(m,out.parent_effect_body+0x7c+n*4);
            while(id!=0xffffffffu) {
                if(!seen.insert(id).second)throw FormatError("TU1.5 light birth effect location chain cycles");
                const auto at=record(m,locations,id);if(id==wanted){found=true;break;}id=word(m,at+4);
            }
        }
        if(!found)throw FormatError("TU1.5 light birth location is not owned by its parent effect");
        out.location=wanted;out.location_address=candidate;
    }
    return out;
}
SourceEffectLightBirthTu15 source_effect_light_birth_after_tu15(const SourceRuntimeMemory& m,
    const SourceEffectLightBirthTu15& input,const SourcePhysicsWorldRuntimeAbi& a) {
    if(a.gp[4]!=input.caller_stack+20)throw FormatError("TU1.5 light birth callee cleanup changed");
    auto out=input;out.light=a.gp[0];if(out.light==0xffffffffu)return out;
    out.light_record=record(m,pool(m,0x4ecbdc,350,0x110),out.light);
    if(word(m,out.light_record+4)!=out.definition || word(m,out.light_record+0x14)!=out.source_tick)
        throw FormatError("TU1.5 light birth returned an unrelated native record");
    std::memcpy(out.light_words.data(),m.view(out.light_record,sizeof(out.light_words)).data(),sizeof(out.light_words));return out;
}
}
