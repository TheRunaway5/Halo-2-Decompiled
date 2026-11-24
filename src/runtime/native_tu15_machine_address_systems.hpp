#pragma once
// Native TU1.5 machine address check, entered at 69AD0 (physics live support
// unit): whether a 6-byte machine address is answered for.
//
//   machine_address::check   69AD0(EAX = address*) -> AL
//
// With networking off (byte [4DC040] clear) every address is: the answer is
// 1. With the network block ([4DC04C]) in no mode (+8 zero) none is: 0.
// Otherwise this machine's address (+0xD, copied to the frame) is compared
// with the given one word by word, as REPE CMPSW does in the string
// direction flag's direction: the same address answers 1; any other is
// looked up among the peers (6C3F0, the network block pushed), and a peer's
// byte +0x78 is the answer, or the frame's answer byte (0) without one.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the original's frame (the answer byte
// and the copied address); the boundary adapter
// (native_tu15_machine_address.hpp) maps the role to the original
// instruction and owns the ABI of 6C3F0.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_machine_address {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t networking=0x4dc040u,network=0x4dc04cu;}
namespace network {inline constexpr std::uint32_t mode=8u,machine=0xdu;}
namespace peer {inline constexpr std::uint32_t answer=0x78u;}
inline constexpr std::uint32_t address_words=3u;
// The frame's slots, by offset from the return address: the answer byte and this machine's address.
namespace local {inline constexpr std::int32_t answer=-9,machine=-8;}

namespace machine_address {
enum class Access {networking,answer_clear,network,mode,machine_low,machine_high,copy_low,copy_high,given_word,copy_word,peer_answer,
    answer_load};
enum class Outcome {offline,no_mode,same,peer,unknown};
// Frame:
//   local(offset), saving(n)                     PUSH EBX (MOV EBX,EAX; XOR AL,AL), then ESI and EDI
//   enabled_tested(enabled), mode_tested(mode), copying(machine)   TEST CL,CL; TEST ECX,ECX; LEA EAX
//   comparing(address, copy), step()             MOV ECX,3; LEA EDI; MOV ESI,EBX; XOR EAX,EAX; the direction
//   compared(given, copy)                        one CMPSW: its flags, ESI, EDI and ECX
//   compared_all(), restoring()                  POP EDI; POP ESI
//   peer(network) -> EAX                         6C3F0(the network block pushed, RET 4), TEST EAX,EAX
template<class Memory,class Frame> std::uint32_t check(Memory &memory,Frame &frame,std::uint32_t address,Outcome &outcome) {
    using enum Access;
    const auto enabled=memory.load8(Access::networking,globals::networking);
    frame.saving(0u);
    frame.enabled_tested(enabled);
    memory.store8(answer_clear,frame.local(local::answer),0u);
    if(!enabled){outcome=Outcome::offline;return 1u;}
    const auto network_address=memory.load32(network,globals::network);
    const auto mode_value=memory.load32(mode,at(network_address,network::mode));
    frame.mode_tested(mode_value);
    if(!mode_value){outcome=Outcome::no_mode;return 0u;}
    const auto machine=network_address+network::machine;
    frame.copying(machine);
    const auto low=memory.load32(machine_low,machine);
    const auto high=memory.load16(machine_high,machine+4u);
    frame.saving(1u);
    memory.store32(copy_low,frame.local(local::machine),low);
    memory.store16(copy_high,frame.local(local::machine)+4u,high);
    // REPE CMPSW over the three words, to the first that differs.
    auto given=address,copy=frame.local(local::machine);
    frame.comparing(given,copy);
    const auto step=frame.step();
    bool same=true;
    for(std::uint32_t word=0;word<address_words && same;++word,given+=step,copy+=step) {
        // The retained translated route reads EDI's word (the copy) before ESI's; the trace follows it.
        const auto copy_value=memory.load16(copy_word,copy);
        const auto given_value=memory.load16(given_word,given);
        frame.compared(given_value,copy_value);
        same=given_value==copy_value;
    }
    frame.restoring();
    if(same){outcome=Outcome::same;return 1u;}
    const auto record=frame.peer(network_address);
    if(record){outcome=Outcome::peer;return memory.load8(peer_answer,at(record,peer::answer));}
    outcome=Outcome::unknown;
    return memory.load8(answer_load,frame.local(local::answer));
}
} // namespace machine_address
} // namespace halo2::native_machine_address
