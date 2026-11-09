#include "native_tu15_allocator_abi_fixture.hpp"
#include "physics/runtime/native_tu15_resource_busy_abi.hpp"
#ifdef HALO2_BUSY_OWNER_DISPATCH
#include "physics/runtime/native_tu15_allocator_owner_dispatch.hpp"
#endif
#include <iostream>
#include <set>

namespace halo2::resource_busy_reference {
using namespace physics_world_runtime_detail;
#include "resource_busy_reference.inc"
}
namespace halo2 {inline unsigned resource_busy_source_fallbacks{};}
#ifdef HALO2_BUSY_COPY_ROUTES
namespace halo2::resource_busy_copies {
using namespace physics_world_runtime_detail;
#include "resource_busy_copy_routes.inc"
}
#endif
namespace {
using namespace native_allocator_fixture;using halo2::physics_world_runtime_detail::WorldState;
const std::set<std::uint32_t> io{0x12fcb0,0x12fcb4,0x12fcba,0x12fcc5,0x12fcd6,0x12fce0,0x12fceb,0x12fcf5,0x12fce8,0x12fd00,
    0x4057f0,0x405800,0x405801,0x405809,0x40581b,0x405825,0x40583a,0x405841,0x405849,0x405850,
    0x405853,0x405854,0x405856,0x405861,0x405869,0x40586a,0x40586d,0x405870,0x405871};
auto &fallbacks=resource_busy_source_fallbacks;unsigned normal_callbacks{},fault_callbacks{};std::string route;
SourcePhysicsWorldRuntimeAbi incoming(const J &row){
    SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp");a.eflags=row.at("eflags");a.xmm=row.at("xmm");a.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};a.x87_status=row.at("before_fp")[1];a.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<a.x87_depth;++n){const auto bytes=unhex(row.at("live_x87")[n]);std::memcpy(&a.x87[n],bytes.data(),10);}return a;
}
J full_packet(const WorldState &state,std::uint32_t pc){
    const auto a=state.save();J live=J::array();for(unsigned n=0;n<a.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));
    return J{{"entry",pc},{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live}};
}
void reference(WorldState &s,std::uint32_t entry){
    if(entry==0x4057f0u)resource_busy_reference::run_4057f0(s);
    else if(entry==0x12fcb0u)resource_busy_reference::run_12fcb0(s);
    else throw std::runtime_error("Unknown resource busy owner");
}
template<class Before>void execute(WorldState &s,std::uint32_t entry,Before before){
#ifdef HALO2_BUSY_COPY_ROUTES
    if(route=="death"){if(entry==0x4057f0u)resource_busy_copies::death::run_4057f0(s);else resource_busy_copies::death::run_12fcb0(s);return;}
    if(route=="combat"){check(entry==0x4057f0u,"Combat callback body is absent");resource_busy_copies::combat::run_4057f0(s);return;}
#endif
#ifdef HALO2_BUSY_OWNER_DISPATCH
    if(route=="owner"){
        if(native_allocator_owner_detail::dispatch(s,entry,[](auto &v,auto target,auto,auto){reference(v,target);}))return;
        ++fallbacks;reference(s,entry);return;
    }
#endif
    bool accepted{};
    if(entry==0x4057f0u)accepted=native_resource_busy_detail::query(s,before);
    else accepted=native_resource_busy_detail::callback(s,[](auto &v,auto target,auto,auto){reference(v,target);},before);
    if(!accepted){++fallbacks;reference(s,entry);}
}
struct ReadBoundary{};
struct PhysicalMemory:Memory {
    std::uint32_t *pc{};WorldState *state{};mutable J events=J::array(),entries=J::array();int stop_read{-1};std::uint32_t stop_pc{};mutable unsigned ordinal{};
    void read(std::uint32_t address,std::span<std::byte> out)const override{
        if(*pc==stop_pc)throw ReadBoundary{};
        if(state&&(*pc==0x12fcb0u||*pc==0x4057f0u))entries.push_back(full_packet(*state,*pc));
        if(int(ordinal++)==stop_read)throw ReadBoundary{};
        events.push_back(J{{"instruction",*pc},{"address",address},{"bytes",out.size()}});Memory::read(address,out);
    }
};
Memory bank_input(const J &row){Memory memory;for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),unhex(bank.at("before")));return memory;}
struct Faults {unsigned baselines{},memory_prefixes{},observer_prefixes{};std::set<unsigned> live_depths;};
Faults retained(const J &fixture){
    Faults stats;
    for(const auto &row:fixture.at("cases")){
        if(route=="combat"&&row.at("entry")!=0x4057f0u)continue;
        if(!row.value("fault_case",false)||row.contains("fault"))continue;
        const auto initial=incoming(row);stats.live_depths.insert(initial.x87_depth);const auto input=bank_input(row);
        for(unsigned protocol=0;protocol<3;++protocol){unsigned limit{};
            for(unsigned fail=0;;++fail){Memory expected=input,actual=input;if(protocol<2)expected.fail_at=actual.fail_at=fail;
                std::vector<SourcePhysicsContactRecordStore> es,ns;SourcePhysicsContactRecordObserver eo,no;
                if(protocol){eo=[&](const auto &e){es.push_back(e);if(protocol==2&&es.size()==fail)throw std::runtime_error("injected resource observer fault");};
                    no=[&](const auto &e){ns.push_back(e);if(protocol==2&&ns.size()==fail)throw std::runtime_error("injected resource observer fault");};}
                SourcePhysicsWorldRuntimeServices er{},nr{};std::uint32_t epc{},npc{};er.instruction=[&](auto pc){epc=pc;};nr.instruction=[&](auto pc){npc=pc;++fault_callbacks;};cursor(nr,npc);bool tracing=std::getenv("HALO2_REFERENCE_RESOURCE_BUSY");nr.instruction_cursor.first_trace=&tracing;
                WorldState x{expected,eo,er,SourcePhysicsContactImage::TitleUpdate15},y{actual,no,nr,SourcePhysicsContactImage::TitleUpdate15};x.load(initial);y.load(initial);
                std::string ee,ne;try{reference(x,row.at("entry"));}catch(const std::exception &error){ee=error.what();}const auto ex=x.save();
                try{execute(y,row.at("entry").get<std::uint32_t>(),native_page_window_detail::Noop{});}catch(const std::exception &error){ne=error.what();}const auto na=y.save();_mm_setcsr(0x1f80u);
                if(ee!=ne||!same_abi(ex,na)||expected.banks!=actual.banks||expected.accesses!=actual.accesses||es!=ns||epc!=npc){
                    std::cerr<<row.at("name")<<" protocol="<<protocol<<" prefix="<<fail<<" pc="<<std::hex<<epc<<'/'<<npc<<" flags="<<ex.eflags<<'/'<<na.eflags<<'\n';
                    for(unsigned n=0;n<8;++n)if(ex.gp[n]!=na.gp[n])std::cerr<<"gp"<<n<<' '<<ex.gp[n]<<'/'<<na.gp[n]<<'\n';throw std::runtime_error("Resource busy retained prefix differs");}
                if(!fail){++stats.baselines;limit=protocol==2?es.size():expected.operations;}else if(protocol==2)++stats.observer_prefixes;else++stats.memory_prefixes;
                if(fail>=limit)break;
            }
        }
    }
    return stats;
}
}
int main(int argc,char **argv)try{
    check(argc==2||argc==3,"Supply resource-busy fixture and optionalroute");route=argc==3?argv[2]:"candidate";const std::filesystem::path path=argv[1];std::ifstream stream(path);const auto fixture=J::parse(stream);const bool forced=std::getenv("HALO2_REFERENCE_RESOURCE_BUSY");unsigned cases{},packets{},entry_packets{},reads{},stores{};
    for(const auto &row:fixture.at("cases"))for(bool reading:{false,true}){
        if(route=="combat"&&row.at("entry")!=0x4057f0u)continue;
        const bool boundary=row.contains("fault")||row.contains("stop_before_entries");if(boundary&&!reading)continue;
        std::uint32_t pc{};PhysicalMemory memory;static_cast<Memory&>(memory)=bank_input(row);memory.pc=&pc;if(row.contains("fault"))memory.stop_read=row.at("fault").at("ordinal");if(row.contains("stop_before_entries"))memory.stop_pc=row.at("stop_before_entries")[0];
        J captured=J::array(),written=J::array();SourcePhysicsContactRecordObserver observer;
        if(!reading)observer=[&](const auto &e){written.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});};
        WorldState *current{};SourcePhysicsWorldRuntimeServices services;const auto before=[&](WorldState &s,auto site){
            if(row.contains("stop_before_entries")&&std::find(row.at("stop_before_entries").begin(),row.at("stop_before_entries").end(),site)!=row.at("stop_before_entries").end())throw ReadBoundary{};
            captured.push_back(full_packet(s,site));};
        services.instruction=[&](auto site){pc=site;++normal_callbacks;if(io.contains(site))before(*current,site);};cursor(services,pc);services.instruction_cursor.first_trace=&forced;
        WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};current=&state;if(reading)memory.state=&state;state.load(incoming(row));bool stopped{};
        try{execute(state,row.at("entry").get<std::uint32_t>(),before);}catch(const ReadBoundary&){stopped=true;}
        const auto name=row.at("name").get<std::string>();check(stopped==!row.at("completed").get<bool>(),name+" physical completion differs");
        const auto actual=full_packet(state,pc);const auto expected=J{{"entry",row.at("expected_pc")},{"gp",row.at("boundary_gp")},{"eflags",row.at("boundary_eflags")},{"xmm",row.at("boundary_xmm")},{"fp",row.at("after_fp")},{"live_x87",row.at("live_x87")}};
        if(actual!=expected)throw std::runtime_error(name+" complete physical packet differs native="+actual.dump()+" original="+expected.dump());
        if(route!="candidate"&&!forced)check(captured.empty(),"Native source/owner invoked instruction callbacks");
        else if(captured!=row.at("calls")){unsigned n{};while(n<captured.size()&&n<row.at("calls").size()&&captured[n]==row.at("calls")[n])++n;
            throw std::runtime_error(name+" pre-I/O packet differs at"+std::to_string(n)+" native="+(n<captured.size()?captured[n].dump():"end")+" original="+(n<row.at("calls").size()?row.at("calls")[n].dump():"end"));}
        if(reading){check(memory.events==row.at("reads"),name+" ordered physical reads differ");reads+=memory.events.size();if(boundary)packets+=captured.size();
            J expected_entries=J::array();for(const auto &entry:row.at("calls"))if(entry.at("entry")==0x12fcb0u||entry.at("entry")==0x4057f0u)expected_entries.push_back(entry);
            check(memory.entries==expected_entries,name+" fullactual-entry Memory packet differs");entry_packets+=memory.entries.size();}
        else{check(written==row.at("writes"),name+" ordered physical stores differ");stores+=written.size();packets+=captured.size();++cases;}
        for(const auto &bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+" complete bank differs");
        _mm_setcsr(0x1f80u);
    }
    const auto faults=retained(fixture);check(forced||(!normal_callbacks&&!fault_callbacks),"Native resource busy invoked instruction callbacks");
    check(forced?fallbacks>0:fallbacks==0,"Resource busy reference control differs");const auto &activity=native_resource_busy_detail::activity();
    J owner_activity=J::object();
