#include "halo2/cache.hpp"
#include "halo2/native_campaign_session_timers_tu15_binding.hpp"
#include "halo2/original_data.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_execution_inputs.hpp"
#include <array>
#include <filesystem>
#include <iostream>
#include <memory>
#include <unistd.h>

using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
struct Directory {
    std::filesystem::path path;
    Directory(){auto name=(std::filesystem::temp_directory_path()/"halo2-timer-world-XXXXXX").string();
        check(::mkdtemp(name.data()),"Cannot create owned World timer storage");path=name;}
    ~Directory(){std::error_code ec;std::filesystem::remove_all(path,ec);}
};
struct Clock final:PhysicsLiveClockSource {
    std::uint32_t time=100;
    mutable unsigned samples{};
    std::uint32_t milliseconds()const noexcept override{++samples;return time;}
    std::uint64_t cycles()const noexcept override{return 1000000;}
    std::uint64_t utc_filetime()const noexcept override{return 132000000000000000ull;}
};
std::uint32_t read(const SourceRuntimeMemory &memory,std::uint32_t address,unsigned width=4){
    const auto bytes=memory.view(address,width);std::uint32_t result{};
    for(unsigned n=0;n<width;++n)result|=std::to_integer<unsigned>(bytes[n])<<(8*n);return result;
}
void write(SourceRuntimeMemory &memory,std::uint32_t address,std::uint32_t value,unsigned width=4){
    std::array<std::byte,4> bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(8*n));
    memory.write(address,std::span(bytes).first(width));
}
constexpr auto group_stride=0x99cu,slot_stride=0x98u;
constexpr std::array<unsigned,5> live_slots{0,3,7,11,15};
void prepare(SourceRuntimeMemory &memory,bool cached,bool active=true){
    // Declared test input to the genuine77AB0 parent, not a gameplay result.
    write(memory,0x51d018u,1);
    write(memory,0x51d048u,cached,1);write(memory,0x51d04cu,0x89abcdefu);
    for(unsigned group=0;group<5;++group)for(unsigned slot=0;slot<16;++slot){
        const auto state=0x515f70u+group*group_stride+slot*slot_stride;
        write(memory,state,active&&slot==live_slots[group]?3:0);
        write(memory,state+4u,0x11223344u);
    }
}
void verify(const SourceRuntimeMemory &memory,std::uint32_t tick,bool active=true){
    for(unsigned group=0;group<5;++group)for(unsigned slot=0;slot<16;++slot){
        const auto state=0x515f70u+group*group_stride+slot*slot_stride;
        const bool selected=active&&slot==live_slots[group];
        check(read(memory,state)==(selected?4u:0u),"World timer state differs from actual parent selection");
        check(read(memory,state+4u)==(selected?tick:0x11223344u),"World timer timestamp or untouched record differs");
    }
    check(read(memory,0x51d01cu,1)==1&&read(memory,0x51d01du,1)==1,"Actual77AB0 parent input publication differs");
}
void parent(PhysicsLiveWorldTu15 &world,std::uint32_t final_result){
    auto abi=world.prepare_source_call(std::array{1u});const auto sp=abi.gp[4];
    const auto calls=native_campaign_session_timers_calls_tu15();
    world.run_actual(0x77ab0u,abi);
    check(abi.gp[4]==sp+8u,"Actual77AB0 parent RET4 differs");
    check(abi.gp[0]==final_result,"Actual final timer continuation EAX differs");
    check(native_campaign_session_timers_calls_tu15()==calls+16u,"Actual77AD3 callers bypassed native timer binding");
}
}
int main(int argc,char **argv)try{
    if(argc!=2||!std::filesystem::is_regular_file(argv[1]))return 77;
    Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path(),OriginalData::native_tu15());
    check(cache.is_mcc(),"Native timer World acceptance requires prepared MCC content");
    Directory directory;auto clock=std::make_shared<Clock>();
    PhysicsLiveWorldTu15 world(cache,directory.path,0,clock);auto &memory=world.memory();
    bool cold_rejected{};
    try{world.anchor_execution_clocks({0xfffffff0u,1000000,132000000000000000ull});}
    catch(const FormatError &){cold_rejected=true;}
    check(cold_rejected,"Unconfigured cold World admitted an execution clock anchor");
    // Complete the real public campaign lifecycle before asking its clock
    // owner for an execution boundary. Never preset session/HS-ready flags.
    world.initialize_map();world.configure_campaign_session(1,1,30,0x12345678u);
    // Exercise the real adjusted producer, not the raw clock or a new epoch.
    world.anchor_execution_clocks({0xfffffff0u,1000000,132000000000000000ull});
    clock->time+=32u;
    prepare(memory,false);SourceExecutionInputs captured;
    auto samples=clock->samples;
    {SourceExecutionInputBinding binding(memory,captured);parent(world,16u);}
    const auto entries=captured.finish();
    check(entries.size()==5,"World timer producer did not journal one read per live group");
    for(const auto &entry:entries)check(entry.kind==SourceExecutionInputKind::Milliseconds&&entry.operand==0&&entry.value==16u,
        "World timer sampled another clock domain or lost adjusted wrap");
    check(clock->samples==samples+5,"World timer resampled its live clock");verify(memory,16u);
    check(read(memory,0x60212000u)==16u,"Actual33B595 producer did not publish its retained import cell");
    prepare(memory,false);clock->time+=900u;samples=clock->samples;
    SourceExecutionInputs replay(entries);
    {SourceExecutionInputBinding binding(memory,replay);parent(world,16u);}
    replay.finish();check(clock->samples==samples,"World timer replay resampled the platform clock");verify(memory,16u);
    for(const bool active:std::array{true,false}){
        prepare(memory,true,active);SourceExecutionInputs tape;samples=clock->samples;
        {SourceExecutionInputBinding binding(memory,tape);parent(world,active?0x89abcdefu:15u*slot_stride);}
        check(tape.finish().empty()&&clock->samples==samples,"Cached/inactive timer requested a live clock");
        verify(memory,0x89abcdefu,active);
    }
    std::cout<<"PASS actual World77AB0/64 native timer calls, adjusted wrap, five journaled clock reads, replay, cached and inactive records; native MCC startup without executable inputs\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
