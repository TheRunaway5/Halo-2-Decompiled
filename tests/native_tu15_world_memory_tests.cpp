#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <ranges>
#include <tuple>
#include <string_view>

namespace {
using namespace halo2;
void check(bool condition,const char* message) {
    if(!condition)throw std::runtime_error(message);
}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    std::uint32_t value=0;const auto bytes=memory.view(address,4);
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
    return value;
}
}
int main(int argc,char** argv)try {
    constexpr std::uint32_t tick=0x60212000u;
    SourceRuntimeKernelTickCount input{tick,{std::byte{0x78},std::byte{0x56},std::byte{0x34},std::byte{0x12}},
        "world memory test retained clock"};
    auto memory=native_tu15_world_memory(input);
    check(word(memory,0x41b774)==tick && word(memory,tick)==0x12345678u,
        "Native tick-count data was not bound to its actual owner");
    check(memory.is_read_only(0x41b774,4) && !memory.is_read_only(tick,4),
        "Tick-count binding or volatile data access differs");
    check(memory.view(word(memory,0x474c70u),12).size()==12,
        "Original scale/color vector cannot be read by its complete native owner");
    input.bytes.fill(std::byte{});
    check(word(memory,tick)==0x12345678u,"World memory borrowed expired caller bytes");
    for(const auto cell:{0x56ee28u,0x4f3218u,0x4f3cd8u,0x51df54u,
                         0x5020c4u,0x4f54f0u,0x51df44u,0x515ec0u,
                         0x56ebacu,0x56e684u,0x502088u,0x56b170u})
        check(!word(memory,cell),
            "Cold composition fabricated a completed constructor result");
    for(const auto cell:{0x4f0430u,0x4922a8u,0x48d480u})
        check(memory.view(cell,1).front()==std::byte{},
            "Cold composition fabricated successful initialization");
    bool missing=false;
    try{(void)memory.view(0x10000,4);}catch(const std::out_of_range&){missing=true;}
    check(missing,"Cold composition included an executable header");
    missing=false;
    try{(void)memory.view(0x13a700,4);}catch(const std::out_of_range&){missing=true;}
    check(missing,"Cold composition included executable instructions");
    std::size_t regions=0,bytes=0;
    const auto checkpoint=memory.checkpoint();
    const auto type_owner=std::ranges::find(checkpoint.regions,0x5020b4u,
        &SourceRuntimeMemory::CheckpointRegion::address);
    check(type_owner!=checkpoint.regions.end() && type_owner->bytes.size()==15u &&
        type_owner->access==SourceRuntimeMemory::Access::ReadWrite &&
        type_owner->owner=="native TU1.5 cold complete 1E7690 object-type runtime flags" &&
        !type_owner->page_bytes && type_owner->committed.empty(),
        "Complete native object-type runtime owner is absent or has an invalid schema");
    check(std::ranges::all_of(type_owner->bytes,[](auto value){return value==std::byte{};}),
        "Native object-type runtime flags were warmed before the source writer");
    check(std::ranges::none_of(checkpoint.regions,[](const auto&region){return region.address==0x5020b8u;}),
        "Old one-byte member remained separately mounted inside the complete array");
    native_tu15_validate_type_runtime_restore(memory,checkpoint);
    unsigned rejected_type_schemas{};
    for(unsigned fault=0;fault<9;++fault) {
        auto invalid=checkpoint;
        const auto owner=std::ranges::find(invalid.regions,0x5020b4u,
            &SourceRuntimeMemory::CheckpointRegion::address);
        switch(fault) {
        case 0:invalid.regions.erase(owner);break;
        case 1:owner->bytes.pop_back();break;
        case 2:owner->bytes.push_back(std::byte{});break;
        case 3:owner->access=SourceRuntimeMemory::Access::ReadOnly;break;
        case 4:owner->owner+=" invalid";break;
        case 5:owner->page_bytes=4096;break;
        case 6:owner->committed={1};break;
        case 7:owner->address=0x5020b8u;owner->bytes={std::byte{}};
            owner->owner="native TU1.5 cold 102A54 scene placement policy byte";break;
        case 8:owner->address+=1;break;
        }
        bool rejected=false;
        try {native_tu15_validate_type_runtime_restore(memory,invalid);}
        catch(const std::exception&error) {
            rejected=std::string(error.what())=="TU1.5 authority lacks its complete object-type runtime owner";
        }
        check(rejected,"Native restore admitted a missing, malformed or old partial type-runtime owner");
        check(memory.checkpoint()==checkpoint,"Rejected type-runtime schema mutated the current factory bank");
        ++rejected_type_schemas;
    }
    // This isolated whole-section topology has no native15-byte declaration;
    // the surrounding authority keeps its existing offline-image admission.
    SourceRuntimeMemory whole_section;
    whole_section.mount(0x502000u,std::vector<std::byte>(4096),
        SourceRuntimeMemory::Access::ReadWrite,"isolated whole-section schema control");
    native_tu15_validate_type_runtime_restore(whole_section,whole_section.checkpoint());
    std::cout<<"Native object-type runtime restore schemas: "<<rejected_type_schemas
        <<" rejected, complete factory owner accepted; offline whole-section route preserved\n";

    for(const auto& region:checkpoint.regions) {
        check(!region.owner.empty(),"World declaration has no retained owner identity");
        check(memory.native_address(region.address,region.bytes.size()).value>UINT32_MAX,
            "World data was narrowed to a 32-bit host address");
        ++regions;bytes+=region.bytes.size();
    }
    // Optional offline audit compares individually declared data against the
    // authenticated TU1.5 reference. The production factory accepts no path.
    if(argc==2) {
        auto original=load_source_runtime_image(argv[1],SourcePhysicsContactImage::TitleUpdate15);
        for(const auto& region:checkpoint.regions) {
            if(region.address==tick || region.address==0x41b774)continue;
            if(!std::ranges::equal(memory.view(region.address,region.bytes.size()),
                original.view(region.address,region.bytes.size()))) {
                std::cerr<<"Reference differs at 0x"<<std::hex<<region.address<<" "<<region.owner<<'\n';
                return 1;
            }
        }
    }else check(argc==1,"Expected only optional offline reference path");
    const auto level_owner=std::ranges::find(checkpoint.regions,0x4fadb8u,
        &SourceRuntimeMemory::CheckpointRegion::address);
    check(level_owner!=checkpoint.regions.end() && level_owner->bytes.size()==0x208u &&
        level_owner->access==SourceRuntimeMemory::Access::ReadWrite &&
        level_owner->owner=="source 64 level-handle pairs and two pool constructor bindings",
        "Complete level-handle source owner is absent or has a partial schema");
    check(std::ranges::all_of(level_owner->bytes,[](auto value){return value==std::byte{};}),
        "Cold level-handle backing fabricated a source constructor result");
    unsigned bounded_name_cases{};
    for(const auto& [address,name,extent]:std::array{
        std::tuple{0x4607ecu,std::string_view{"level handles"},16u},
        std::tuple{0x4607d0u,std::string_view{"multiplayer level handles"},28u}}) {
        check(memory.is_read_only(address,extent),"Level-handle name extent is not immutable");
        const auto owner=std::ranges::find(checkpoint.regions,address,
            &SourceRuntimeMemory::CheckpointRegion::address);
        check(owner!=checkpoint.regions.end() && owner->bytes.size()==extent,
            "Level-handle pool name has an inexact owned extent");
        check(memory.view(0x4607ecu+15u,1).front()==std::byte{},
            "Level-handle original post-NUL DWORD byte differs");
        bool read_only{};try{memory.mutable_view(address,1);}catch(const std::exception&){read_only=true;}
        check(read_only,"Authored pool name accepted a write");
        constexpr std::uint32_t stack=0x61008000u,destination=0x61010000u,stop=0x70000000u;
        for(const auto count:std::array{0u,1u,unsigned(name.size()+1u),32u}) {
            SourceRuntimeMemory call_bank;call_bank.restore(checkpoint);
            call_bank.mount(stack-4096u,std::vector<std::byte>(8192),SourceRuntimeMemory::Access::ReadWrite,
                "isolated bounded pool-name actual caller stack");
            call_bank.mount(destination,std::vector<std::byte>(64,std::byte{0xa5}),SourceRuntimeMemory::Access::ReadWrite,
                "isolated bounded pool-name caller destination");
            const auto put=[&](auto p,auto value){std::array<std::byte,4> raw{};
                for(unsigned i=0;i<4;++i)raw[i]=std::byte(value>>(8*i));call_bank.write(p,raw);};
            put(stack,stop);put(stack+4,destination);put(stack+8,address);put(stack+12,count);
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=stack;abi.eflags=0x202;
            source_run_physics_live_support_tu15(call_bank,0x32a4e0u,abi,{0x27f,0x1f80},{});
            check(abi.gp[0]==destination && abi.gp[4]==stack+4,
                "Actual bounded pool-name copy returned a different caller frame");
            const auto output=call_bank.view(destination,64);
            for(unsigned i=0;i<64;++i) {
                const auto expected=i>=count?std::byte{0xa5}:i<name.size()?std::byte(name[i]):std::byte{};
                check(output[i]==expected,"Actual bounded pool-name copy differs from source string/zero padding");
            }
            ++bounded_name_cases;
        }
    }
    std::cout<<"Native bounded level-handle name controls: "<<bounded_name_cases<<" cases\n";
    for(const auto& bad:std::array{
        SourceRuntimeKernelTickCount{0x1000,{},"invalid low clock"},
        SourceRuntimeKernelTickCount{0xffffffffu,{},"invalid wrapped clock"},
        SourceRuntimeKernelTickCount{tick,{},""}}) {
        bool rejected=false;
        try{(void)native_tu15_world_memory(bad);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected,"Invalid native tick-count owner was admitted");
    }
    std::cout<<"Native TU1.5 world cold declarations: "<<regions<<" regions, "<<bytes<<" bytes\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
