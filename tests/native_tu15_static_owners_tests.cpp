#include "halo2/native_tu15_static_owners.hpp"
#include "halo2/native_tu15_arena_bootstrap.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_runtime_image.hpp"
#include <openssl/evp.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#if defined(__linux__)
#include <unistd.h>
#endif

namespace {
using namespace halo2;
using Json=nlohmann::json;
void check(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
void put(SourcePhysicsContactMemory& memory,std::uint32_t address,std::uint32_t value) {
    std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));memory.write(address,bytes);
}
std::uint32_t word(const SourcePhysicsContactMemory& memory,std::uint32_t address) {
    std::array<std::byte,4> bytes{};memory.read(address,bytes);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);return value;
}
std::string hex(std::span<const std::byte> bytes) {
    std::ostringstream out;for(auto b:bytes)out<<std::hex<<std::setfill('0')<<std::setw(2)<<std::to_integer<unsigned>(b);return out.str();
}
std::string digest(std::span<const std::byte> bytes) {
    std::array<unsigned char,EVP_MAX_MD_SIZE> result{};unsigned size=0;
    check(EVP_Digest(bytes.data(),bytes.size(),result.data(),&size,EVP_sha256(),nullptr)==1,"SHA256 unavailable");
    return hex(std::as_bytes(std::span(result).first(size)));
}
struct Directory {
    std::filesystem::path path;
    Directory(){
#if defined(__linux__)
        std::string name="/tmp/halo2-native-static-XXXXXX";check(::mkdtemp(name.data()),"Cannot create native startup test directory");path=name;
#else
        throw std::runtime_error("Native startup test file owner unavailable");
#endif
    }
    ~Directory(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
Json buffers(const SourceRuntimeMemory& memory,const SourceRuntimeMemory& declarations) {
    Json result=Json::array();declarations.visit_committed([&](auto address,auto bytes){
        result.push_back({{"address",address},{"bytes",bytes.size()},{"sha256",digest(memory.view(address,bytes.size()))}});
    });return result;
}
}
int main(int argc,char** argv)try {
#if !defined(__linux__)
    return 77;
#endif
    std::filesystem::path reference,fixture_path,capture;
    for(int i=1;i<argc;i+=2) {
        check(i+1<argc,"Missing native static test argument");const std::string_view option=argv[i];
        if(option=="--reference-xbe")reference=argv[i+1];
        else if(option=="--original-fixture")fixture_path=argv[i+1];
        else if(option=="--capture")capture=argv[i+1];
        else throw std::runtime_error("Unknown native static test argument");
    }
    check(reference.empty()==fixture_path.empty(),"Original static comparison needs both authenticated reference and fixture");
    if(!reference.empty() && (!std::filesystem::is_regular_file(reference) || !std::filesystem::is_regular_file(fixture_path)))return 77;
    SourceRuntimeMemory declarations;mount_native_tu15_static_data(declarations);
    std::size_t declared_bytes=0,regions=0;
    declarations.visit_committed([&](auto,auto bytes){declared_bytes+=bytes.size();++regions;});
    check(declared_bytes==16484 && regions==7,"Static owners include unclassified data");
    Json fixture;
    if(!reference.empty()) {
        auto original=load_source_runtime_image(reference,SourcePhysicsContactImage::TitleUpdate15);
        declarations.visit_committed([&](auto address,auto bytes){
            check(std::ranges::equal(bytes,original.view(address,bytes.size())),"Native static field differs from authenticated TU1.5 data");
        });
        std::ifstream input(fixture_path);fixture=Json::parse(input);
        check(fixture.at("schema")=="H2_NATIVE_TU15_STATIC_ORIGINAL_V1" &&
            fixture.at("source_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
            fixture.at("correspondence_sha256")=="92379da9dc9d29baea9eaca194cebdab7d69f8be319aecf6cd0b759e26e744cc" &&
            fixture.at("instruction_patches").empty() && fixture.at("supplied_gameplay_results").empty(),"Static oracle is not unchanged authenticated TU1.5 execution");
    }
    Json report{{"schema","H2_NATIVE_TU15_STATIC_OWNERS_V1"},{"typed_data_bytes",declared_bytes},{"classified_regions",regions},
        {"reference_executable_is_runtime_input",false},{"world_startup_complete",false},{"normal_gameplay_admission",false},{"steps",Json::array()}};
    constexpr std::array entries{0x3835f0u,0x381370u,0x3825d0u,0x383970u,0x383c90u};
    std::vector<SourcePhysicsContactRecordStore> all_stores;
    {
        SourceRuntimeMemory memory;mount_native_tu15_static_data(memory);
        memory.mount(0x60000000u,std::vector<std::byte>(0x100000),SourceRuntimeMemory::Access::ReadWrite,"native static test caller stack");
        std::vector<std::uint32_t> exits,instructions;
        SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto pc){instructions.push_back(pc);};
        services.required_owner=[&](auto& bank,const auto& call,auto& abi,const auto& observer) {
            check(&bank==&memory,"Static source call crossed memory owners");const auto sp=abi.gp[4];
            if(!call.tail && call.target==0x32b0b4u && (call.instruction==0x3835ffu || call.instruction==0x381389u)) {
                const auto callback=call.instruction==0x3835ffu?0x383c90u:0x383970u;
                check(word(bank,sp)==call.instruction+5u && word(bank,sp+4)==callback,"Static registration native ABI differs");
                exits.push_back(callback);abi.gp[0]=0;abi.gp[4]+=4;return;
            }
            if(call.tail && call.instruction==0x3233d0u && call.target==0xb4150u && abi.gp[1]==0x48d3d8u) {
                source_run_physics_world_runtime(bank,0xdd970u,abi,SourcePhysicsContactImage::TitleUpdate15,{0x27f,0x1f80},services,observer);return;
            }
            throw std::runtime_error("Static source graph reached an undeclared owner");
        };
        const auto observe=[&](const SourcePhysicsContactRecordStore& store){if(store.address<0x60000000u || store.address>=0x60100000u)all_stores.push_back(store);};
        for(unsigned i=0;i<entries.size();++i) {
            if(i>=3){check(!exits.empty() && exits.back()==entries[i],"Source shutdown order differs");exits.pop_back();}
            const auto first_store=all_stores.size(),first_instruction=instructions.size();
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=0x600f0000u;abi.eflags=2;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
            put(memory,abi.gp[4],0x60300000u);
            source_run_physics_live_support_tu15(memory,entries[i],abi,{0x27f,0x1f80},services,observe);
            check(abi.gp[4]==0x600f0004u,"Static source caller frame differs");
            Json stores=Json::array();std::vector<std::uint32_t> trace;
            const auto original_pc=[&](std::uint32_t pc) {
                if(fixture.is_null() || pc<0xdd970u || pc>0xdd99au)return pc;
                std::ostringstream key;key<<std::hex<<std::setfill('0')<<std::setw(8)<<pc;
                return fixture.at("canonical_instruction_map").at(key.str()).get<std::uint32_t>();
            };
            for(auto n=first_store;n<all_stores.size();++n) {
                const auto& store=all_stores[n];stores.push_back({{"instruction",original_pc(store.instruction)},
                    {"address",store.address},{"before",hex(store.before)},{"written",hex(store.written)}});
            }
            for(auto n=first_instruction;n<instructions.size();++n)trace.push_back(original_pc(instructions[n]));
            auto final=buffers(memory,declarations);
            if(!fixture.is_null()) {
                const auto& row=fixture.at("steps").at(i);
                check(row.at("entry")==entries[i] && row.at("returned_gp").get<decltype(abi.gp)>()==abi.gp &&
                    (row.at("returned_eflags").get<std::uint32_t>()&0xcd5u)==(abi.eflags&0xcd5u),"Unchanged static owner returned GP/defined flags differ");
                check(row.at("instructions").get<decltype(trace)>()==trace && row.at("stores")==stores,"Unchanged static owner instruction trace or ordered stores differ");
                auto expected=row.at("buffers");std::sort(expected.begin(),expected.end(),[](const auto& a,const auto& b){return a.at("address")<b.at("address");});
                check(final==expected,"Unchanged static owner output fields differ");
            }
            report["steps"].push_back({{"entry",entries[i]},{"returned_gp",abi.gp},{"returned_eflags",abi.eflags},
                {"instructions",trace},{"stores",stores},{"buffers",final}});
        }
        check(exits.empty() && all_stores.size()==15 && instructions.size()==53,"Incomplete static constructor/shutdown graph");
    }
    // Exercise the real production composition over the completed heap/arena
    // owners, including its retained native exit registrations and lifetime.
    std::vector<SourcePhysicsContactRecordStore> production_stores;
    {
        Directory files;NativeTu15ArenaBootstrap startup(files.path);
        const auto observe=[&](const SourcePhysicsContactRecordStore& store){if(store.address<0x60000000u || store.address>=0x60100000u)production_stores.push_back(store);};
        {
            NativeTu15StaticOwners owners(startup.memory(),observe);
            check(owners.scratch().value>0xffffffffu && owners.deactivator().value>0xffffffffu && owners.engine_table().value>0xffffffffu,
                "Static source storage was narrowed into a source pointer");
            check(word(startup.memory(),0x486088u)==0x4860a0u && word(startup.memory(),0x48608cu)==0x4000u &&
                word(startup.memory(),0x48d3d8u)==0x41f1b0u,"Production static constructors did not publish their original storage");
            bool rejected=false;try{put(startup.memory(),0x47d174u,0);}catch(const std::exception&){rejected=true;}
            check(rejected,"Static owner made an immutable engine class binding writable");
            owners.close();check(word(startup.memory(),0x48d3d8u)==0x45edf0u && word(startup.memory(),0x486080u)==0x45f4a0u,
                "Production source shutdown callbacks did not run");
            owners.close();
        }
    }
    check(production_stores==all_stores,"Production static owners differ from the complete validated source graph");
    report["original_cpu_comparison"]=!fixture.is_null();report["native_instruction_events"]=53;report["ordered_game_stores"]=15;
    report["original_instructions"]=fixture.is_null()?0:53;report["ordered_original_stores"]=fixture.is_null()?0:15;
    report["composed_heap_arena_static_startup"]=true;report["checked_native_addresses"]=true;report["source_shutdown_callbacks"]=true;report["passed"]=true;
    if(!capture.empty()){std::filesystem::create_directories(capture.parent_path());std::ofstream out(capture);out<<report.dump(2)<<'\n';check(bool(out),"Cannot write native static evidence");}
    std::cout<<"Native TU1.5 static owners: 16484 classified bytes, 3 constructors and 2 shutdown callbacks; 53 instructions/15 game stores; heap/arena composition and checked 64-bit storage; "
        <<(fixture.is_null()?"no executable input":"unchanged original CPU comparison passed")<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
