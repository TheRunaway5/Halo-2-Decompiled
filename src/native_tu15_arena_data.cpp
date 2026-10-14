#include "halo2/native_tu15_arena_data.hpp"
#include <array>

namespace halo2 {
namespace {
void word(SourceRuntimeMemory& memory,std::uint32_t address,std::uint32_t value,const char* owner) {
    std::array<std::byte,4> bytes{};
    for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));
    memory.mount(address,bytes,SourceRuntimeMemory::Access::ReadOnly,owner);
}
void cold(SourceRuntimeMemory& memory,std::uint32_t address,std::size_t bytes,const char* owner) {
    memory.mount(address,std::vector<std::byte>(bytes),SourceRuntimeMemory::Access::ReadWrite,owner);
}
}
void mount_native_tu15_arena_data(SourceRuntimeMemory& memory) {
    // TU1.5 12DF10 initializes this 13-word physical partition record before
    // 21B920 consumes its bounds. These imports identify native page services.
    cold(memory,0x4f2cf0u,13*4,"native TU1.5 cold physical partition record");
    word(memory,0x41b640u,0x800000a6u,"native TU1.5 physical allocation import identity");
    word(memory,0x41b648u,0x800000b6u,"native TU1.5 physical protection import identity");

    // 123450: four resource bindings and a job counter, 150 source 0x40-byte
    // jobs, free/work list heads, and the file-job sequence. All are loader-
    // zeroed storage in the authenticated reference, not captured live data.
    cold(memory,0x4ecc24u,5*4+150*0x40+3*4,"native TU1.5 cold async job owner");
    cold(memory,0x506ba0u,0x8000,"native TU1.5 cold async IO buffer");
    cold(memory,0x50eba0u,3*4,"native TU1.5 cold async IO buffer descriptor");
    cold(memory,0x51db10u,4,"native TU1.5 cold executing async job binding");

    // 126640 and 21B990 construct the arena and its two actual savegame
    // files. Preserve separately named fields; do not mount their surrounding
    // globals or presume the values produced by the constructors.
    cold(memory,0x4f0430u,1,"native TU1.5 cold game arena initialized flag");
    cold(memory,0x4f2950u,2*4,"native TU1.5 cold game arena base and cursor");
    cold(memory,0x4f295cu,4,"native TU1.5 cold game arena checksum");
    cold(memory,0x4f2964u,4,"native TU1.5 cold game arena storage binding");
    cold(memory,0x50ebb0u,1,"native TU1.5 cold physical suballocation active flag");
    cold(memory,0x50ebb4u,2*4,"native TU1.5 cold physical suballocation address and size");
    cold(memory,0x50ebccu,1,"native TU1.5 cold savegame files initialized flag");
    cold(memory,0x50ebd0u,2*4,"native TU1.5 cold savegame file handles");
    cold(memory,0x51df2cu,4,"native TU1.5 cold arena object binding");

    // 166AE0 computes the full CRC polynomial table. Initializing the storage
    // to zero is a loader declaration, never a replacement for running it.
    cold(memory,0x56e225u,1,"native TU1.5 cold CRC table initialized flag");
    cold(memory,0x56e258u,256*4,"native TU1.5 cold CRC polynomial table");
    constexpr char pattern[]="n:\\savegame%d.bin";
    memory.mount(0x462bfcu,std::as_bytes(std::span(pattern)),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 savegame filename format");
    word(memory,0x47c76cu,0x462bfcu,"native TU1.5 savegame filename format binding");
}
}
