#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>

namespace halo2::native_mopp_aabb_detail {
enum class ProjectionRead {low_threshold,high_threshold,upper_component,lower_component};
struct Projection {
    std::uint32_t opcode{},low_byte{},high_byte{},low{},high{},upper{},lower{};
    std::array<std::uint32_t,6>components{};
    unsigned component_count{};
};
// Project a quantized AABB onto the authored MOPP split direction. Fields may
// be owned semantic values or a live caller view. Keep every field/byte read
// in its original order; a live Memory owner can distinguish reordered reads.
// Notify publishes semantic read progress, never Xbox instruction execution.
template<class Fields,class Bytes,class Notify>
Projection project(std::uint32_t opcode,Fields&fields,Bytes bytes,Notify notify){
    Projection d;d.opcode=opcode;
    const auto low=[&]{notify(ProjectionRead::low_threshold,0u,d);d.low_byte=bytes(1u);};
    const auto high=[&]{notify(ProjectionRead::high_threshold,0u,d);d.high_byte=bytes(2u);};
    const auto upper=[&](unsigned axis){
        notify(ProjectionRead::upper_component,axis,d);const auto value=fields.upper(axis);
        d.components[d.component_count++]=value;return value;
    };
    const auto lower=[&](unsigned axis){
        notify(ProjectionRead::lower_component,axis,d);const auto value=fields.lower(axis);
        d.components[d.component_count++]=value;return value;
    };
    switch(opcode){
    case 0x10:case 0x11:case 0x12:
        high();low();d.lower=lower(opcode-0x10u);d.upper=upper(opcode-0x10u);break;
    case 0x13:
        d.lower=lower(1);low();high();d.lower+=lower(2);
        d.upper=upper(2);d.upper+=upper(1);break;
    case 0x14:
        d.upper=upper(2);d.lower=lower(1);low();high();d.lower-=d.upper;
        d.upper=upper(1);d.upper-=lower(2);break;
    case 0x15:
        d.lower=lower(0);low();high();d.lower+=lower(2);
        d.upper=upper(2);d.upper+=upper(0);break;
    case 0x16:
        d.upper=upper(2);d.lower=lower(0);low();high();d.lower-=d.upper;
        d.upper=upper(0);d.upper-=lower(2);break;
    case 0x17:
        d.lower=lower(0);low();high();d.lower+=lower(1);
        d.upper=upper(1);d.upper+=upper(0);break;
    case 0x18:
        low();high();d.upper=upper(1);d.lower=lower(0);d.lower-=d.upper;
        d.upper=upper(0);d.upper-=lower(1);break;
    case 0x19:
        low();d.lower=lower(1);high();d.lower+=lower(2);d.lower+=lower(0);
        d.upper=upper(2);d.upper+=upper(1);d.upper+=upper(0);break;
    case 0x1a:
        low();d.upper=upper(2);high();d.lower=lower(1);d.lower-=d.upper;d.lower+=lower(0);
        d.upper=upper(1);d.upper-=lower(2);d.upper+=upper(0);break;
    case 0x1b:
        low();d.upper=upper(1);high();d.lower=lower(2);d.lower-=d.upper;d.lower+=lower(0);
        d.upper=upper(2);d.upper-=lower(1);d.upper+=upper(0);break;
    case 0x1c:
        low();d.upper=upper(2);high();d.lower=lower(0);d.lower-=d.upper;d.lower-=upper(1);
        d.upper=upper(0);d.upper-=lower(2);d.upper-=lower(1);break;
    default:throw std::runtime_error("Native MOPP AABB projection requires its authored split direction");
    }
    if(opcode<=0x12u){d.low=d.low_byte;d.high=d.high_byte;}
    else if(opcode<=0x18u){
        const auto bias=(opcode==0x14u||opcode==0x16u||opcode==0x18u)?255u:0u;
        d.low=d.low_byte*2u-bias;d.high=d.high_byte*2u-bias;
    }else{
        const auto bias=(opcode==0x1au||opcode==0x1bu)?0x55u:opcode==0x1cu?0xaau:0u;
        d.low=(d.low_byte-bias)*3u;d.high=(d.high_byte-bias)*3u;
    }
    return d;
}
} // namespace halo2::native_mopp_aabb_detail
