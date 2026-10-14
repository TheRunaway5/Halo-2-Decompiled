#include "halo2/source_game_storage_startup_tu15.hpp"
#include "halo2/native_tu15_arena_bootstrap.hpp"
#include "halo2/physics_game_arena_runtime.hpp"
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
using namespace halo2;using Json=nlohmann::json;
void check(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
void put(SourcePhysicsContactMemory& m,std::uint32_t p,std::uint32_t value) {
    std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));m.write(p,bytes);
}
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p) {
    auto bytes=m.view(p,4);std::uint32_t value=0;for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);return value;
}
std::string hex(std::span<const std::byte> bytes) {
    std::ostringstream out;for(auto b:bytes)out<<std::hex<<std::setfill('0')<<std::setw(2)<<std::to_integer<unsigned>(b);return out.str();
}
std::string digest(std::span<const std::byte> bytes) {
    std::array<unsigned char,EVP_MAX_MD_SIZE> output{};unsigned size=0;
    check(EVP_Digest(bytes.data(),bytes.size(),output.data(),&size,EVP_sha256(),nullptr)==1,"SHA256 unavailable");return hex(std::as_bytes(std::span(output).first(size)));
}
struct Directory {
    std::filesystem::path path;
    Directory(){
#if defined(__linux__)
        std::string name="/tmp/halo2-native-game-startup-XXXXXX";check(::mkdtemp(name.data()),"Cannot create private startup directory");path=name;
#else
        throw std::runtime_error("Native startup directory unavailable");
#endif
    }
    ~Directory(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
enum class IdentitySpace {Actual,Arena,Control};
struct Scope {
    IdentitySpace& space;IdentitySpace old;
    Scope(IdentitySpace& space,IdentitySpace value):space(space),old(space){space=value;}
    ~Scope(){space=old;}
};
struct MissingSubsystem final : std::exception {};
}
int main(int argc,char** argv)try {
#if !defined(__linux__)
    return 77;
#endif
    std::filesystem::path reference,fixture_path,capture;
    for(int i=1;i<argc;i+=2) {
        check(i+1<argc,"Missing game startup test argument");const std::string_view option=argv[i];
        if(option=="--reference-xbe")reference=argv[i+1];else if(option=="--original-fixture")fixture_path=argv[i+1];
        else if(option=="--capture")capture=argv[i+1];else throw std::runtime_error("Unknown game startup test argument");
    }
    check(reference.empty()==fixture_path.empty(),"Original startup comparison needs both reference and fixture");
    if(!reference.empty() && (!std::filesystem::is_regular_file(reference) || !std::filesystem::is_regular_file(fixture_path)))return 77;
    SourceRuntimeMemory declarations;mount_native_tu15_game_startup_data(declarations);std::size_t bytes=0,regions=0;
    declarations.visit_committed([&](auto,auto b){bytes+=b.size();++regions;});check(bytes==276 && regions==69,"Game startup data includes unclassified storage");
    Json fixture;
    if(!reference.empty()) {
        auto original=load_source_runtime_image(reference,SourcePhysicsContactImage::TitleUpdate15);
        declarations.visit_committed([&](auto p,auto b){check(std::ranges::equal(b,original.view(p,b.size())),"Native startup initializer identity differs from TU1.5");});
        std::ifstream input(fixture_path);fixture=Json::parse(input);
        check(fixture.at("schema")=="H2_NATIVE_TU15_GAME_STARTUP_ORIGINAL_V1" &&
            fixture.at("source_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
            fixture.at("world_correspondence_sha256")=="92379da9dc9d29baea9eaca194cebdab7d69f8be319aecf6cd0b759e26e744cc" &&
            fixture.at("instruction_patches").empty() && fixture.at("supplied_gameplay_results").empty(),"Game startup oracle is not unchanged TU1.5 execution");
    }
    Directory directory;NativeTu15ArenaBootstrap startup(directory.path);auto& memory=startup.memory();mount_native_tu15_game_startup_data(memory);
    constexpr std::uint32_t sp=0x6000f000u;
    const auto frame=[&]{SourcePhysicsWorldRuntimeAbi a{};a.gp[4]=sp;a.eflags=2;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};put(memory,sp,0x60200000u);return a;};
    // An absent owner leaves the real first CALL outstanding, without allocating
    // game options or pretending that the already-constructed arena returned.
    auto missing=frame();bool rejected=false;
    try{source_run_game_storage_startup_tu15(memory,missing,{0x27f,0x1f80},{});}catch(const std::runtime_error&){rejected=true;}
    check(rejected && missing.gp[4]==sp-16 && word(memory,missing.gp[4])==0x13a708u && !word(memory,0x4f3218u),"Missing arena owner did not retain the actual caller frontier");
    IdentitySpace space=IdentitySpace::Actual;std::vector<std::uint32_t> instructions;Json stores=Json::array();
    SourcePhysicsContactRuntimeCall frontier{};
    const auto pc=[&](std::uint32_t value) {
        if(fixture.is_null() || space==IdentitySpace::Actual)return value;
        std::ostringstream key;key<<std::hex<<std::setfill('0')<<std::setw(8)<<value;
        return fixture.at(space==IdentitySpace::Arena?"canonical_instruction_map":"control_instruction_map").at(key.str()).get<std::uint32_t>();
    };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto instruction){instructions.push_back(pc(instruction));};
    services.required_owner=[&](auto& bank,const auto& call,auto& abi,const auto& observer) {
        check(&bank==&memory,"Game startup crossed native bank owners");
        if(call.instruction==0x13a703u && call.target==0x126640u && !call.tail) {
            Scope scope(space,IdentitySpace::Arena);
            source_run_physics_game_arena_runtime(bank,0x125280u,abi,SourcePhysicsContactImage::TitleUpdate15,{0x27f,0x1f80},services,observer);return;
        }
        if(call.instruction==0x13a736u && call.target==0x166a80u && !call.tail) {
            Scope scope(space,IdentitySpace::Actual);source_run_physics_live_support_tu15(bank,0x166a80u,abi,{0x27f,0x1f80},services,observer);return;
        }
        if(call.instruction==0x13a75cu && call.target==0x32b3d6u && !call.tail) {
            Scope scope(space,IdentitySpace::Control);source_run_physics_world_runtime(bank,0x327866u,abi,SourcePhysicsContactImage::TitleUpdate15,{0x27f,0x1f80},services,observer);return;
        }
        if(call.instruction==0x13a766u && !call.tail){frontier=call;throw MissingSubsystem();}
        throw std::runtime_error("Unexpected game startup dependency");
    };
    const auto observe=[&](const SourcePhysicsContactRecordStore& store) {
        if(store.address>=0x60000000u && store.address<0x60100000u)return;
        stores.push_back({{"instruction",pc(store.instruction)},{"address",store.address},{"before",hex(store.before)},{"written",hex(store.written)}});
    };
    auto state=frame();bool stopped=false;
    try{source_run_game_storage_startup_tu15(memory,state,{0x27f,0x1f80},services,observe);}catch(const MissingSubsystem&){stopped=true;}
    check(stopped && frontier.target==0x6de60u && state.gp[6]==0 && state.gp[4]==sp-16 && word(memory,state.gp[4])==0x13a76cu,
        "Game startup completed or lost the first actual subsystem frontier");
    const auto options=word(memory,0x4f3218u);const auto address=memory.native_address(options,0x1200);
    check(address.value>0xffffffffu && word(memory,0x4f2954u)==9356 && word(memory,0x4f295cu)==2408784992u,
        "Native game-options ownership, cursor or source CRC differs");
    check(digest(memory.view(options,0x1200))=="29f46cc48679c4bdb125ee2f8a32451e056532af46c4b0e3e32b46e95473c30a", "Source game-options clearing/selected-BSP state differs");
    check(instructions.size()==1340 && stores.size()==1156,"Game startup prefix source corpus differs");
    // Native x87 stores reserved CW bit 6 as one; the original-code emulator
    // retains the zero requested by FLDCW. Compare every other control bit and
    // report both raw words, without altering the original instruction stream.
    check((state.floating_point->x87_control_word&~0x40u)==0x23fu && state.floating_point->mxcsr==0x1f80u &&
        !state.x87_depth && !state.x87_status && !state.active_mmx && state.xmm==decltype(state.xmm){},
        "Native game startup changed defined floating-point state");
    if(!fixture.is_null()) {
        const auto& abi=fixture.at("game_frontier");
        check(abi.at("gp").get<decltype(state.gp)>()==state.gp && (abi.at("eflags").get<std::uint32_t>()&0xcd5u)==(state.eflags&0xcd5u),"Original game startup frontier GP/defined flags differ");
        check(fixture.at("prefix_instructions").get<decltype(instructions)>()==instructions && fixture.at("prefix_stores")==stores,"Original game startup instruction trace or ordered stores differ");
        check(fixture.at("game_options")==options && fixture.at("game_options_sha256").get<std::string>()==digest(memory.view(options,0x1200)),"Original game-options bytes differ");
        check((abi.at("x87_control").get<unsigned>()&~0x40u)==(state.floating_point->x87_control_word&~0x40u) &&
            abi.at("mxcsr")==state.floating_point->mxcsr && abi.at("x87_status")==state.x87_status,
            "Original game startup defined floating-point controls/status differ");
        for(const auto& xmm:abi.at("xmm"))check(xmm.get<std::string>()==std::string(32,'0'),
            "Original game startup retained XMM state differs");
    }
    bool immutable=false;try{put(memory,0x44aed0u,0);}catch(const std::exception&){immutable=true;}
    check(immutable,"Native startup made subsystem identities writable");
    Json report{{"schema","H2_NATIVE_TU15_GAME_STARTUP_V1"},{"passed",true},{"typed_data_bytes",bytes},{"initializer_slots",68},
        {"instructions",instructions.size()},{"ordered_stores",stores.size()},{"frontier_target",frontier.target},{"frontier_gp",state.gp},
        {"x87_control",state.floating_point->x87_control_word},{"mxcsr",state.floating_point->mxcsr},{"game_options",options},
        {"excluded_reserved_x87_control_bit",6},{"defined_floating_point_state_verified",true},
        {"game_options_native_address_bits",64},{"source_game_storage_complete",false},{"world_startup_complete",false},
        {"reference_executable_is_runtime_input",false},{"original_cpu_comparison",!fixture.is_null()},{"normal_gameplay_admission",false}};
    if(!capture.empty()){std::filesystem::create_directories(capture.parent_path());std::ofstream out(capture);out<<report.dump(2)<<'\n';check(bool(out),"Cannot write native game startup evidence");}
    std::cout<<"Native TU1.5 game startup: 276 typed bytes, original allocation/CRC/clear/control87; 1340 instructions and 1156 stores; retained unimplemented 6DE60 frontier; "
        <<(fixture.is_null()?"no executable input":"unchanged original CPU comparison passed")<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
