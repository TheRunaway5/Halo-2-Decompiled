#include "gameplay/objects/native_tu15_map_world_property_query_source_frame.hpp"
#include "gameplay/objects/native_tu15_map_world_property_caller_source_frame.hpp"
#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include <fstream>
#include <iostream>
#include <set>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
namespace {
#include "map_world_property_caller_reference.inc"

using Banks=std::map<std::uint32_t,std::vector<std::byte>>;
struct Seed {
    SourcePhysicsWorldRuntimeAbi abi{};
    Banks banks;
    explicit Seed(const J&row){
        abi.gp=row.at("gp");abi.eflags=row.at("eflags");abi.xmm=row.at("xmm");
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
        abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
        for(const auto&bank:row.at("banks"))banks.emplace(bank.at("address"),bytes(bank.at("before")));
    }
};
struct Record {
    J trace=J::array(),exit;
    Banks banks;
    bool stopped{},observing{};
    friend bool operator==(const Record&,const Record&)=default;
};
struct HarnessMemory final:mopp_ray_frame_test::Memory {
    WorldState*state{};const std::uint32_t*position{};
    J*trace{};int stop{-1},feedback{};
    SourcePhysicsContactRecordObserver*observer{};
    const SourcePhysicsContactRecordObserver*collector{};
    void record(std::string_view kind,std::uint32_t site,J operand)const{
        trace->push_back(J{{"kind",kind},{"state",packet(*state,site)},{"cursor",*position},{"operand",std::move(operand)}});
        if(int(trace->size())-1==stop)throw Boundary{};
    }
    void read(std::uint32_t address,std::span<std::byte>data)const override{
        record("read",*position,J::array({address,data.size()}));
        mopp_ray_frame_test::Memory::read(address,data);
        if(feedback==2){
            _mm_setcsr((_mm_getcsr()&~0x6000u)|0x4000u);
            if(*position==0x1db825u){const std::uint32_t denominator=0xc0000000u;std::memcpy(data.data(),&denominator,4);}
        }
    }
    void write(std::uint32_t address,std::span<const std::byte>data)override{
        record("write",*position,J::array({address,hex(data)}));
        mopp_ray_frame_test::Memory::write(address,data);
        if(feedback==4){
            if(*position==0x1db857u)*observer=*collector;
            if(*position==0x1db866u)*observer={};
        }
    }
};
struct Mode {bool literal{},quiet{},bound{},old_key{};int feedback{};};
std::set<std::uint32_t>coverage;
Record run(const Seed&seed,const Mode&mode,bool native,int stop=-1){
    Record result;HarnessMemory memory;memory.banks=seed.banks;
    SourcePhysicsWorldRuntimeServices services;
    SourcePhysicsContactRecordObserver observer;
    std::uint32_t cursor{};const bool trace_enabled=!mode.quiet;
    memory.position=&cursor;memory.trace=&result.trace;memory.stop=stop;memory.feedback=mode.feedback;
    const SourcePhysicsContactRecordObserver collector=[&](const auto&event){
        memory.record("observer",event.instruction,J::array({event.address,hex(event.before),hex(event.written)}));
        if(mode.feedback==3&&event.instruction==0x1db857u){
            memory.state->flags.cf=false;memory.state->flags.zf=!memory.state->flags.zf;
            memory.state->set(3,0x91u,1,0);
        }
    };
    if(mode.feedback!=4)observer=collector;
    memory.observer=&observer;memory.collector=&collector;
    services.instruction=[&](std::uint32_t pc){
        memory.record("instruction",pc,J{});cursor=pc;if(!native)coverage.insert(pc);
        if(mode.feedback==1){
            auto&state=*memory.state;
            // Preserve the declared mapped pointer for scalar/argument aliases.
            // Clearing this flag would store a null byte-output pointer.
            if(pc==0x1db84fu&&state.gp[2]!=state.gp[4]+4u)state.flags.zf=!state.flags.zf;
            if(pc==0x1db85bu){state.flags.cf=false;state.flags.zf=!state.flags.zf;}
            if(pc==0x11e4e4u)state.flags.zf=!state.flags.zf;
            if(pc==0x11e4e6u)state.set(3,0xa5u,1,0);
            if(pc==0x1db868u||pc==0x1db873u)state.set(0,0xa5u,1,0);
            if(pc==0x1db825u||pc==0x1db837u)state.xmm[0]=_mm_set_ss(std::bit_cast<float>(0x7fc0a55au));
        }
    };
    if(mode.literal){
        static constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        services.instruction_cursor={&cursor,&trace_enabled,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true};
    }
    SourceEffectCompiledSymbolsTu15 symbols{memory,mode.bound,map_world_query_fixture_name};
    auto abi=seed.abi;
    if(mode.bound&&!mode.old_key){
        const auto address=abi.gp[4]+8u;const auto bank=memory.at(address,4);
        auto&data=memory.banks.at(bank->first);std::uint32_t key{};
        std::memcpy(&key,data.data()+address-bank->first,4);key=map_world_query_fixture_key(key);
        std::memcpy(data.data()+address-bank->first,&key,4);
    }
    WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};memory.state=&state;
    const physics_step_detail::FpScope scope{*abi.floating_point};state.load(abi);
    try{
        if(mode.literal){
            if(native)native_map_world_property_caller_detail::run_source_frame<true>(state,[](auto& child){
                native_map_world_property_query_detail::run_source_frame<true>(child);
            });
            else retained_map_world_caller_literal(state);
        }else{
            if(native)native_map_world_property_caller_detail::run_source_frame<false>(state,[](auto& child){
                native_map_world_property_query_detail::run_source_frame<false>(child);
            });
            else retained_map_world_caller_raw(state);
        }
    }catch(const Boundary&){result.stopped=true;}
    result.exit=packet(state,cursor);result.banks=std::move(memory.banks);result.observing=bool(observer);return result;
}
void equal(const Record&actual,const Record&expected,const std::string&name){
    if(actual==expected)return;
    std::size_t i{};
    while(i<actual.trace.size()&&i<expected.trace.size()&&actual.trace[i]==expected.trace[i])++i;
    const auto a=i<actual.trace.size()?actual.trace[i].dump():"end";
    const auto e=i<expected.trace.size()?expected.trace[i].dump():"end";
    throw std::runtime_error("MapWorld caller retained source differs "+name+" event "+std::to_string(i)+
        " actual="+a+" expected="+e+" exit_equal="+std::to_string(actual.exit==expected.exit)+
        " banks_equal="+std::to_string(actual.banks==expected.banks));
}
} // namespace
int main(int argc,char**argv)try{
    check(argc==2,"Supply original-backed MapWorld property source inputs");std::ifstream input(argv[1]);const auto rows=J::parse(input);
    const std::array modes{Mode{},Mode{true},Mode{true,true},Mode{false,false,true},Mode{true,false,true},
        Mode{true,true,true},Mode{false,false,true,true},Mode{false,false,false,false,1},
        Mode{true,false,true,false,1},Mode{false,false,false,false,2},Mode{true,false,true,false,3},Mode{false,false,false,false,4}};
    unsigned normal{},prefixes{},packets{};
    for(std::size_t n=0;n<rows.size();++n){
        const Seed seed{rows[n]};
        for(std::size_t m=0;m<modes.size();++m){
            const auto&mode=modes[m];
            const auto name=std::to_string(n)+"/mode"+std::to_string(m);
            const auto expected=run(seed,mode,false);
            check(!expected.stopped,"Unexpected retained stop");
            equal(run(seed,mode,true),expected,name);++normal;packets+=expected.trace.size();
            if(n<8&&(m==0||m==1||m==2||m==4||m==10||m==11)){
                for(std::size_t i=0;i<expected.trace.size();++i){
                    const auto prefix=run(seed,mode,false,int(i));check(prefix.stopped,"Retained prefix failed to stop");
                    equal(run(seed,mode,true,int(i)),prefix,name+"/stop"+std::to_string(i));
                    ++prefixes;packets+=prefix.trace.size();
                }
            }
        }
    }
    check(coverage.size()==64,"Retained MapWorld caller source coverage incomplete");
    std::cout<<J{{"passed",true},{"normal",normal},{"source_failure_prefixes",prefixes},{"complete_source_packets",packets},
        {"covered_sites",coverage.size()},{"modes",modes.size()},{"production_bound",false}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
