#include "halo2/source_startup_sound_storage_tu15.hpp"
#include "halo2/source_ui_script_startup_tu15.hpp"
#include "halo2/source_final_startup_storage_tu15.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace halo2;
using Json = nlohmann::json;
namespace {
constexpr std::uint32_t arena=0x81000000, arena_bytes=0x3fe000, object=0x2001000;
constexpr std::uint32_t stack=0x2008000, stop=0x2100000, prior_renderer=0x12000000;
constexpr SourcePhysicsFactoryFloatingPoint fp{0x27f,0x1f80};
void check(bool value,const std::string& message) {if(!value)throw std::runtime_error(message);}
void put(SourceRuntimeMemory& memory,std::uint32_t address,std::uint32_t value) {
    std::array<std::byte,4> bytes{};
    for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(i*8));
    memory.write(address,bytes);
}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    const auto bytes=memory.view(address,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
    return value;
}
std::string hex(std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string value(bytes.size()*2,'0');
    for(std::size_t i=0;i<bytes.size();++i){const auto b=std::to_integer<unsigned>(bytes[i]);value[2*i]=digits[b>>4];value[2*i+1]=digits[b&15];}
    return value;
}
std::vector<std::byte> unhex(const std::string& value) {
    check(!(value.size()%2),"Odd original byte encoding");
    auto nibble=[](char c)->unsigned{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;throw std::runtime_error("Invalid original byte encoding");};
    std::vector<std::byte> bytes(value.size()/2);
    for(std::size_t i=0;i<bytes.size();++i)bytes[i]=std::byte((nibble(value[i*2])<<4)|nibble(value[i*2+1]));
    return bytes;
}
void same(const SourceRuntimeMemory& memory,std::uint32_t address,std::size_t bytes,const Json& expected) {
    const auto original=unhex(expected.get<std::string>());
    std::vector<std::byte> actual(bytes);memory.read(address,actual);
    check(original.size()==bytes&&actual==original,
        "Combined native constructor storage differs at "+std::to_string(address));
}
SourcePhysicsWorldRuntimeAbi seed() {
    SourcePhysicsWorldRuntimeAbi abi{};
    for(unsigned i=0;i<8;++i){abi.gp[i]=0x11000000+i*0x111111;for(unsigned lane=0;lane<4;++lane)abi.xmm[i][lane]=0x3f000000+i*0x10000+lane*0x100;}
    abi.eflags=0x202;abi.floating_point=fp;return abi;
}
void abi_same(const SourcePhysicsWorldRuntimeAbi& abi,const Json& expected,const std::string& where,bool original_mulss_executed=false) {
    check(abi.gp==expected.at("gp").get<decltype(abi.gp)>(),where+": original GP/shadow ABI differs");
    check(abi.xmm==expected.at("xmm").get<decltype(abi.xmm)>(),where+": original incoming XMM/shadow ABI differs");
    check((abi.eflags&0xcc5u)==(expected.at("eflags").get<std::uint32_t>()&0xcc5u),where+": original defined flags differ");
    check(abi.floating_point&&abi.floating_point->x87_control_word==expected.at("x87_control")&&
        (abi.floating_point->mxcsr&~0x3fu)==(expected.at("mxcsr").get<std::uint32_t>()&~0x3fu),where+": original retained FP controls differ; native CW="+
        std::to_string(abi.floating_point?abi.floating_point->x87_control_word:0)+" MXCSR="+
        std::to_string(abi.floating_point?abi.floating_point->mxcsr:0)+" original CW="+expected.at("x87_control").dump()+" MXCSR="+expected.at("mxcsr").dump());
    const auto status_delta=abi.floating_point->mxcsr^expected.at("mxcsr").get<std::uint32_t>();
    // A separate physical-original-instruction test proves the one observed
    // Unicorn limitation:29BF64 sets PE on six captured products. No other
    // sticky difference is admitted; the genuine native status is forwarded
    // untouched into subsequent constructors, rather than cleared to match
    // the emulator. This is not a claim of full FP exception parity.
    check(status_delta==0||(original_mulss_executed&&status_delta==0x20u),where+": unexplained MXCSR sticky status differs");
    check((abi.x87_status&0x3800u)==(expected.at("x87_status").get<std::uint16_t>()&0x3800u)&&!abi.x87_depth&&!abi.active_mmx,
        where+": original x87 TOP/empty register-file state differs");
}
void state_same(const SourceRuntimeMemory& memory,const SourcePhysicsWorldRuntimeAbi& abi,const Json& expected,const std::string& where,bool original_mulss_executed=false) {
    abi_same(abi,expected.at("abi"),where,original_mulss_executed);
    check(word(memory,0x4f2954)==expected.at("arena_cursor")&&word(memory,0x4f2958)==expected.at("arena_top")&&
        word(memory,0x4f295c)==expected.at("crc"),where+": shared arena/CRC state differs");
}
template<class F>void reject(F action) {
    bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}
    check(rejected,"Invalid/foreign/incomplete late-startup owner was admitted");
}
template<class Views>void check_views(const SourceRuntimeMemory& memory,const Views& views,std::size_t count) {
    check(views.regions.size()==count,"Combined native storage view count differs");
    for(const auto& region:views.regions)check(region.native.value>0xffffffffu&&
        memory.source_address(region.native,region.bytes)==region.source,"Combined native view truncates or detaches its wide allocation");
}
void run(const Json& expected,Json& reports) {
    SourceRuntimeMemory memory;
    // Mount every module before executing any constructor. These are genuine
    // loader declarations; a subsequent mounter must not replace live state.
    mount_native_tu15_startup_sound_storage_data(memory);
    mount_native_tu15_ui_script_startup_data(memory);
    mount_native_tu15_final_startup_storage_data(memory);
    const auto fill=expected.at("fill").get<unsigned>();
    memory.mount(arena,std::vector<std::byte>(arena_bytes,std::byte(fill)),SourceRuntimeMemory::Access::ReadWrite,"declared prior native arena");
    memory.mount(0x2000000,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"declared original caller and arena object");
    memory.mount(0x4f2950,std::array<std::byte,16>{},SourceRuntimeMemory::Access::ReadWrite,"declared prior arena state and CRC");
    memory.mount(0x51df2c,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"declared prior arena object binding");
    memory.mount(0x474cbc,std::array<std::byte,4>{std::byte{0x38},std::byte{0x60},std::byte{0x48},std::byte{}},SourceRuntimeMemory::Access::ReadOnly,"original generic pool default owner");
    constexpr std::array<char,16> format{"%s reference"};
    memory.mount(0x466e00,std::as_bytes(std::span(format)),SourceRuntimeMemory::Access::ReadOnly,"original authored reference-name CRT format");
    memory.mount(0x474c20,std::array<std::byte,4>{std::byte{255},std::byte{255},std::byte{255},std::byte{255}},SourceRuntimeMemory::Access::ReadWrite,"declared prior no-scenario index");
    memory.mount(0x4ecc20,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"declared prior unloaded scenario binding");
    memory.mount(0x56e225,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,"original CRC cold flag");
    memory.mount(0x56e258,std::vector<std::byte>(0x400,std::byte(fill)),SourceRuntimeMemory::Access::ReadWrite,"original CRC table backing");
    memory.mount(0x51df44,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"declared prior renderer raw binding");
    memory.mount(prior_renderer,std::vector<std::byte>(0x46000,std::byte(fill)),SourceRuntimeMemory::Access::ReadWrite,"declared prior renderer native pages");
    for(const auto& step:expected.at("steps")) {
        const auto entry=step.at("entry").get<std::uint32_t>();
        std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(entry>>(8*i));
        memory.mount(0x44aed0+step.at("source_offset").get<std::uint32_t>(),bytes,SourceRuntimeMemory::Access::ReadOnly,"authenticated TU1.5 initializer table entry");
    }
    put(memory,0x51df2c,object);put(memory,object,0x45f1d8);put(memory,0x51df44,prior_renderer);
    put(memory,0x4f2950,arena);put(memory,0x4f2954,0x100);put(memory,0x4f2958,0x200);put(memory,0x4f295c,0x12345678);
    NativeTu15StorageFormatting formatting(memory);
    Json allocations=Json::array();
    SourcePhysicsPoolHostStorage host(memory,0x10000000,[&](const auto& event){
        check(event.allocation,"Combined startup unexpectedly released an allocation");
        std::ranges::fill(memory.mutable_view(event.result[1],event.result[2]),std::byte(fill));
        allocations.push_back({{"source",event.result[1]},{"requested",event.request[2]},{"reserved",event.result[2]}});
    });
    Json stores=Json::array();
    SourcePhysicsContactRecordObserver observer=[&](const auto& store){
        stores.push_back({store.instruction,store.address,store.written.size(),hex(store.before),hex(store.written)});
    };
    std::size_t instructions=0;std::uint32_t first_native_mxcsr_precision_pc=0;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto pc){++instructions;if(!first_native_mxcsr_precision_pc&&(_mm_getcsr()&0x20u))first_native_mxcsr_precision_pc=pc;};
    services.required_owner=[](auto&,const auto& call,auto&,const auto&){throw std::runtime_error("Combined original constructor reached unowned target "+std::to_string(call.target));};
    reject([&]{native_tu15_startup_sound_storage_views(memory);});
    reject([&]{native_tu15_ui_script_startup_views(memory);});
    reject([&]{native_tu15_final_startup_storage_views(memory);});
    auto abi=seed();put(memory,stack,stop);abi.gp[4]=stack;
    const auto& prior=expected.at("prior_static_constructor");
    state_same(memory,abi,prior.at("incoming"),"Prior CRT incoming");
    source_run_ui_script_startup_prior_constructor(memory,abi,fp,services,observer);
    state_same(memory,abi,prior.at("returned"),"Prior CRT returned");
    check(instructions==prior.at("instructions")&&stores.size()==prior.at("stores"),"Prior CRT original instruction/store totals differ");
    const auto constructed=word(memory,0x55c454);
    auto before=abi;
    reject([&]{source_run_ui_script_startup_prior_constructor(memory,abi,fp,services,observer);});
    check(abi.gp==before.gp&&word(memory,0x55c454)==constructed,"Rejected repeated CRT changed original state");
    // CRT registration precedes main. Later caller registers are explicit
    // component inputs, rather than accidental CRT machine-register outputs.
    abi=seed();abi.gp[4]=stack;abi.gp[6]=0x654;
    state_same(memory,abi,expected.at("suffix_incoming"),"Component suffix incoming");
    for(const auto& step:expected.at("steps")) {
        const auto slot=step.at("slot").get<unsigned>();
        check(abi.gp[6]==step.at("source_offset")&&slot==abi.gp[6]/0x24+1,"Original initializer ESI chain order differs");
        state_same(memory,abi,step.at("incoming"),"Slot incoming "+std::to_string(slot),slot>59);
        const auto begin=instructions, store_begin=stores.size();
        physics_world_runtime_detail::WorldState state(memory,observer,services,SourcePhysicsContactImage::TitleUpdate15);
        state.load(abi);
        // The four source control instructions are retained literally here
        // solely as a component test driver, not as a cold-startup bypass.
        services.instruction(0x13a766);const auto target=word(memory,0x44aed0+state.gp[6]);
        check(target==step.at("entry"),"Original initializer immutable dispatch differs");
        state.push(0x13a766,0x13a76c);abi=state.save();
        if(slot<=50)source_run_startup_sound_storage_with_native_storage(memory,target,abi,fp,services,host,observer);
        else if(slot<=58)source_run_ui_script_startup_with_native_storage(memory,target,abi,fp,services,host,formatting,observer);
        else source_run_final_startup_storage_with_native_storage(memory,target,abi,fp,services,host,observer);
        state_same(memory,abi,step.at("returned"),"Slot returned "+std::to_string(slot),slot>=59);
        check(instructions-begin==step.at("instructions")&&stores.size()-store_begin==step.at("stores"),"Combined original per-slot instruction/store totals differ");
        state.load(abi);
        services.instruction(0x13a76c);state.set(6,state.flags.plus(state.gp[6],0x24,4),4,0);
        services.instruction(0x13a76f);state.flags.minus(state.gp[6],0x990,4);
        services.instruction(0x13a775);check(state.flags.cf==(slot<68),"Original initializer loop branch differs");
        abi=state.save();
    }
    state_same(memory,abi,expected.at("final"),"Authentic suffix end before13A777",true);
    check(abi.gp[6]==0x990&&abi.gp[4]==stack,"Original initializer suffix did not reach the authentic end");
    check(instructions==expected.at("instructions"),"Combined original instruction total differs");
    check(stores==expected.at("source_stores"),"Combined original ordered before/after source stores differ");
    check(allocations.size()==6&&host.live_allocations()==6&&formatting.calls()==1,"Combined native allocation/format owner counts differ");
    for(std::size_t i=0;i<allocations.size();++i)for(const auto* field:{"source","requested","reserved"})
        check(allocations[i].at(field)==expected.at("allocations")[i].at(field),"Combined original raw allocation differs");
    for(const auto& field:expected.at("fields"))same(memory,field.at("address"),field.at("bytes"),field.at("hex"));
    for(const auto& field:expected.at("allocations_bytes"))same(memory,field.at("source"),field.at("bytes"),field.at("hex"));
    same(memory,arena,arena_bytes,expected.at("arena_hex"));same(memory,0x56e258,0x400,expected.at("crc_table_hex"));
    check_views(memory,native_tu15_startup_sound_storage_views(memory),7);
    check_views(memory,native_tu15_ui_script_startup_views(memory),22);
    check_views(memory,native_tu15_final_startup_storage_views(memory),36);
    // Save/restore all storage with the real raw owner's registry, then rebind
    // every borrowed view against the fresh wide backing. No source result is
    // supplied by this quiescent owner checkpoint transaction.
    const auto saved=memory.checkpoint();const auto ownership=host.ownership_checkpoint();
    SourceRuntimeMemory restored;restored.restore(saved);
    SourcePhysicsPoolHostStorage restored_host(restored);
    auto prepared=restored_host.prepare_ownership(ownership,saved);
    check(restored_host.commit_ownership(std::move(prepared)),"Combined raw ownership restore did not commit");
    check(restored.checkpoint()==saved&&restored_host.ownership_checkpoint()==ownership,"Combined saved storage/ownership changed on restore");
    check_views(restored,native_tu15_startup_sound_storage_views(restored),7);
    check_views(restored,native_tu15_ui_script_startup_views(restored),22);
    check_views(restored,native_tu15_final_startup_storage_views(restored),36);
    auto foreign=seed();SourceRuntimeMemory foreign_memory;SourcePhysicsPoolHostStorage foreign_host(foreign_memory);
    const auto foreign_before=foreign;
    reject([&]{source_run_startup_sound_storage_with_native_storage(memory,0x2131b0,foreign,fp,services,foreign_host);});
    reject([&]{source_run_ui_script_startup_with_native_storage(memory,0x31d400,foreign,fp,services,foreign_host,formatting);});
    reject([&]{source_run_final_startup_storage_with_native_storage(memory,0x31d400,foreign,fp,services,foreign_host);});
    check(foreign.gp==foreign_before.gp&&foreign.xmm==foreign_before.xmm,"Rejected foreign raw owner changed incoming shadow ABI");
    reports.push_back({{"fill",fill},{"instructions",instructions},{"ordered_before_after_stores_compared",stores.size()},
        {"wide_views",65},{"arena_bytes_compared",arena_bytes},{"raw_allocations",6},{"prior_crt_constructors",1},
        {"source_initializer_slots_compared",23},{"final_esi",abi.gp[6]},{"full_storage_and_owner_checkpoint_restored",true},
        {"native_mxcsr",abi.floating_point->mxcsr},{"original_emulator_mxcsr",expected.at("final").at("abi").at("mxcsr")},
        {"only_calibrated_precision_status_difference_admitted",true},
        {"first_native_mxcsr_precision_observed_before_pc",first_native_mxcsr_precision_pc},{"fp_sticky_exception_parity_verified",false},{"passed",true}});
    std::cout<<"Combined original slots46..68 fill "<<fill<<": "<<instructions<<" instructions, "<<stores.size()<<" stores; final ESI990\n";
}
}
int main(int argc,char** argv) {
    try {
        check(argc==2||argc==4,"Expected original late-startup component fixture and optional --report path");
        std::ifstream input(argv[1]);Json fixture;input>>fixture;
        check(fixture.at("passed")==true&&fixture.at("schema")=="H2_ORIGINAL_LATE_STARTUP_COMPONENT_V1"&&
            fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&
            fixture.at("instruction_patches").empty()&&fixture.at("supplied_game_results").empty()&&
            fixture.at("all68_startup_verified")==false,"Original late-startup component evidence is unauthenticated or overclaims");
        Json cases=Json::array();for(const auto& row:fixture.at("cases"))run(row,cases);
        if(argc==4) {
            check(std::string_view(argv[2])=="--report","Unknown combined startup test argument");
            std::ofstream out(argv[3]);out<<Json{{"schema","H2_NATIVE_LATE_STARTUP_COMPONENT_DIFFERENTIAL_V1"},{"passed",true},
                {"cases",cases},{"slot45_verified",false},{"cold_startup_verified",false},{"all68_startup_verified",false},
                {"gameplay_admission",false}}.dump(2)<<'\n';check(bool(out),"Cannot save combined startup report");
        }
        return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
