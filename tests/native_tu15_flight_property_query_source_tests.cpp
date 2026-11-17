#include "gameplay/objects/native_tu15_flight_property_query_source_frame.hpp"
#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include <fstream>
#include <iostream>
#include <set>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
namespace flight_reference {
#include "flight_property_query_reference.inc"
}
namespace {
using Banks=std::map<std::uint32_t,std::vector<std::byte>>;
struct Seed {
    SourcePhysicsWorldRuntimeAbi abi{};
    Banks banks;
    explicit Seed(const J& row) {
        abi.gp=row.at("gp");abi.eflags=row.at("eflags");abi.xmm=row.at("xmm");
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
        abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
        for(const auto&bank:row.at("banks"))banks.emplace(bank.at("address"),bytes(bank.at("before")));
    }
};
struct Packet {
    std::array<std::uint32_t,8> gp{};
    std::array<std::array<std::uint32_t,4>,8> xmm{};
    std::array<std::array<std::byte,10>,8> x87{};
    std::uint32_t flags{},fs{},mxcsr{};
    std::uint16_t control{},status{};
    unsigned depth{};
    explicit Packet(const WorldState&state) {
        const auto abi=state.save();gp=abi.gp;xmm=abi.xmm;flags=abi.eflags;fs=abi.fs_base;
        mxcsr=abi.floating_point->mxcsr;control=abi.floating_point->x87_control_word;status=abi.x87_status;
        depth=abi.x87_depth;
        if(abi.active_mmx)throw std::runtime_error("Unexpected active MMX in E0900 source fixture");
        for(unsigned n=0;n<depth;++n)std::memcpy(x87[n].data(),&abi.x87[n],10);
    }
    friend bool operator==(const Packet&,const Packet&)=default;
    J describe()const {
        J live=J::array();for(unsigned n=0;n<depth;++n)live.push_back(hex(x87[n]));
        return J{{"gp",gp},{"xmm",xmm},{"eflags",flags},{"fs",fs},
                 {"fp",J::array({control,status,mxcsr})},{"live_x87",live}};
    }
};
enum class Mark {instruction,read,write,observer};
struct Event {
    Mark kind;
    Packet state;
    std::uint32_t pc,cursor,address;
    unsigned width;
    std::array<std::byte,4> before{},written{};
    friend bool operator==(const Event&,const Event&)=default;
    J describe()const {
        return J{{"kind",unsigned(kind)},{"state",state.describe()},{"entry",pc},{"cursor",cursor},
            {"address",address},{"width",width},{"before",hex(std::span(before).first(width))},
            {"written",hex(std::span(written).first(width))}};
    }
};
struct Record {
    std::vector<Event> trace;
    std::optional<Packet> exit;
    std::uint32_t exit_cursor{};
    Banks banks;
    bool stopped{},observing{};
    friend bool operator==(const Record&,const Record&)=default;
};
struct HarnessMemory final:mopp_ray_frame_test::Memory {
    WorldState* state{};
    SourcePhysicsContactRecordObserver* observer{};
    const SourcePhysicsContactRecordObserver* collector{};
    const std::uint32_t* cursor{};
    std::vector<Event>* trace{};
    int stop{-1},feedback{};
    void record(Mark kind,std::uint32_t pc,std::uint32_t address=0,unsigned width=0,
                std::span<const std::byte> before={},std::span<const std::byte> written={})const {
        if(width>4||before.size()>width||written.size()>width)
            throw std::runtime_error("Unexpected E0900 source operand width");
        auto&event=trace->emplace_back(Event{kind,Packet{*state},pc,*cursor,address,width});
        std::copy(before.begin(),before.end(),event.before.begin());
        std::copy(written.begin(),written.end(),event.written.begin());
        if(int(trace->size())-1==stop)throw Boundary{};
    }
    void read(std::uint32_t address,std::span<std::byte>data)const override {
        record(Mark::read,*cursor,address,data.size());
        const auto bank=at(address,data.size());
        std::copy_n(bank->second.begin()+address-bank->first,data.size(),data.begin());
        if(feedback==2&&*cursor==0xe0999u){
            const std::uint32_t limit=0xc0000000u;std::memcpy(data.data(),&limit,4);
            _mm_setcsr((_mm_getcsr()&~0x6000u)|0x4000u);
        }
    }
    void write(std::uint32_t address,std::span<const std::byte>data)override {
        record(Mark::write,*cursor,address,data.size(),{},data);
        const auto bank=at(address,data.size());auto&output=banks.at(bank->first);
        std::copy(data.begin(),data.end(),output.begin()+address-bank->first);
        if(feedback==4){
            if(*cursor==0xe099fu)*observer=*collector;
            if(*cursor==0xe09e8u)*observer={};
        }
    }
};
struct Mode {bool quiet{},bound{},old_key{},callback_free{};int feedback{};};
std::set<std::uint32_t>coverage;
std::pair<std::uint16_t,unsigned> host_fp() {
    std::uint16_t control;asm volatile("fnstcw %0":"=m"(control));return {control,_mm_getcsr()};
}
template<class State> Record run_typed(const Seed&seed,const Mode&mode,unsigned variant,bool native,int stop) {
    Record result;HarnessMemory memory;memory.banks=seed.banks;
    SourcePhysicsWorldRuntimeServices services;
    SourcePhysicsContactRecordObserver observer;
    std::uint32_t cursor{};const bool active_trace=!mode.quiet;
    memory.cursor=&cursor;memory.trace=&result.trace;memory.stop=stop;memory.feedback=mode.feedback;
    const SourcePhysicsContactRecordObserver collector=[&](const auto&event){
        memory.record(Mark::observer,event.instruction,event.address,event.written.size(),event.before,event.written);
        if(mode.feedback==3){
            if(event.instruction==0xe099fu){
                // Source w() has written but has not retired the modeled pop.
                memory.state->fp.status^=0x200u;
                const auto bank=memory.at(event.address,4);const std::uint32_t value=0xbf000000u;
                auto& data=memory.banks.at(bank->first);
                std::memcpy(data.data()+event.address-bank->first,&value,4);
            }
            if(event.instruction==0xe09e8u){
                memory.state->flags.cf=false;memory.state->flags.zf=!memory.state->flags.zf;
                memory.state->set(3,0x91u,1,0);
            }
        }
    };
    if(mode.feedback!=4)observer=collector;
    memory.observer=&observer;memory.collector=&collector;
    if(!mode.callback_free)services.instruction=[&](std::uint32_t pc){
        memory.record(Mark::instruction,pc);cursor=pc;if(!native)coverage.insert(pc);
        auto&state=*memory.state;
        if(mode.feedback==1){
            if(pc==0xe09c8u)state.flags.zf=!state.flags.zf;
            if(pc==0xe09d2u){state.flags.cf=false;state.flags.zf=!state.flags.zf;}
            if(pc==0xe09e1u)state.xmm[0]=_mm_set_ss(-0.5f);
            if(pc==0xe09fau||pc==0xe0a06u)state.set(0,0xabu,1,0);
        }
        if(mode.feedback==5){
            if(pc==0xe0993u||pc==0xe0995u||pc==0xe0997u)state.fp.status|=0x200u;
            if(pc==0xe099fu){state.fp.st(0)=0x1.000003p0L;state.fp.status&=~0x200u;}
        }
    };
    if((variant&2u)&&!mode.callback_free){
        static constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        services.instruction_cursor={&cursor,&active_trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true};
    }
    SourceEffectCompiledSymbolsTu15 symbols{memory,mode.bound,flight_reference::fixture_name};
    auto abi=seed.abi;
    if(mode.bound&&!mode.old_key){
        const auto address=abi.gp[4]+8u;const auto bank=memory.at(address,4);
        auto&data=memory.banks.at(bank->first);std::uint32_t key{};
        std::memcpy(&key,data.data()+address-bank->first,4);key=flight_reference::fixture_key(key);
        std::memcpy(data.data()+address-bank->first,&key,4);
    }
    const auto outer_fp=host_fp();
    {
        const physics_step_detail::FpScope environment{*abi.floating_point};
        State state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
        memory.state=&state;state.load(abi);
        try{if(native)flight_reference::run_native(state,variant);else flight_reference::run_reference(state,variant);}
        catch(const Boundary&){result.stopped=true;}
        result.exit.emplace(state);result.exit_cursor=cursor;
    }
    check(outer_fp==host_fp(),"Flight source did not restore host FP controls");
    result.banks=std::move(memory.banks);result.observing=bool(observer);return result;
}
Record run(const Seed&seed,const Mode&mode,unsigned variant,bool native,int stop=-1) {
    if(variant&1u)return run_typed<flight_reference::fresh::WorldState>(seed,mode,variant,native,stop);
    return run_typed<flight_reference::current::WorldState>(seed,mode,variant,native,stop);
}
void equal(const Record&actual,const Record&expected,const std::string&name) {
    if(actual==expected)return;
    std::size_t i{};
    while(i<actual.trace.size()&&i<expected.trace.size()&&actual.trace[i]==expected.trace[i])++i;
    const auto a=i<actual.trace.size()?actual.trace[i].describe().dump():"end";
    const auto e=i<expected.trace.size()?expected.trace[i].describe().dump():"end";
    throw std::runtime_error("Flight query retained source differs "+name+" event "+std::to_string(i)+
        " actual="+a+" expected="+e+" exit_equal="+std::to_string(actual.exit==expected.exit&&actual.exit_cursor==expected.exit_cursor)+
        " banks_equal="+std::to_string(actual.banks==expected.banks));
}
} // namespace
int main(int argc,char**argv)try {
    check(argc==2,"Supply original-backed flight query source inputs");
    std::ifstream input(argv[1]);const auto rows=J::parse(input);
    const std::array modes{Mode{},Mode{true},Mode{false,true},Mode{true,true},Mode{false,true,true},
        Mode{false,false,false,true},Mode{false,false,false,false,1},Mode{false,true,false,false,1},
        Mode{false,false,false,false,2},Mode{false,true,false,false,3},Mode{false,false,false,false,4},
        Mode{false,false,false,false,5}};
    unsigned normal{},prefixes{},packets{},profile_witnesses{};
    for(std::size_t n=0;n<rows.size();++n){
        const Seed seed{rows[n]};
        for(std::size_t m=0;m<modes.size();++m){
            const auto&mode=modes[m];
            std::array<Record,4>reference;
            for(unsigned variant=0;variant<4;++variant){
                const auto name=std::to_string(n)+"/mode"+std::to_string(m)+"/profile"+std::to_string(variant);
                auto&expected=reference[variant];expected=run(seed,mode,variant,false);
                check(!expected.stopped,"Unexpected retained flight stop");
                equal(run(seed,mode,variant,true),expected,name);++normal;packets+=expected.trace.size();
                if(n<12&&(m==0||m==1||m==4||m==9||m==10||m==11)){
                    for(std::size_t i=0;i<expected.trace.size();++i){
                        const auto prefix=run(seed,mode,variant,false,int(i));check(prefix.stopped,"Retained flight prefix failed to stop");
                        equal(run(seed,mode,variant,true,int(i)),prefix,name+"/stop"+std::to_string(i));
                        ++prefixes;packets+=prefix.trace.size();
                    }
                }
            }
            if(m==11&&reference[0]!=reference[1]&&reference[2]!=reference[3])++profile_witnesses;
        }
    }
    check(coverage.size()==76,"Retained flight source coverage incomplete");
    check(profile_witnesses!=0,"Flight fresh and current spill/C1 profiles were not distinguished");
    std::cout<<J{{"passed",true},{"normal",normal},{"source_failure_prefixes",prefixes},{"complete_source_packets",packets},
        {"covered_sites",coverage.size()},{"modes",modes.size()},{"raw_prepared_profiles",4},
        {"distinguished_fresh_current_witnesses",profile_witnesses},{"production_bound",false}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
