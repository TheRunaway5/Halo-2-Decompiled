#include "halo2/source_game_storage_startup_tu15.hpp"
#include "halo2/source_network_storage_startup_tu15.hpp"
#include "halo2/physics_pool_host_storage.hpp"
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
#include <set>
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
enum class IdentitySpace {Actual,Arena,Control,Pool};
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
    SourceRuntimeMemory declarations;mount_native_tu15_game_startup_data(declarations);mount_native_tu15_network_storage_data(declarations);std::size_t bytes=0,regions=0;
    declarations.visit_committed([&](auto,auto b){bytes+=b.size();++regions;});check(bytes==788 && regions==77,"Network startup data includes unclassified storage");
    Json fixture;
    if(!reference.empty()) {
        auto original=load_source_runtime_image(reference,SourcePhysicsContactImage::TitleUpdate15);
        declarations.visit_committed([&](auto p,auto b){check(std::ranges::equal(b,original.view(p,b.size())),"Native startup initializer identity differs from TU1.5");});
        std::ifstream input(fixture_path);fixture=Json::parse(input);
        check(fixture.at("schema")=="H2_NATIVE_TU15_NETWORK_STORAGE_ORIGINAL_V1" &&
            fixture.at("source_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
            fixture.at("world_correspondence_sha256")=="92379da9dc9d29baea9eaca194cebdab7d69f8be319aecf6cd0b759e26e744cc" &&
            fixture.at("pool_correspondence_sha256")=="6d10227d9838a4e6e2e2db443df0c699bbb203a7843a2b6f756a569fd755b527" &&
            fixture.at("instruction_patches").empty() && fixture.at("supplied_gameplay_results").empty(),"Game startup oracle is not unchanged TU1.5 execution");
    }
    Directory directory;NativeTu15ArenaBootstrap startup(directory.path);auto& memory=startup.memory();mount_native_tu15_game_startup_data(memory);mount_native_tu15_network_storage_data(memory);
    std::vector<SourcePhysicsPoolResourceEvent> resources;
    SourcePhysicsPoolHostStorage raw(memory,0x10000000u,[&](const auto& event){resources.push_back(event);});
    constexpr std::uint32_t sp=0x6000f000u;
    const auto frame=[&]{SourcePhysicsWorldRuntimeAbi a{};a.gp[4]=sp;a.eflags=2;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};put(memory,sp,0x60200000u);return a;};
    // An absent owner leaves the real first CALL outstanding, without allocating
    // game options or pretending that the already-constructed arena returned.
    auto missing=frame();bool rejected=false;
    try{source_run_game_storage_startup_tu15(memory,missing,{0x27f,0x1f80},{});}catch(const std::runtime_error&){rejected=true;}
    check(rejected && missing.gp[4]==sp-16 && word(memory,missing.gp[4])==0x13a708u && !word(memory,0x4f3218u),"Missing arena owner did not retain the actual caller frontier");
    IdentitySpace space=IdentitySpace::Actual;std::vector<std::uint32_t> instructions;Json stores=Json::array();
    SourcePhysicsContactRuntimeCall frontier{};
    std::set<std::uint32_t> pool_actual_instruction_sites;
    if(!fixture.is_null()) {
        const auto& mapping=fixture.at("pool_instruction_map");
        for(const auto& row:mapping.items())pool_actual_instruction_sites.insert(row.value().get<std::uint32_t>());
        for(const auto& row:mapping.items())check(!pool_actual_instruction_sites.contains(std::stoul(row.key(),nullptr,16)),
            "Pool canonical and actual instruction namespaces overlap");
    }
    const auto pc=[&](std::uint32_t value,bool instruction_callback=false) {
        if(fixture.is_null() || space==IdentitySpace::Actual)return value;
        if(instruction_callback && space==IdentitySpace::Pool && pool_actual_instruction_sites.contains(value))return value;
        std::ostringstream key;key<<std::hex<<std::setfill('0')<<std::setw(8)<<value;
        const auto* map=space==IdentitySpace::Arena?"canonical_instruction_map":space==IdentitySpace::Pool?"pool_instruction_map":"control_instruction_map";
        return fixture.at(map).at(key.str()).get<std::uint32_t>();
    };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto instruction){instructions.push_back(pc(instruction,true));};
    services.required_owner=[&](auto& bank,const auto& call,auto& abi,const auto& observer) {
        check(&bank==&memory,"Game startup crossed native bank owners");
        if(raw.try_call(bank,call,abi))return;
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
        if(call.instruction==0x13a766u && call.target==0x6de60u && !call.tail) {
            Scope scope(space,IdentitySpace::Actual);source_run_network_storage_startup_tu15(bank,call.target,abi,{0x27f,0x1f80},services,observer);return;
        }
        if(call.instruction==0x7ecadu && call.target==0x11f3f0u && !call.tail) {
            Scope scope(space,IdentitySpace::Pool);source_run_physics_pool_startup_runtime(bank,0x11e150u,abi,SourcePhysicsContactImage::TitleUpdate15,{0x27f,0x1f80},services,observer);return;
        }
        if(call.instruction==0x7ecc9u && call.target==0x16e450u && !call.tail) {
            Scope scope(space,IdentitySpace::Actual);source_run_physics_live_support_tu15(bank,call.target,abi,{0x27f,0x1f80},services,observer);return;
        }
        if(call.instruction==0x6de80u && call.target==0x37654du && !call.tail){frontier=call;throw MissingSubsystem();}
        throw std::runtime_error("Unexpected game startup dependency");
    };
    const auto observe=[&](const SourcePhysicsContactRecordStore& store) {
        if(store.address>=0x60000000u && store.address<0x60100000u)return;
        stores.push_back({{"instruction",pc(store.instruction)},{"address",store.address},{"before",hex(store.before)},{"written",hex(store.written)}});
    };
    auto state=frame();bool stopped=false;
    try{source_run_game_storage_startup_tu15(memory,state,{0x27f,0x1f80},services,observe);}catch(const MissingSubsystem&){stopped=true;}
    check(stopped && frontier.target==0x37654du && state.gp[6]==0 && state.gp[4]==sp-24 && word(memory,state.gp[4])==0x6de85u,
        "Network startup completed or lost its actual XNet caller frontier");
    const auto options=word(memory,0x4f3218u);const auto address=memory.native_address(options,0x1200);
    check(address.value>0xffffffffu && word(memory,0x4f2954u)==9356 && word(memory,0x4f295cu)==2408784992u,
        "Native game-options ownership, cursor or source CRC differs");
    check(digest(memory.view(options,0x1200))=="29f46cc48679c4bdb125ee2f8a32451e056532af46c4b0e3e32b46e95473c30a", "Source game-options clearing/selected-BSP state differs");
    const auto qos=word(memory,0x4dc538u);const auto native_qos=memory.native_address(qos,0x150);
    check(qos==0x10000000u && native_qos.value>0xffffffffu && raw.live_allocations()==1 && resources.size()==1 && resources[0].allocation &&
        resources[0].request==std::array<std::uint32_t,5>{0,0,0x150,0x101000,4} && resources[0].result==std::array<std::uint32_t,3>{0,0x10000000u,0x1000u},
        "Network QoS pool lacks its actual native allocation outcome");
    check(word(memory,qos+0x20u)==0x20 && word(memory,qos+0x24u)==8 && word(memory,0x4dc064u)==1,
        "Network source pool dimensions or callback registration differ");
    check(instructions.size()==1741 && stores.size()==1322,"Network startup prefix source corpus differs");
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
        check(fixture.at("qos_pool")==qos && fixture.at("qos_pool_sha256").get<std::string>()==digest(memory.view(qos,0x150)),
            "Original QoS pool constructor bytes differ");
        for(const auto& region:fixture.at("network_fields"))check(region.at("sha256").get<std::string>()==
            digest(memory.view(region.at("address").get<std::uint32_t>(),region.at("bytes").get<std::size_t>())),"Original complete network data bank differs");
    }
    bool immutable=false;try{put(memory,0x44aed0u,0);}catch(const std::exception&){immutable=true;}
    check(immutable,"Native startup made subsystem identities writable");
    {
        SourceRuntimeMemory foreign;auto refused=state;bool rejected_bank=false;
        try{raw.try_call(foreign,{0x33786fu,0x800000b8u,false},refused);}catch(const std::exception&){rejected_bank=true;}
        check(rejected_bank && refused.gp==state.gp && raw.live_allocations()==1,
            "Shared raw allocator accepted a foreign native bank or changed its caller");
    }
    // After measuring the suspended source prefix, retire the actual owned
    // allocation through its complete source release method. No source job
    // resumes after this teardown-only check.
    {
        SourcePhysicsWorldRuntimeServices release;
        release.required_owner=[&](auto& bank,const auto& call,auto& abi,const auto&) {
            if(!raw.try_call(bank,call,abi))throw std::runtime_error("Unowned QoS teardown dependency");
        };
        SourcePhysicsWorldRuntimeAbi done{};done.gp[4]=sp-0x800u;done.eflags=2;
        done.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
        put(memory,done.gp[4],0x60200000u);put(memory,done.gp[4]+4u,qos);
        source_run_physics_pool_startup_runtime(memory,0x11e180u,done,SourcePhysicsContactImage::TitleUpdate15,{0x27f,0x1f80},release);
        bool released=false;try{memory.source_address(native_qos,1);}catch(const std::exception&){released=true;}
        check(released && raw.live_allocations()==0 && done.gp[4]==sp-0x800u+8u && resources.size()==2 && !resources.back().allocation,
            "QoS source release retained a native allocation or lost its caller frame");
    }
    Json report{{"schema","H2_NATIVE_TU15_NETWORK_STORAGE_V1"},{"passed",true},{"typed_data_bytes",bytes},{"initializer_slots",68},
        {"instructions",instructions.size()},{"ordered_stores",stores.size()},{"frontier_target",frontier.target},{"frontier_gp",state.gp},
        {"x87_control",state.floating_point->x87_control_word},{"mxcsr",state.floating_point->mxcsr},{"game_options",options},
        {"excluded_reserved_x87_control_bit",6},{"defined_floating_point_state_verified",true},
        {"native_qos_pool_address_bits",64},{"native_qos_pool_bytes",0x150},{"native_raw_allocation_bytes",0x1000},
        {"source_qos_pool_constructor_complete",true},{"source_network_startup_complete",false},{"xnet_link_status_returned",false},
        {"native_raw_allocation_request",resources[0].request},{"native_raw_allocation_result",resources[0].result},
        {"foreign_allocator_bank_rejected",true},{"source_release_and_retired_native_pointer_verified",true},
        {"game_options_native_address_bits",64},{"source_game_storage_complete",false},{"world_startup_complete",false},
        {"reference_executable_is_runtime_input",false},{"original_cpu_comparison",!fixture.is_null()},{"normal_gameplay_admission",false}};
    if(!capture.empty()){std::filesystem::create_directories(capture.parent_path());std::ofstream out(capture);out<<report.dump(2)<<'\n';check(bool(out),"Cannot write native game startup evidence");}
    std::cout<<"Native TU1.5 network storage: 788 total typed startup bytes, actual raw allocation and QoS pool; "<<instructions.size()<<" instructions and "<<stores.size()<<" stores; retained XNet link-status frontier; "
        <<(fixture.is_null()?"no executable input":"unchanged original CPU comparison passed")<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
