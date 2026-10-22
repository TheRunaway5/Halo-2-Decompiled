#include "halo2/source_ai_frame_tu15.hpp"
#include "halo2/source_final_startup_storage_tu15.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace {
using namespace halo2;
struct PrefixEnd {};
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::vector<std::byte> unhex(const std::string& text) {
    check(text.size()%2==0,"Odd callback reference bytes");std::vector<std::byte> result;
    for(unsigned i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
    return result;
}
std::string hex(std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string result;
    for(auto b:bytes){auto v=std::to_integer<unsigned>(b);result+=digits[v>>4];result+=digits[v&15];}
    return result;
}
void fields(const nlohmann::json& fixture) {
    SourceRuntimeMemory memory;mount_native_tu15_final_startup_storage_data(memory);
    for(const auto& row:fixture.at("callback_fields")) {
        const unsigned address=row.at("address");
        check(hex(memory.view(address,8))==row.at("data_hex").get<std::string>(),"Authored callback tail differs");
        check(memory.is_read_only(address,8),"Authored callback tail became writable");
        // Some descriptors end immediately before another authored record;
        // that next owner can already map+4C. Check this declaration's exact
        // extent instead of requiring another legitimate record to be absent.
        const auto snapshot=memory.checkpoint();
        const auto owner=std::ranges::find(snapshot.regions,address,&SourceRuntimeMemory::CheckpointRegion::address);
        check(owner!=snapshot.regions.end()&&owner->bytes.size()==8,
            "Kind2 callback declaration borrowed the following record or padding");
    }
    const auto selected=fixture.at("callback_fields").at(0);
    const unsigned address=selected.at("address");auto expected=unhex(selected.at("data_hex"));
    SourceRuntimeMemory partial;partial.mount(address,std::span(expected).first(4),SourceRuntimeMemory::Access::ReadOnly,
        "Retained kind2 callback first identity");
    mount_native_tu15_final_startup_storage_data(partial);
    const auto snapshot=partial.checkpoint();
    const auto owner=std::ranges::find(snapshot.regions,address,&SourceRuntimeMemory::CheckpointRegion::address);
    check(owner!=snapshot.regions.end()&&owner->owner=="Retained kind2 callback first identity"&&owner->bytes.size()==4,
        "Callback tail installation replaced an existing compatible identity owner");
    for(bool writable:{false,true}) {
        auto raw=expected;if(!writable)raw[0]^=std::byte{1};
        SourceRuntimeMemory conflict;conflict.mount(address,raw,writable?SourceRuntimeMemory::Access::ReadWrite:
            SourceRuntimeMemory::Access::ReadOnly,"Retained conflicting callback identity");
        bool rejected=false;try{mount_native_tu15_final_startup_storage_data(conflict);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected&&hex(conflict.view(address,8))==hex(raw),"Callback publisher repaired a conflicting alias");
    }
}
void run_case(const nlohmann::json& fixture,const nlohmann::json& input) {
    SourceRuntimeMemory memory;mount_native_tu15_final_startup_storage_data(memory);
    const auto& bank=fixture.at("native_component_bank");
    memory.mount(bank.at(0),std::vector<std::byte>(bank.at(1).get<unsigned>(),
        std::byte(input.value("poison",fixture.value("native_component_poison",0xa5u)))),SourceRuntimeMemory::Access::ReadWrite,
        "Declared original actor header, slots, caller and prior stack bytes");
    for(const auto& patch:input.at("patches"))memory.write(patch.at(0),unhex(patch.at(1)));
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=input.at("gp").get<decltype(abi.gp)>();
    abi.xmm=input.at("xmm").get<decltype(abi.xmm)>();abi.eflags=0x202;
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    std::vector<unsigned> trace;nlohmann::json stores=nlohmann::json::array();
    SourcePhysicsWorldRuntimeServices services{};bool stopped=false;
    services.instruction=[&](unsigned pc){
        const bool end=!input.at("prefix_end").is_null()&&pc==input.at("prefix_end").get<unsigned>();
        if(end&&input.at("entry")==0x274020u)throw PrefixEnd{};
        trace.push_back(pc);if(end)throw PrefixEnd{};};
    services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Selector supplied a callback owner");};
    try{source_run_ai_frame_tu15(memory,input.at("entry"),abi,
        *abi.floating_point,services,[&](const auto& store){stores.push_back(nlohmann::json::array({
            store.instruction,store.address,hex(store.before),hex(store.written)}));});}
    catch(const PrefixEnd&){stopped=true;}
    check(stopped==!input.at("prefix_end").is_null(),"Callback selector whole/prefix boundary differs");
    check(abi.gp==input.at("final_gp").get<decltype(abi.gp)>(),"Callback selector GPR or caller state differs");
    check(abi.xmm==input.at("final_xmm").get<decltype(abi.xmm)>(),"Callback selector SIMD state differs");
    check((abi.eflags&~0x10u)==(input.at("final_flags").get<unsigned>()&~0x10u),"Callback selector modeled flags differ");
    check(trace==input.at("trace").get<std::vector<unsigned>>(),"Callback selector source instruction path differs");
    check(stores==input.at("stores"),"Callback selector ordered stack stores or prior bytes differ");
    if(stopped&&input.contains("callback"))check(abi.gp[1]==input.at("callback").get<unsigned>(),"Callback selector selected a different original identity");
    if(input.contains("outputs"))for(const auto& output:input.at("outputs")) {
        const auto expected=unhex(output.at(1));
        check(hex(memory.view(output.at(0),expected.size()))==hex(expected),"Tracking constructor prefix stores or preserved padding differ");
    }
}
}
int main(int argc,char** argv)try {
    check(argc==4,"Expected independent original callback selector, object tracking and dispatch references");
    nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
    check(fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&
        fixture.at("instruction_patches").empty()&&fixture.at("supplied_dependency_returns").empty(),"Callback selector original authority differs");
    fields(fixture);for(const auto& input:fixture.at("cases"))run_case(fixture,input);
    nlohmann::json tracking;std::ifstream(argv[2])>>tracking;
    check(tracking.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&tracking.at("instruction_patches").empty()&&
        tracking.at("supplied_dependency_returns").empty(),"Tracking original authority differs");
    SourceRuntimeMemory memory;mount_native_tu15_final_startup_storage_data(memory);
    const auto snapshot=memory.checkpoint();
    for(const auto& row:tracking.at("rows")) {
        const unsigned address=row.at("address");
        check(hex(memory.view(address,8))==row.at("data_hex").get<std::string>()&&memory.is_read_only(address,8),
            "Original tracking fields or access differ");
        const auto owner=std::ranges::find(snapshot.regions,address,&SourceRuntimeMemory::CheckpointRegion::address);
        check(owner!=snapshot.regions.end()&&owner->bytes.size()==8,"Tracking declaration borrowed row ID or unread tail");
    }
    for(const auto& input:tracking.at("cases"))run_case(tracking,input);
    nlohmann::json dispatch;std::ifstream(argv[3])>>dispatch;
    check(dispatch.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&dispatch.at("instruction_patches").empty()&&
        dispatch.at("supplied_dependency_returns").empty(),"Tracking dispatch original authority differs");
    for(const auto& row:dispatch.at("rows")) {
        const unsigned address=row.at("address");
        check(hex(memory.view(address,8))==row.at("data_hex").get<std::string>()&&memory.is_read_only(address,8),
            "Original tracking callback identities or access differ");
        const auto owner=std::ranges::find(snapshot.regions,address,&SourceRuntimeMemory::CheckpointRegion::address);
        check(owner!=snapshot.regions.end()&&owner->bytes.size()==8,"Tracking dispatch declaration borrowed row headers");
    }
    std::cout<<"PASS "<<fixture.at("cases").size()<<" original callback selector whole/prefix cases, complete ABI, "
        "instruction/store paths,55 immutable kind2 tails, exact extents and alias controls, "<<tracking.at("cases").size()<<
        " original tracking predicate/constructor-prefix cases; later callbacks remain unqualified\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
