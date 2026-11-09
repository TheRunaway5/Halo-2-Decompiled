#pragma once
#include <array>
#include <cstdint>

namespace halo2::native_mopp_primitive_key_detail {
struct Progress {
    std::uint32_t program{},opcode{},count{},index{},encoded{};
    std::array<std::uint32_t,4>bytes{};
};

// Primitive identifiers are authored geometry DATA: compact inline keys or
// one through four big-endian bytes. A caller view owns lazy DATA access and
// retained ABI publication; the decoder needs no processor flags/registers.
template<class Frame>
bool decode(Frame&frame,std::uint32_t program,std::uint32_t opcode,std::uint32_t&key){
    if(opcode<0x30u||opcode>0x53u)return false;
    Progress d{};d.program=program;d.opcode=opcode;
    if(opcode<0x50u)d.encoded=opcode-0x30u;
    else{
        d.count=opcode-0x4fu;
        for(d.index=0;d.index<d.count;++d.index){
            const auto value=frame.encoded_byte(d);d.bytes[d.index]=value;
            d.encoded=(d.encoded<<8u)+value;
        }
    }
    frame.decoded(d);key=d.encoded;return true;
}
}
