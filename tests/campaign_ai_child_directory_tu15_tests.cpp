#include "halo2/source_ai_actor_tu15.hpp"
#include "halo2/source_final_startup_storage_tu15.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace {
using namespace halo2;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::vector<std::byte> unhex(const std::string& text) {
    check(text.size()%2==0,"Odd directory reference bytes");std::vector<std::byte> result;
    for(unsigned i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
    return result;
}
std::string hex(std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string result;
    for(auto b:bytes){auto v=std::to_integer<unsigned>(b);result+=digits[v>>4];result+=digits[v&15];}
    return result;
}
void run_case(const nlohmann::json& fixture,const nlohmann::json& input) {
    SourceRuntimeMemory memory;mount_native_tu15_final_startup_storage_data(memory);
    for(const auto& range:fixture.at("selection_ranges")) {
        const unsigned address=range.at(0);const auto expected=unhex(range.at(1));
        std::vector<std::byte> actual(expected.size());memory.read(address,actual);
        check(actual==expected,"Original property-child selection input changed");
        for(unsigned i=0;i<actual.size();++i)check(memory.is_read_only(address+i,1),
            "Property-child input became writable");
    }
    for(const auto& style:fixture.at("styles"))for(const auto& entry:style.at("entries"))
        for(const auto& command:entry.at("commands")) {
            const unsigned child=command.at("child_address");
            for(unsigned offset:{6u,7u,12u,13u,14u,15u,16u,17u,18u,19u}) {
                bool hole=false;try{(void)memory.view(child+offset,1);}catch(const std::out_of_range&){hole=true;}
                check(hole,"Property-child input borrowed unread padding or tail");
            }
        }
    for(const auto& bank:fixture.at("native_component_banks"))memory.mount(bank.at(0),
        std::vector<std::byte>(bank.at(1).get<unsigned>()),SourceRuntimeMemory::Access::ReadWrite,
        "Declared original child-directory caller input");
    for(const auto& patch:input.at("patches"))memory.write(patch.at(0),unhex(patch.at(1)));
    for(const auto& output:input.at("outputs")) {
        const auto bytes=unhex(output.at(1));
        memory.write(output.at(0),std::vector<std::byte>(bytes.size(),
            std::byte(input.value("poison",0xa5u))));
    }
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=input.at("gp").get<decltype(abi.gp)>();
    abi.xmm=input.at("xmm").get<decltype(abi.xmm)>();abi.eflags=0x202;
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    std::vector<unsigned> trace;nlohmann::json stores=nlohmann::json::array();
    SourcePhysicsWorldRuntimeServices services{};
    services.instruction=[&](unsigned pc){trace.push_back(pc);};
    services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Child-directory producer delegated an owner");};
    source_run_ai_actor_tu15(memory,input.at("entry"),abi,*abi.floating_point,services,[&](const auto& store){
        stores.push_back(nlohmann::json::array({store.instruction,store.address,hex(store.before),hex(store.written)}));});
    check(abi.gp==input.at("final_gp").get<decltype(abi.gp)>(),"Directory producer complete GPR state differs");
    check(abi.xmm==input.at("final_xmm").get<decltype(abi.xmm)>(),"Directory producer SIMD state differs");
    check((abi.eflags&~0x10u)==(input.at("final_flags").get<unsigned>()&~0x10u),"Directory producer flags differ");
    check(trace==input.at("trace").get<std::vector<unsigned>>(),"Directory producer original instruction path differs");
    check(stores==input.at("stores"),"Directory producer ordered stores, prior bytes or caller stack differ");
    for(const auto& output:input.at("outputs")) {
        const auto expected=unhex(output.at(1));std::vector<std::byte> actual(expected.size());
        memory.read(output.at(0),actual);
        check(actual==expected,"Directory producer pointers, compact headers or preserved padding differ");
    }
    const auto saved=memory.checkpoint();
    for(const auto& output:input.at("outputs"))memory.write(output.at(0),
        std::vector<std::byte>(unhex(output.at(1)).size(),std::byte{0x42}));
    memory.restore(saved);
    for(const auto& output:input.at("outputs")) {
        const auto expected=unhex(output.at(1));std::vector<std::byte> actual(expected.size());memory.read(output.at(0),actual);
        check(actual==expected,"Whole-bank restore lost child-directory pointers or header padding");
    }
}
void check_dispatch_data(const nlohmann::json& fixture) {
    SourceRuntimeMemory memory;mount_native_tu15_final_startup_storage_data(memory);
    for(const auto& table:fixture.at("tables")) {
        const unsigned address=table.at("address"),width=table.at("width"),count=table.at("count");
        const auto expected=unhex(table.at("data_hex"));check(expected.size()==width*count,"Dispatch reference extent differs");
        std::vector<std::byte> actual(expected.size());memory.read(address,actual);
        check(actual==expected,"Original AI dispatch bytes differ");
        for(unsigned i=0;i<expected.size();++i)check(memory.is_read_only(address+i,1),"AI dispatch data became writable");
    }
    bool end_hole=false;try{(void)memory.view(0x1fc100u,1);}catch(const std::out_of_range&){end_hole=true;}
    check(end_hole,"Reached six-lane AI switch borrowed the following function bytes");
    const auto first=unhex("b3bd1f00");
    SourceRuntimeMemory partial;
    partial.mount(0x1fc0e8u,first,SourceRuntimeMemory::Access::ReadOnly,"Retained original AI switch first lane");
    mount_native_tu15_final_startup_storage_data(partial);
    const auto snapshot=partial.checkpoint();
    const auto owner=std::ranges::find(snapshot.regions,0x1fc0e8u,&SourceRuntimeMemory::CheckpointRegion::address);
    check(owner!=snapshot.regions.end()&&owner->owner=="Retained original AI switch first lane"&&
        owner->bytes.size()==4&&hex(partial.view(0x1fc0e8u,4))=="b3bd1f00",
        "AI dispatch installation replaced a compatible existing lane");
    for(bool writable:{false,true}) {
        SourceRuntimeMemory conflict;auto bytes=first;if(!writable)bytes[0]=std::byte{0xb2};
        conflict.mount(0x1fc0e8u,bytes,writable?SourceRuntimeMemory::Access::ReadWrite:
            SourceRuntimeMemory::Access::ReadOnly,"Retained conflicting AI switch lane");
        bool rejected=false;
        try{mount_native_tu15_final_startup_storage_data(conflict);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected&&hex(conflict.view(0x1fc0e8u,4))==hex(bytes),"AI dispatch installation repaired an incompatible alias");
    }
}
}
int main(int argc,char** argv)try {
    check(argc==3,"Expected independent original property/directory and dispatch references");
    nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
    check(fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&
        fixture.at("instruction_patches").empty()&&fixture.at("supplied_dependency_returns").empty(),
        "Property/directory original authority differs");
    nlohmann::json dispatch;std::ifstream(argv[2])>>dispatch;
    check(dispatch.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&
        dispatch.at("instruction_patches").empty()&&dispatch.at("supplied_dependency_returns").empty(),
        "AI dispatch original authority differs");
    check_dispatch_data(dispatch);
    for(const auto& input:fixture.at("cases"))run_case(fixture,input);
    // A separately retained alias must never be repaired into a successful
    // child operand, even when its bytes happen to match but it is writable.
    for(bool writable:{false,true}) {
        SourceRuntimeMemory conflict;
        const auto bytes=unhex(writable?"7800":"7900");
        conflict.mount(0x47d3cc,bytes,writable?SourceRuntimeMemory::Access::ReadWrite:
            SourceRuntimeMemory::Access::ReadOnly,"Retained conflicting property-child owner");
        bool rejected=false;
        try{mount_native_tu15_final_startup_storage_data(conflict);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected&&hex(conflict.view(0x47d3cc,2))==hex(bytes),"Property-child owner replaced a conflicting alias");
    }
    std::cout<<"PASS "<<fixture.at("cases").size()<<" original authored-property and whole-directory cases, "
        "GPR/SIMD/flags, all instruction/store paths, prior padding, checkpoint restore and "
        <<dispatch.at("tables").size()<<" immutable AI switches\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
