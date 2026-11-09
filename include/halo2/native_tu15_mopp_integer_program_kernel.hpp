#pragma once
#include <array>
#include <cstdint>

namespace halo2::native_mopp_integer_program_detail {
enum class Family {jump,key_base,property};
struct Progress {
    Family family{};
    std::uint32_t program{},opcode{},count{},index{},encoded{},value{},base{};
    std::array<std::uint32_t,4>bytes{};
};

// Decode authored geometry DATA, never processor instructions. Frame owns
// actual field storage, lazy copy lifetime and semantic I/O publication.
// Both the public value controller and retained caller adapter use this core.
template<class Frame>
bool step(Frame&frame,std::uint32_t&program,std::uint32_t opcode){
    Progress d{};d.program=program;d.opcode=opcode;
    if(opcode>=5u&&opcode<=7u){d.family=Family::jump;d.count=opcode-4u;}
    else if(opcode==9u||opcode==10u||opcode==11u){
        d.family=Family::key_base;d.count=opcode==9u?1u:opcode==10u?2u:4u;
    }else if(opcode>=0x60u&&opcode<=0x6bu){
        d.family=Family::property;d.count=opcode<0x64u?1u:opcode<0x68u?2u:4u;
    }else return false;
    for(d.index=0;d.index<d.count;++d.index){
        d.value=frame.encoded_byte(d);d.bytes[d.index]=d.value;
        d.encoded=(d.encoded<<8u)+d.value;
    }
    frame.decoded(d);
    if(d.family==Family::jump){
        program+=d.encoded+d.count+1u;frame.advance(d,program);return true;
    }
    if(d.family==Family::key_base){
        frame.ensure_variables(d);
        d.base=opcode==11u?0u:frame.key_base(d);
        frame.store_key_base(d,d.base+d.encoded);
        program+=d.count+1u;frame.advance(d,program);return true;
    }
    // The one-byte form advances before its first property store; the other
    // forms advance between the property store and the lazy variable copy.
    if(d.count==1u){program+=2u;frame.advance(d,program);}
    frame.store_property(d,opcode&3u,d.encoded);
    if(d.count!=1u){program+=d.count+1u;frame.advance(d,program);}
    frame.finish_property(d);
    return true;
}
} // namespace halo2::native_mopp_integer_program_detail
