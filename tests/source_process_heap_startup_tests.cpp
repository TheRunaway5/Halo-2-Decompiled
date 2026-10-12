#include "halo2/source_process_heap_platform.hpp"
#include "halo2/source_runtime_image.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace {
using J=nlohmann::json;
using M=halo2::SourceRuntimeMemory;
void check(bool value,const std::string &why) { if(!value)throw std::runtime_error(why); }
std::vector<std::byte> bytes(const std::string &s) {
    check(!(s.size()&1),"Invalid original byte capture");std::vector<std::byte> b;
    for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));
    return b;
}
void put(M &m,std::uint32_t p,std::uint32_t v) {
    std::array<std::byte,4> b;
    for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(i*8));m.write(p,b);
}
struct Boundary {};
}
int main(int argc,char **argv) {
    try {
        check(argc==4,"Supply original startup fixture and both selected images");
        for(unsigned i=1;i<4;++i)if(!std::filesystem::exists(argv[i]))return 77;
#if !defined(__linux__)
        return 77;
#endif
        std::ifstream in(argv[1]);const auto fixture=J::parse(in);
        const bool joined=fixture.contains("compiled_compatibility_registry") &&
            !fixture.at("compiled_compatibility_registry").is_null();
        const bool initializers=fixture.contains("compiled_compatibility_initializers");
        std::size_t cases=0,stores=0,platform_stores=0,instructions=0;
        for(const auto &row:fixture.at("cases")) {
            const auto name=row.at("image").get<std::string>()+"/"+std::to_string(row.at("entry").get<std::uint32_t>());
            const bool later=row.at("image")=="1.5";
            const auto image=later?halo2::SourcePhysicsContactImage::TitleUpdate15:halo2::SourcePhysicsContactImage::MapPack11;
            auto memory=halo2::load_source_runtime_image(argv[later?3:2],image);
            memory.mount(fixture.at("stack_address"),bytes(fixture.at("initial_stack_hex")),M::Access::ReadWrite,
                "explicit standalone patterned source stack input");
            memory.mount(0,std::vector<std::byte>(0x1000),M::Access::ReadWrite,"explicit standalone FS exception-chain input");
            put(memory,0,0xffffffff);put(memory,row.at("stack_pointer"),0x3000000);
            halo2::SourceRuntimeHostVirtualMemory vm(memory);
            halo2::PhysicsPlatformLocks locks;
            std::unique_ptr<halo2::SourceProcessShutdownRegistry> registry;
            if(joined)registry=std::make_unique<halo2::SourceProcessShutdownRegistry>(memory,
                fixture.at("compiled_compatibility_registry").at("head"));
            halo2::SourcePhysicsContactRuntimeAbi abi{};
            const auto &before=row.at("before");
            abi.gp=before.at("gp").get<decltype(abi.gp)>();
            abi.xmm=before.at("xmm").get<decltype(abi.xmm)>();
            abi.eflags=before.at("eflags");abi.fs_base=before.at("fs_base");abi.x87_status=before.at("x87_status");
            abi.floating_point=halo2::SourcePhysicsFactoryFloatingPoint{before.at("x87_control_word"),before.at("mxcsr")};
            halo2::SourcePhysicsContactRuntimeServices services;
            std::size_t trace=0,store=0,platform_store=0,boundaries=0;
            services.instruction=[&](auto p) {
                check(trace<row.at("trace").size() && p==row.at("trace").at(trace++),name+" original instruction trace differs");
            };
            const auto observe=[&](const auto &actual) {
                check(store<row.at("stores").size(),name+" unexpected game store");
                const auto &expected=row.at("stores").at(store++);
                check(actual.instruction==expected.at("instruction") && actual.address==expected.at("address") &&
                    actual.before==bytes(expected.at("before")) && actual.written==bytes(expected.at("written")),
                    name+" original ordered game store differs at "+std::to_string(store-1));
            };
            std::vector<J> expected_platform;
            if(joined)for(const auto &call:row.at("platform_calls"))
                for(const auto &change:call.at("stores"))expected_platform.push_back(change);
            const auto platform_observe=[&](const auto &actual) {
                check(platform_store<expected_platform.size(),name+" unexpected native platform store");
                const auto &expected=expected_platform.at(platform_store++);
                check(actual.address==expected.at("address") && actual.before==bytes(expected.at("before")) &&
                    actual.written==bytes(expected.at("written")),name+" compiled primary platform store differs");
            };
            services.required_owner=[&](auto &,const auto &call,auto &state,const auto &) {
                // Historical fixtures deliberately stop before107. Keep their
                // original owner set instead of changing immutable captures.
                if(!initializers && registry && halo2::source_try_native_process_shutdown_call(
                    memory,call,state,image,*registry,platform_observe))return;
                const auto &expected=row.at("boundary");
                check(++boundaries==1 && call.instruction==expected.at("instruction") &&
                    call.target==expected.at("target") && call.tail==expected.at("tail"),name+" required callee differs");
                const auto frame=bytes(expected.at("frame"));
                check(state.gp[4]==expected.at("frame_address") &&
                    std::ranges::equal(memory.view(state.gp[4],frame.size()),frame),name+" live pending frame differs");
                throw Boundary{};
            };
            const halo2::PhysicsPlatformLockServices forbidden{
                []()->std::uint32_t { throw std::runtime_error("Startup prefix requested thread identity"); },
                [](auto) { throw std::runtime_error("Startup prefix requested wait"); },
                [](auto) { throw std::runtime_error("Startup prefix requested wake"); }};
            bool stopped=false;
            try {
                if(initializers)halo2::source_run_process_heap_with_native_platform(memory,row.at("entry"),abi,image,
                    {0x27f,0x1f80},services,vm,locks,forbidden,observe,{}, {},registry.get(),platform_observe,platform_observe);
                else halo2::source_run_process_heap_runtime(memory,row.at("entry"),abi,image,{0x27f,0x1f80},services,observe);
            } catch(const Boundary &) { stopped=true; }
            catch(const halo2::SourceProcessHeapDataUnavailable &e) {
                const auto &expected=row.at("boundary");const auto &actual=e.request;
                check(initializers && ++boundaries==1 && expected.at("kind")=="imported-data" &&
                    actual.instruction==expected.at("instruction") && actual.actual_instruction==expected.at("actual_instruction") &&
                    actual.cell==expected.at("cell") && actual.ordinal==expected.at("ordinal") && actual.bytes==expected.at("bytes"),
                    name+" pre-load imported-data owner differs");
                const auto frame=bytes(expected.at("frame"));
                check(abi.gp[4]==expected.at("frame_address") &&
                    std::ranges::equal(memory.view(abi.gp[4],frame.size()),frame),name+" pending data-load stack differs");
                stopped=true;
            }
            check(stopped && boundaries==1 && trace==row.at("trace").size() && store==row.at("stores").size() &&
                platform_store==expected_platform.size(),name+" incomplete original prefix/boundary comparison");
            const auto &after=row.at("after");
            check(abi.gp==after.at("gp").get<decltype(abi.gp)>() && abi.xmm==after.at("xmm").get<decltype(abi.xmm)>(),
                name+" live GP/XMM state differs");
            check(((abi.eflags^after.at("eflags").get<std::uint32_t>())&0xcd5u)==0 && abi.fs_base==after.at("fs_base") &&
                abi.x87_depth==0 && abi.x87_status==after.at("x87_status") && !abi.active_mmx && abi.floating_point &&
                abi.floating_point->x87_control_word==after.at("x87_control_word") && abi.floating_point->mxcsr==after.at("mxcsr"),
                name+" live flags/FS/FP state differs");
            for(const auto &bank:row.at("final_banks")) {
                const auto expected=bytes(bank.at("hex"));
                check(std::ranges::equal(memory.view(bank.at("address"),expected.size()),expected),name+" retained original bank differs");
            }
            ++cases;stores+=store;platform_stores+=platform_store;instructions+=trace;
        }
        std::cout<<cases<<" original cases across both images, "<<instructions<<" executed instructions, "<<stores
            <<" ordered game stores, "<<platform_stores<<" compiled primary platform stores and pending full ABI match\n";
    } catch(const std::exception &e) { std::cerr<<e.what()<<'\n';return 1; }
}