#ifdef HALO2_BUSY_OWNER_DISPATCH
    const auto &a=native_allocator_owner_detail::activity();for(const auto &[entry,count]:a.resource_entries)owner_activity[std::to_string(entry)]=count;
    check(a.entries.empty()&&a.tick_entries.empty()&&a.epoch_entries.empty()&&a.bsp_entries.empty()&&a.bsp_polygon_entries.empty()&&a.delete_entries.empty()&&a.probe_entries.empty(),"Busy owner invoked unrelated actual-owner group");
    check(route=="owner"&&!forced?!a.resource_entries.empty():a.resource_entries.empty(),"Busy actual-owner admission differs");
#endif
    std::cout<<J{{"passed",true},{"cases",cases},{"physical_pre_io_packets",packets},{"complete_actual_entry_packets",entry_packets},{"actual_owner_activity",owner_activity},{"ordered_reads",reads},{"ordered_stores",stores},{"translated_fallbacks",fallbacks},{"normal_instruction_callbacks",normal_callbacks},{"native_queries",activity.queries},{"native_page_callbacks",activity.callbacks},{"reference_query_calls",activity.reference_queries},
        {"retained_faults",J{{"baselines",faults.baselines},{"memory_prefixes",faults.memory_prefixes},{"observer_prefixes",faults.observer_prefixes},{"instruction_callbacks",fault_callbacks},{"live_x87_depths",faults.live_depths}}}}.dump()<<'\n';return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
