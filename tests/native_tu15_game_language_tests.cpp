#include "halo2/cache.hpp"
#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/source_cache_startup_tu15.hpp"
#include "halo2/source_map_world_support_tu15.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void check(bool value,const char* message){if(!value)throw FormatError(message);}
std::vector<std::byte> bytes(const std::string& text){std::vector<std::byte> result;for(unsigned i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));return result;}
}
int main(int argc,char**argv)try {
    check(argc==2,"Supply authenticated original language fixture");std::ifstream input(argv[1]);const auto fixture=nlohmann::json::parse(input);
    check(fixture["original_xbe_sha256"]=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        fixture["cached_language_address"]==0x48c750u && fixture["converter_entry"]==0x11f440u,
        "Language fixture authority differs");
    SourceRuntimeKernelTickCount tick{0x60212000u,{},"language ownership unit clock"};auto memory=native_tu15_world_memory(tick);
    check(std::ranges::equal(memory.view(0x48c750u,4),bytes(fixture["authored_initial_hex"])),"Language cache lost its authored unset state");
    memory.validate_mutable_view(0x48c750u,4);
    check(std::ranges::equal(memory.view(0x11f47cu,36),bytes(fixture["branch_table_hex"])) &&
        memory.is_read_only(0x11f47cu,36),"Language converter lost its complete immutable table");
    constexpr unsigned stack=0x600f0000u;std::vector<std::byte> frame(4096);frame[3]=std::byte{3};
    memory.mount(stack,frame,SourceRuntimeMemory::Access::ReadWrite,"declared language conversion caller");
    SourcePhysicsWorldRuntimeServices services;services.required_owner=[](auto&,const auto&,auto&,const auto&){throw FormatError("Pure language conversion requested an external result");};unsigned cases{};
    for(const auto& row:fixture["vectors"]) {
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp[0]=row[0];abi.gp[4]=stack;
        source_run_map_world_support_tu15(memory,0x11f440u,abi,{0x27f,0x1f80},services);
        check(abi.gp[0]==row[1].get<unsigned>() && abi.gp[4]==stack+4,"Native language conversion differs from unchanged11F440");++cases;
    }
    const std::array<std::byte,4> resolved{};memory.write(0x48c750u,resolved);
    mount_native_tu15_game_language_storage(memory);
    check(std::ranges::equal(memory.view(0x48c750u,4),resolved),"Storage remount reset the resolved language");
    SourceRuntimeMemory cache;mount_native_tu15_cache_startup_data(cache);cache.write(0x48c750u,resolved);
    mount_native_tu15_game_language_storage(cache);
    check(std::ranges::equal(cache.view(0x48c750u,4),resolved),"Full cache startup and prompt getter do not share language storage");
    unsigned rejected{};
    for(bool bad_table:{false,true}) {
        SourceRuntimeMemory bad;
        if(bad_table)bad.mount(0x11f47cu,std::vector<std::byte>(36),SourceRuntimeMemory::Access::ReadOnly,"declared wrong language table");
        else bad.mount(0x48c750u,resolved,SourceRuntimeMemory::Access::ReadOnly,"declared immutable language-cache control");
        bool stopped=false;try{mount_native_tu15_game_language_storage(bad);}catch(const std::exception&){stopped=true;}
        check(stopped,"Invalid language owner was admitted");++rejected;
    }
    std::cout<<nlohmann::json{{"passed",true},{"original_language_cases",cases},{"complete_conversion_table_bytes",36},
        {"authored_unset_cache_preserved",true},{"live_owner_preservation_controls",2},{"rejected_invalid_owners",rejected},
        {"MCC_prompt_acceptance",false},{"full_weapon_parity",false}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
