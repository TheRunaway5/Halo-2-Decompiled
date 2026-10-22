#include "halo2/cache.hpp"
#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/source_network_session_startup_tu15.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool value,const char* message){if(!value)throw FormatError(message);}
std::vector<std::byte> bytes(const std::string& hex) {
    check(hex.size()%2==0,"Odd original byte string");std::vector<std::byte> out;
    for(unsigned i=0;i<hex.size();i+=2)out.push_back(std::byte(std::stoul(hex.substr(i,2),nullptr,16)));
    return out;
}
}
int main(int argc,char**argv)try {
    check(argc==2,"Supply unchanged original680C0 fixture");std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
    check(fixture["original_xbe_sha256"]=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        fixture["entry"]==0x680c0u && fixture["bank_address"]==0x4da868u && fixture["bank_bytes"]==0x17d8u,
        "Network configuration fixture authority differs");
    const auto cold=bytes(fixture["cold_hex"]),expected=bytes(fixture["after_hex"]);
    SourceRuntimeKernelTickCount tick{0x60212000u,{},"network configuration unit clock"};auto memory=native_tu15_world_memory(tick);
    check(std::ranges::equal(memory.view(0x4da868u,0x17d8u),cold),"Configuration storage is absent or already constructed");
    memory.validate_mutable_view(0x4da868u,0x17d8u);
    for(const auto& row:fixture["immutable_operands"]) {
        const unsigned address=row["address"];const auto raw=bytes(row["hex"]);
        std::vector<std::byte> actual(raw.size());memory.read(address,actual);
        check(actual==raw,"Original constructor scalar changed");
        for(unsigned i=0;i<raw.size();++i)check(memory.is_read_only(address+i,1),"Constructor scalar lost immutable ownership");
    }
    constexpr unsigned stack=0x600f0000u;std::vector<std::byte> frame(4096);frame[3]=std::byte{3};
    memory.mount(stack,frame,SourceRuntimeMemory::Access::ReadWrite,"declared constructor call frame");
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=stack;
    std::vector<unsigned> instructions;Json stores=Json::array();
    const auto hex=[](std::span<const std::byte> raw){constexpr char digits[]="0123456789abcdef";std::string out;for(auto value:raw){const auto v=std::to_integer<unsigned>(value);out+=digits[v>>4];out+=digits[v&15];}return out;};
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](unsigned pc){instructions.push_back(pc);};
    services.required_owner=[](auto&,const auto&,auto&,const auto&){throw FormatError("Pure configuration constructor requested an external owner");};
    source_run_network_session_startup_tu15(memory,0x680c0u,abi,{0x27f,0x1f80},services,
        [&](const auto& store){stores.push_back({{"instruction",store.instruction},{"address",store.address},
            {"before_hex",hex(store.before)},{"written_hex",hex(store.written)}});});
    check(instructions==fixture["executed_instruction_sequence"].get<std::vector<unsigned>>() && stores==fixture["stores"],
        "Native configuration constructor changed original instructions or ordered stores");
    check(std::ranges::equal(memory.view(0x4da868u,0x17d8u),expected) &&
        abi.gp[0]==fixture["return_eax"].get<unsigned>() && abi.gp[1]==fixture["return_ecx"].get<unsigned>() && abi.gp[4]==stack+4,
        "Native configuration constructor changed full bank or caller ABI");
    const std::array<std::byte,4> changed{std::byte{0x89},std::byte{0x13},std::byte{0},std::byte{0}};
    memory.write(0x4db6b0u,changed);mount_native_tu15_network_configuration_storage(memory);
    check(std::ranges::equal(memory.view(0x4db6b0u,4),changed),"Storage remount reset a live source configuration field");
    SourceRuntimeMemory complete;mount_native_tu15_network_session_data(complete);complete.write(0x4db6b0u,changed);
    mount_native_tu15_network_configuration_storage(complete);
    check(std::ranges::equal(complete.view(0x4db6b0u,4),changed),"Full network owner and local configuration do not share live storage");
    unsigned rejected{};
    for(bool wrong_scalar:{false,true}) {
        SourceRuntimeMemory bad;
        if(wrong_scalar)bad.mount(0x44a8a0u,changed,SourceRuntimeMemory::Access::ReadOnly,"declared wrong scalar control");
        else bad.mount(0x4da868u,cold,SourceRuntimeMemory::Access::ReadOnly,"declared read-only configuration control");
        bool stopped=false;try{mount_native_tu15_network_configuration_storage(bad);}catch(const std::exception&){stopped=true;}
        check(stopped,"Invalid configuration owner was admitted");++rejected;
    }
    std::cout<<Json{{"passed",true},{"original_instructions",instructions.size()},{"original_ordered_stores",stores.size()},
        {"complete_bank_bytes",expected.size()},{"immutable_inputs",fixture["immutable_operands"].size()},
        {"live_owner_preservation_controls",2},{"rejected_invalid_owners",rejected},
        {"MCC_runtime_acceptance",false},{"full_weapon_parity",false}}.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
