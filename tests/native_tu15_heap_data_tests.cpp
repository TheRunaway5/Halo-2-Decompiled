#include "halo2/native_tu15_heap_data.hpp"
#include "halo2/native_tu15_heap_bootstrap.hpp"
#include "halo2/source_process_heap_platform.hpp"
#include "halo2/source_runtime_image.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace {
using Memory=halo2::SourceRuntimeMemory;
using Json=nlohmann::json;
constexpr std::uint32_t stack=0x60000000,sp=stack+0xf0000,fs=0x60200000,parameters=fs+0x1000;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void put(Memory& m,std::uint32_t p,std::uint32_t v) {
    std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(v>>(8*i));m.write(p,bytes);
}
std::uint32_t word(const Memory& m,std::uint32_t p) {
    const auto bytes=m.view(p,4);std::uint32_t value{};
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);return value;
}
std::string hex(std::span<const std::byte> bytes) {
    std::string result;constexpr char digits[]="0123456789abcdef";
    for(auto b:bytes){const auto n=std::to_integer<unsigned>(b);result+=digits[n>>4];result+=digits[n&15];}return result;
}
struct Case {const char* name;std::uint32_t flags,reserve,commit;bool parameter_record;};
struct Result {
    halo2::SourcePhysicsContactRuntimeAbi abi{};
    std::vector<halo2::SourcePhysicsContactRecordStore> stores;
    std::vector<halo2::PhysicsPlatformLockStore> lock_stores;
    std::vector<std::uint32_t> instructions;
    std::vector<std::array<std::uint32_t,10>> resources;
    std::map<std::uint32_t,std::vector<std::byte>> outputs;
    Json receipt;
};
Result construct(Memory& memory,const Case& input) {
    Result result;
    memory.mount(stack,std::vector<std::byte>(0x100000),Memory::Access::ReadWrite,"native heap test caller stack");
    memory.mount(fs,std::vector<std::byte>(0x2000),Memory::Access::ReadWrite,"native heap test FS and parameters");
    put(memory,fs,0xffffffffu);put(memory,parameters,0x30);put(memory,sp,0x73000000u);
    auto args=halo2::native_tu15_process_heap_arguments(input.parameter_record?parameters:0);
    args[0]=input.flags;args[2]=input.reserve;args[3]=input.commit;
    for(unsigned i=0;i<args.size();++i)put(memory,sp+4+i*4,args[i]);
    halo2::SourceRuntimeHostVirtualMemory vm(memory,0x08000000u);
    halo2::PhysicsPlatformLocks locks;
    result.abi.gp={0x11223344,0x22334455,0x33445566,0x44556677,sp,0x66778899,0x778899aa,0x8899aabb};
    result.abi.eflags=2;result.abi.fs_base=fs;result.abi.floating_point=halo2::SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    result.receipt={{"name",input.name},{"args",args},{"initial_gp",result.abi.gp},{"stack",stack},{"sp",sp},{"fs",fs}};
    halo2::SourcePhysicsContactRuntimeServices services;
    services.instruction=[&](auto instruction){result.instructions.push_back(instruction);};
    services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Native heap constructor reached an undeclared owner");};
    const halo2::PhysicsPlatformLockServices forbidden{
        []()->std::uint32_t{throw std::runtime_error("Heap construction queried a thread");},
        [](auto){throw std::runtime_error("Heap construction waited");},
        [](auto){throw std::runtime_error("Heap construction woke a thread");}};
    halo2::source_run_process_heap_with_native_platform(memory,0x2da83a,result.abi,
        halo2::SourcePhysicsContactImage::TitleUpdate15,{0x27f,0x1f80},services,vm,locks,forbidden,
        [&](const auto& store){result.stores.push_back(store);},
        [&](const auto& call){result.resources.push_back({call.instruction,call.base_cell,call.size_cell,
            call.request.base,call.request.zero_bits,call.request.bytes,call.request.allocation_type,
            call.request.protection,call.result.base,call.result.bytes});},
        [&](const auto& store){result.lock_stores.push_back(store);});
    check(result.abi.gp[0]==0x08000000u,"Native heap constructor did not publish its actual reserved heap");
    check(result.abi.gp[4]==sp+28,"Heap constructor lost its retained stdcall frame");
    const auto owner=vm.ownership_checkpoint();check(owner.reservations.size()==1,"Heap has no single actual VM owner");
    memory.visit_committed([&](auto p,auto bytes) {
        if(p==stack || p==fs || (p>=0x08000000u && p<0x10000000u))
            result.outputs.emplace(p,std::vector<std::byte>(bytes.begin(),bytes.end()));
    });
    check(memory.native_address(0x08000000,0x1000).value>0xffffffffu,"Heap backing was narrowed into a source pointer");
    auto& receipt=result.receipt;
    receipt["returned_gp"]=result.abi.gp;receipt["returned_eflags"]=result.abi.eflags;
    receipt["instructions"]=result.instructions;receipt["resources"]=result.resources;
    receipt["stores"]=Json::array();receipt["lock_stores"]=Json::array();receipt["outputs"]=Json::array();
    for(const auto& s:result.stores)receipt["stores"].push_back({{"instruction",s.instruction},{"address",s.address},{"before",hex(s.before)},{"written",hex(s.written)}});
    for(const auto& s:result.lock_stores)receipt["lock_stores"].push_back({{"address",s.address},{"before",hex(s.before)},{"written",hex(s.written)}});
    // Retain only the actual committed heap and the bounded touched caller
    // frame; these diagnostic outputs are never native startup inputs.
    for(const auto& [p,bytes]:result.outputs) {
        if(p==stack)receipt["outputs"].push_back({{"address",sp-0x200},{"hex",hex(std::span(bytes).subspan(sp-stack-0x200,0x300))}});
        else if(p==fs)receipt["outputs"].push_back({{"address",fs},{"hex",hex(std::span(bytes).first(4))}});
        else receipt["outputs"].push_back({{"address",p},{"hex",hex(bytes)}});
    }
    return result;
}
void same(const Result& a,const Result& b) {
    check(a.abi.gp==b.abi.gp && a.abi.xmm==b.abi.xmm && a.abi.eflags==b.abi.eflags &&
          a.abi.fs_base==b.abi.fs_base && a.abi.x87_depth==b.abi.x87_depth && a.abi.x87_status==b.abi.x87_status,
          "Typed native heap operands changed returned CPU state");
    check(a.stores==b.stores && a.lock_stores==b.lock_stores && a.instructions==b.instructions &&
          a.resources==b.resources && a.outputs==b.outputs,"Typed native heap operands changed construction or actual resource ownership");
}
}
int main(int argc,char** argv)try {
#if !defined(__linux__)
    return 77;
#endif
    std::filesystem::path reference,capture;
    for(int i=1;i<argc;i+=2) {
        check(i+1<argc,"Missing heap test argument");
        if(std::string_view(argv[i])=="--reference-xbe")reference=argv[i+1];
        else if(std::string_view(argv[i])=="--capture")capture=argv[i+1];
        else throw std::runtime_error("Unknown heap test argument");
    }
    if(!reference.empty() && !std::filesystem::exists(reference))return 77;
    Json report{{"schema","H2_NATIVE_TU15_HEAP_BOOTSTRAP_V1"},{"world_startup_complete",false},
        {"reference_file_required_for_native_factory",false},{"native_platform_abi",true},{"cases",Json::array()}};
    auto operands=halo2::native_tu15_process_heap_memory();std::size_t bytes=0;
    operands.visit_committed([&](auto,auto b){bytes+=b.size();});check(bytes==54,"Native heap data includes unclassified storage");
    if(!reference.empty()) {
        const auto original=halo2::load_source_runtime_image(reference,halo2::SourcePhysicsContactImage::TitleUpdate15);
        operands.visit_committed([&](auto p,auto b){check(std::ranges::equal(b,original.view(p,b.size())),"Native heap initializer differs from the authenticated reference");});
        const auto args=halo2::native_tu15_process_heap_arguments(parameters);
        check(args[2]==word(original,0x10134) && args[3]==word(original,0x10138),
              "Native heap caller extents differ from their recovered source inputs");
    }
    constexpr std::array cases{
        Case{"startup",2,0x100000,0x1000,true},Case{"startup_null_parameters",2,0x100000,0x1000,false},
        Case{"default_extents",2,0,0,true},Case{"rounded_extents",2,0x12345,0x1234,true},
        Case{"no_serialization",1,0x100000,0x1000,true},Case{"no_serialization_defaults",1,0,0,false}};
    for(const auto& input:cases) {
        auto memory=halo2::native_tu15_process_heap_memory();const auto actual=construct(memory,input);
        if(!reference.empty()) {
            auto original=halo2::load_source_runtime_image(reference,halo2::SourcePhysicsContactImage::TitleUpdate15);
            same(actual,construct(original,input));
        }
        report["cases"].push_back(actual.receipt);
    }
    {
        halo2::NativeTu15HeapBootstrap startup,other;
        check(!startup.heap().is_null() && startup.heap().value>0xffffffffu,"Native process heap has no checked 64-bit live address");
        check(word(startup.memory(),0x56ee28)==startup.memory().source_address(startup.heap(),0x1000),
              "Native startup failed to publish its completed heap owner");
        check(word(startup.memory(),fs)==0xffffffffu,"Native process heap failed to retire its exception frame");
        startup.memory().view(startup.heap(),0x1000);
        bool rejected=false;
        try{startup.memory().source_address(other.heap(),0x1000);}catch(const std::exception&){rejected=true;}
        check(rejected,"Native process startup accepted another heap owner's host address");
        rejected=false;
        try{put(startup.memory(),0x46e2c0u,0);}catch(const std::exception&){rejected=true;}
        check(rejected,"Native process startup made immutable heap defaults writable");
    }
    if(!capture.empty()) {std::filesystem::create_directories(capture.parent_path());std::ofstream out(capture);out<<report.dump(2)<<'\n';check(bool(out),"Cannot write heap bootstrap evidence");}
    std::cout<<"Native TU1.5 heap bootstrap: "<<cases.size()<<" complete constructors and published heap; 54 typed cold-data bytes; real VM/lock owners; "
        <<(reference.empty()?"no executable input":"authenticated reference data/state comparisons passed")<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
